#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <pthread.h>
#include <utils/Log.h>
#include <errno.h>  
#include <ctype.h>
#include <dirent.h>	
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include "cutils/properties.h" 	      
#include "packet.h"

#define	MODEM_SOCKET_NAME	"modemd"
#define	MAX_CLIENT_NUM		10
#define	MODEM_PATH		"/dev/vbpipe2"
#define	MODEM_STATE_PATH	"/sys/devices/platform/modem_interface/state"

#define	DLOADER_PATH		"/dev/dloader"

#define DATA_BUF_SIZE (128)
#define TEST_BUFFER_SIZE (33 * 1024)


typedef enum _MOMDEM_STATE{
	MODEM_STA_SHUTDOWN,
	MODEM_STA_BOOT,
	MODEM_STA_INIT,
	MODEM_STA_ALIVE,
	MODEM_STA_ASSERT,
	MODEM_STA_BOOTCOMP,
	MODEM_STA_MAX
} MODEM_STA_E;

extern int modem_boot(void);
extern int  send_start_message(int fd,int size,unsigned long addr,int flag);
static char test_buffer[TEST_BUFFER_SIZE]={0};
static char log_data[DATA_BUF_SIZE];
static int  client_fd[MAX_CLIENT_NUM];
static MODEM_STA_E modem_state = MODEM_STA_SHUTDOWN;
static int flash_less_flag = 0xff;

#define MONITOR_APP	"/system/bin/sprd_monitor"
static int reset_modem_if_assert(void)
{
    char prop[16]={0};

    property_get("persist.sys.sprd.modemreset", prop, "");

    printf("modem reset: %s\n", prop);

    if(!strncmp(prop, "1", 2)){
	return 1;
    }
    return 0;
}

static int poweron_by_charger(void)
{
    char charge_prop[16]={0};
    property_get("ro.bootmode", charge_prop, "");

    printf("charge_prop: %s\n", charge_prop);

    if(!strncmp(charge_prop, "charge", 6)){
	return 1;
    }
    return 0;
}
/* helper function to get pid from process name */
static int get_task_pid(char *name)
{
    DIR *d;
    struct dirent *de;
    char cmdline[1024];

    d = opendir("/proc");
    if (d == 0) return -1;

    while ((de = readdir(d)) != 0) {
        if(isdigit(de->d_name[0])) {
            int pid = atoi(de->d_name);
            int fd, ret;
            sprintf(cmdline, "/proc/%d/cmdline", pid);
            fd = open(cmdline, O_RDONLY);
            if (fd < 0) continue;
            ret = read(fd, cmdline, 1023);
            close(fd);
            if (ret < 0) ret = 0;
            cmdline[ret] = 0;
            if (strcmp(name, cmdline) == 0) {
                closedir(d);
                return pid;
            }
        }
    }
    closedir(d);
    return -1;
}
static void print_log_data(char *buf, int cnt)
{
	int i;

	if (cnt > DATA_BUF_SIZE)
		cnt = DATA_BUF_SIZE;

	printf("received:\n");
	for(i = 0; i < cnt; i++) {
		printf("%c ", buf[i]);
	}
	printf("\n");
}
static int get_modem_state(char *buffer,size_t size)
{
	int modem_state_fd = 0;
	ssize_t numRead;
	modem_state_fd = open(MODEM_STATE_PATH, O_RDONLY);
	if(modem_state_fd < 0)
		return 0;
	numRead = read(modem_state_fd,buffer,size);
	close(modem_state_fd);
	if(numRead > 0){
		printf("modem_state = %s\n",buffer);
	} else{
		printf("modem_state error...\n");
		return 0;
	}
	return numRead;
}
static void reset_modem_state(void)
{
	int modem_state_fd = 0;
	ssize_t numRead;

	modem_state_fd = open(MODEM_STATE_PATH, O_WRONLY);
	if(modem_state_fd < 0)
		return ;
	numRead = write(modem_state_fd,"0",2);
	close(modem_state_fd);
}
static void *modemd_listenaccept_thread(void *par)
{
	int sfd, n, i;
	char buffer[64] = {0};
	unsigned short *data = (unsigned short *)buffer;
	
	for(i=0; i<MAX_CLIENT_NUM; i++)
		client_fd[i]=-1;
	
	sfd = socket_local_server(MODEM_SOCKET_NAME, 
		0,/*ANDROID_SOCKET_NAMESPACE_RESERVED,*/ SOCK_STREAM);
	if (sfd < 0) {
		printf("modemd_listenaccept_thread: cannot create local socket server\n");
		exit(-1);
	}

	for(; ;){

		printf("modemd_listenaccept_thread: Waiting for new connect ...\n");
		if ( (n=accept(sfd,NULL,NULL)) == -1)
		{			
			printf("engserver accept error\n");
			continue;
		}

		printf("modemd_listenaccept_thread: accept client n=%d\n",n);
		for(i=0; i<MAX_CLIENT_NUM; i++) {
			if(client_fd[i]==-1){
				client_fd[i]=n;
				printf("modemd_listenaccept_thread: fill %d to client[%d]\n", n, i);
				break;
			}
		}
	}
}
static int get_modem_assert_information(char *assert_info,int size)
{
        extern int open_uart_device(int mode);
        char ch, *buffer;
        int     read_len=0,ret=0,timeout;

        if((assert_info == NULL) || (size == 0))
                return 0;

        int uart_fd = open_uart_device(1);

        if(uart_fd < 0){
               printf("open_uart_device failed \n");
               return 0;
        }
        ch = 'P';
        ret = write(uart_fd,&ch,1);

        buffer = assert_info;
        read_len = 0;
        timeout=0;
        do{
                ret = read(uart_fd,buffer,size);
                timeout++;
                if(ret > 0){
                        printf("read_len[%d] = %d \n",timeout,read_len);
                        size -= ret;
                        read_len += ret;
                        buffer += ret;
                }
                if(timeout > 100000)
                        break;
        }while(size >0);
        ch = 'O';
        write(uart_fd,&ch,1);
        close(uart_fd);
        return read_len;
}

static int translate_modem_state_message(char *message)
{
	int modem_gpio_mask;
	sscanf(message, "%d", &modem_gpio_mask);
	return modem_gpio_mask;
}
static void broadcast_modem_state(char *message,int size)
{
	int i,ret;
	for(i=0; i<MAX_CLIENT_NUM; i++) {
		if(client_fd[i] > 0) {
			printf("client_fd[%d]=%d\n",i, client_fd[i]);
			ret = write(client_fd[i], message, size);
			if(ret > 0)
				printf("write %d bytes to client socket [%d] to reset modem\n", ret, client_fd[i]);
			else {
				printf("%s error: %s\n",__func__,strerror(errno));
				close(client_fd[i]);
				client_fd[i] = -1;
			}
		}
	}
}

static void process_modem_state_message(char *message,int size)
{
	int alive_status=0,reset_status = 0;
	int ret=0;
        int first_alive = 0;

	ret = translate_modem_state_message(message);
	if(ret < 0)
		return;
	alive_status = ret & 0x1;
	reset_status = ret >> 1;
	// check flash less or not and first alive 
	if(flash_less_flag == 0xff){
		if(alive_status == 1)
		{
			flash_less_flag = 0;
			first_alive = 1; // first alive for nand
		}
		else
			flash_less_flag = 1;
	} else if(flash_less_flag == 1){
		if(alive_status == 1){
			flash_less_flag = 2;
			first_alive = 1; // first alive for flash less
		}
	}
	printf("alive=%d reset=%d flash_less = %d\n",alive_status,reset_status,flash_less_flag);
	switch(modem_state){
		case MODEM_STA_SHUTDOWN:
		break;
        	case MODEM_STA_INIT:
		{
			if(reset_status==0){
				if(alive_status == 1){
					int pid;
					modem_state = MODEM_STA_ALIVE;
					printf("modem_state0 = MODEM_STA_ALIVE\n"); 
					pid = get_task_pid(MONITOR_APP);
					if((pid > 0)&&(!first_alive)){
						kill(pid, SIGUSR2);
						printf("Send SIGUSR2 to MONITOR_APP\n"); 
					}
				} else {
					modem_state = MODEM_STA_BOOT;
					printf("modem_state1 = MODEM_STA_BOOT\n");
				}
			}
		}
		break;
		case MODEM_STA_ASSERT:
			if(reset_status==1){
				
				int pid;

				pid = get_task_pid(MONITOR_APP);

				if(pid > 0)
					kill(pid, SIGUSR1);
				printf("modem_state2 = MODEM_STA_BOOT\n");
				modem_state = MODEM_STA_BOOT;
			}
		break;
		case MODEM_STA_BOOTCOMP:
			if(reset_status==0){
				if(alive_status == 1){
					int pid;
					modem_state = MODEM_STA_ALIVE;
					printf("modem_state5 = MODEM_STA_ALIVE\n"); 
					broadcast_modem_state("Modem Alive",strlen("Modem Alive"));
                                        /*start bt&wifi set mac app*/
                                        if(first_alive)
                                            property_set("ctl.start", "engsetmacaddr");

					pid = get_task_pid(MONITOR_APP);
					if((pid > 0)&&(!first_alive)){
						kill(pid, SIGUSR2);
						printf("Send SIGUSR2 to MONITOR_APP\n"); 
					}
				}
			}
		break;
        	case MODEM_STA_ALIVE:
			if(alive_status == 0){
				modem_state = MODEM_STA_ASSERT;
                                memset(test_buffer,0,300);
                                ret = get_modem_assert_information(test_buffer,256);
                                printf("modem_state4 = MODEM_STA_ASSERT(%d)\n",ret);
                                if(ret > 0){
                                        broadcast_modem_state(test_buffer,strlen(test_buffer));
                                } else {
                                        broadcast_modem_state("Modem Assert\n",strlen("Modem Assert\n"));
                                }
			}
			
			if((reset_status==1) || reset_modem_if_assert()){
				int pid;
				pid = get_task_pid(MONITOR_APP);
				if((pid > 0)&&(!first_alive)){
					kill(pid, SIGUSR1);
					printf("Send SIGUSR1 to MONITOR_APP\n");
				}
				printf("modem_state2 = MODEM_STA_BOOT\n");
				modem_state = MODEM_STA_BOOT;
			}
		break;
		default:
		break;
	}
}
void signal_pipe_handler(int sig_num)
{
	printf("receive signal SIGPIPE\n");
}
int main(int argc, char *argv[])
{
	int ret, i;
	char buf[DATA_BUF_SIZE]={0};	
	pthread_t t1;
	int priority;
	pid_t pid;
	struct sigaction act;

	int modem_state_fd = open(MODEM_STATE_PATH, O_RDONLY);

	printf(">>>>>> start modem manager program ......\n");
	if(poweron_by_charger() == 1){
		printf(">>>>>> power on by charger,modem_reboot exits...\n");
		return 0;
	}
	if(modem_state_fd < 0){
		printf("!!! Open %s failed, modem_reboot exit\n",MODEM_STATE_PATH);
		return 0;
	}
        memset (&act, 0x00, sizeof(act));
        act.sa_handler = &signal_pipe_handler;
        act.sa_flags = SA_NODEFER;
        sigfillset(&act.sa_mask);   //block all signals when handler is running.
        ret = sigaction (SIGPIPE, &act, NULL);
        if (ret < 0) {
            perror("sigaction() failed!\n");
            exit(1);
        }

	pid = getpid();
	priority = getpriority(PRIO_PROCESS,pid);
	setpriority(PRIO_PROCESS,pid,-15);

	close(modem_state_fd);

	{
                extern int get_modem_images_info(void);
                extern void print_modem_image_info(void);
                get_modem_images_info();
                print_modem_image_info();
	}

	pthread_create(&t1, NULL, (void*)modemd_listenaccept_thread, NULL);
	
	modem_state = MODEM_STA_INIT;
	do{
		memset(buf,0,sizeof(buf));
		ret = get_modem_state(buf,sizeof(buf));
		if(ret > 0){
			process_modem_state_message(buf,ret);
		}
		if(modem_state == MODEM_STA_BOOT){
			modem_boot();
			sleep(1);
			modem_state = MODEM_STA_BOOTCOMP;
			continue;
		}
	}while(1);
	return 0;
}
