#define LOG_TAG 	"MODEMD"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <signal.h>
#include "modemd.h"

int  client_fd[MAX_CLIENT_NUM];
static char ttydev[12];
static int ttydev_fd;

/*
 * Returns 1 if found, 0 otherwise. needle must be null-terminated.
 * strstr might not work because WebBox sends garbage before the first OK read
 *
 */
int findInBuf(unsigned char *buf, int len, char *needle)
{
	int i;
	int needleMatchedPos = 0;

	if (needle[0] == '\0') {
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (needle[needleMatchedPos] == buf[i]) {
			needleMatchedPos++;
			if (needle[needleMatchedPos] == '\0') {
				/* Entire needle was found */
				return 1;
			}
		} else {
			needleMatchedPos = 0;
		}
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

static int stop_engservice(int modem)
{
	MODEMD_LOGD("stop engservice!");
	switch(modem){
		case TD_MODEM:
			property_set("ctl.stop", "engservicet");
			property_set("ctl.stop", "engmodemclientt");
			property_set("ctl.stop", "engpcclientt");
			break;
		case W_MODEM:
			property_set("ctl.stop", "engservicew");
			property_set("ctl.stop", "engmodemclientw");
			property_set("ctl.stop", "engpcclientw");
			break;
		default:
			property_set("ctl.stop", "engservicet");
			property_set("ctl.stop", "engmodemclientt");
			property_set("ctl.stop", "engpcclientt");
			break;
	}

	return 0;
}

static int start_engservice(int modem)
{
	MODEMD_LOGD("start engservice!");

	switch(modem){
		case TD_MODEM:
			property_set("ctl.start", "engservicet");
			property_set("ctl.start", "engmodemclientt");
			property_set("ctl.start", "engpcclientt");
			break;
		case W_MODEM:
			property_set("ctl.start", "engservicew");
			property_set("ctl.start", "engmodemclientw");
			property_set("ctl.start", "engpcclientw");
			break;
		default:
			property_set("ctl.start", "engservicet");
			property_set("ctl.start", "engmodemclientt");
			property_set("ctl.start", "engpcclientt");
			break;
	}
	return 0;
}

static int stop_phser(int modem)
{
	switch(modem) {
	case W_MODEM:
		MODEMD_LOGD("stop w phoneserver!");
		property_set("ctl.stop", "phoneserver_w");
		break;
	case TD_MODEM:
	default:
		MODEMD_LOGD("stop td phoneserver!");
		property_set("ctl.stop", "phoneserver_t");
		break;
	}
	return 0;
}

static int start_phser(int modem)
{
	switch(modem) {
	case W_MODEM:
		MODEMD_LOGD("start w phoneserver!");
		property_set("ctl.start", "phoneserver_w");
		break;
	case TD_MODEM:
	default:
		MODEMD_LOGD("start td phoneserver!");
		property_set("ctl.start", "phoneserver_t");
		break;
	}
	return 0;
}

static int stop_rild(int modem)
{
	char phoneCount[5] = {0};

	/* stop rild */
	if(modem == TD_MODEM) {
		MODEMD_LOGD("stop td rild!");
		property_get(TD_SIM_NUM, phoneCount, "");
		if(!strcmp(phoneCount, "2")) {
			property_set("ctl.stop", "tril-daemon");
			property_set("ctl.stop", "tril-daemon1");
		} else {
			property_set("ctl.stop", "tril-daemon");
		}
	} else if(modem == W_MODEM) {
		MODEMD_LOGD("stop w rild!");
		property_get(W_SIM_NUM, phoneCount, "");
		if(!strcmp(phoneCount, "2")) {
			property_set("ctl.stop", "wril-daemon");
			property_set("ctl.stop", "wril-daemon1");
		} else {
			property_set("ctl.stop", "wril-daemon");
		}
	}

	return 0;
}

static int start_rild(int modem)
{
	char phoneCount[5] = {0};

	/* start rild */
	if(modem == TD_MODEM) {
		MODEMD_LOGD("start td rild!");
		property_get(TD_SIM_NUM, phoneCount, "");
		if(!strcmp(phoneCount, "2")) {
			property_set("ctl.start", "tril-daemon");
			property_set("ctl.start", "tril-daemon1");
		} else {
			property_set("ctl.start", "tril-daemon");
		}
	} else if(modem == W_MODEM) {
		MODEMD_LOGD("start w rild!");
		property_get(W_SIM_NUM, phoneCount, "");
		if(!strcmp(phoneCount, "2")) {
			property_set("ctl.start", "wril-daemon");
			property_set("ctl.start", "wril-daemon1");
		} else {
			property_set("ctl.start", "wril-daemon");
		}
	}

	return 0;
}

int stop_service(int modem, int is_vlx)
{
	pid_t pid;
	char pid_str[32] = {0};

	MODEMD_LOGD("enter stop_service!");

	/* stop eng */
	stop_engservice(modem);

	/* stop phoneserver */
	stop_phser(modem);

	/* close ttydev */
	if (is_vlx == 1) {
		if (ttydev_fd >= 0)
			close(ttydev_fd);
	}

	/* stop rild */
	stop_rild(modem);

	/* restart com.android.phone */
	pid = get_task_pid(PHONE_APP);
	if (pid > 0) {
		MODEMD_LOGD("restart %s (%d)!", PHONE_APP, pid);
		snprintf(pid_str, sizeof(pid_str), "%d", pid);
		property_set(PHONE_APP_PROP, pid_str);
	}
	if(modem == TD_MODEM)
		property_set("ctl.start", "kill_td_phone");
	else if(modem == W_MODEM)
		property_set("ctl.start", "kill_w_phone");

	return 0;
}

int start_service(int modem, int is_vlx, int restart)
{
	char mux_2sim_swap[]="echo 1 > /proc/mux_mode";
	char mux_3sim_swap[]="echo 2 > /proc/mux_mode";
	char phoneCount[5];
	char path[32];
	int stty_fd;
        char modem_dev[20];
	int ret, length;
	char at_str[20];
	unsigned char buffer[50] = {0};
	unsigned char *p_buf = buffer;
	int count =0, resend = 0;

	MODEMD_LOGD("enter start_service!");

	/* open vuart/ttyNK for vlx modem */
	if(is_vlx == 1) {
		property_get(PROP_TTYDEV, ttydev, "ttyNK3");
		sprintf(path, "/dev/%s", ttydev);
		MODEMD_LOGD("open tty dev: %s", path);
		ttydev_fd = open(path, O_RDWR);
		if (ttydev_fd < 0)
			MODEMD_LOGE("Failed to open %s!\n", path);
		if(modem == TD_MODEM)
			property_get(TD_SIM_NUM, phoneCount, "");
		else
			property_get(W_SIM_NUM, phoneCount, "");
		if(!strcmp(phoneCount, "2"))
			system(mux_2sim_swap);
		else if(!strcmp(phoneCount, "3"))
			system(mux_3sim_swap);
	} else {
		if(modem == TD_MODEM) {
			property_get(TD_TTY_DEV_PRO, modem_dev, "");
			property_get(TD_SIM_NUM, phoneCount, "");
		} else {
			property_get(W_TTY_DEV_PRO, modem_dev, "");
			property_get(W_SIM_NUM, phoneCount, "");
		}
		sprintf(path, "%s0", modem_dev);
		MODEMD_LOGD("open stty dev: %s", path);
		stty_fd = open(path, O_RDWR);
		if (stty_fd < 0) {
			MODEMD_LOGE("Failed to open %s!\n", path);
			return -1;
		}
		if(!strcmp(phoneCount, "2"))
			strcpy(at_str, "AT+SMMSWAP=0\r");
		else if(!strcmp(phoneCount, "3"))
			strcpy(at_str, "AT+SMMSWAP=1\r");
		else
			strcpy(at_str, "AT\r");
rewrite:
		MODEMD_LOGD("write %s to %s", at_str, path);
		length = strlen(at_str);
		ret = write(stty_fd, at_str, length);
		if (ret != length) {
			MODEMD_LOGE("write error length = %d  ret = %d\n", length, ret);
			close(stty_fd);
			exit(-1);
		}
		while(1) {
			ret = read(stty_fd, p_buf, sizeof(buffer) - (p_buf - buffer));
			MODEMD_LOGD("ret = %d, buffer = %s", ret, buffer);
			if(ret > 0) {
				p_buf += ret;
				if (findInBuf(buffer, sizeof(buffer), "OK")) {
					MODEMD_LOGD("read OK from %s", path);
					resend++;
					break;
				} else if (findInBuf(buffer, sizeof(buffer), "ERROR")) {
					MODEMD_LOGD("wrong modem state, exit!");
					close(stty_fd);
					exit(-1);
				}
			} else {
				ret = write(stty_fd, at_str, length);
				if (ret != length) {
					MODEMD_LOGE("rewrite error length = %d  ret = %d\n", length, ret);
					close(stty_fd);
					exit(-1);
				}
				count++;
			}
			if(count > 5) {
				MODEMD_LOGE("write 5 times, modem no response, exit!");
				close(stty_fd);
				exit(-1);
			}
		}
		if(resend == 1) {
			memset(buffer, 0, sizeof(buffer));
			p_buf = buffer;
			strcpy(at_str, "AT+CMUX=0\r");
			goto rewrite;
		}
		close(stty_fd);
	}

	/*start phoneserver*/
	start_phser(modem);

	/*start eng*/
	start_engservice(modem);

	if(restart == 1) {
		MODEMD_LOGD("restart rild!");
		start_rild(modem);

		MODEMD_LOGD("restart mediaserver!");
		property_set("ctl.restart", "media");
	} else {
		MODEMD_LOGD("start rild!");
		start_rild(modem);
	}

	return 0;
}

static void *modemd_listenaccept_thread(void *par)
{
	int sfd, n, i;

	for(i=0; i<MAX_CLIENT_NUM; i++)
		client_fd[i]=-1;

	sfd = socket_local_server(MODEM_SOCKET_NAME,
			ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	if (sfd < 0) {
		MODEMD_LOGE("%s: cannot create local socket server", __FUNCTION__);
		exit(-1);
	}

	for(; ;){

		MODEMD_LOGD("%s: Waiting for new connect ...", __FUNCTION__);
		if ( (n=accept(sfd,NULL,NULL)) == -1)
		{
			MODEMD_LOGE("engserver accept error\n");
			continue;
		}

		MODEMD_LOGD("%s: accept client n=%d",__FUNCTION__, n);
		for(i=0; i<MAX_CLIENT_NUM; i++) {
			if(client_fd[i]==-1){
				client_fd[i]=n;
				MODEMD_LOGD("%s: fill %d to client[%d]\n",__FUNCTION__, n, i);
				break;
			}
			/* if client_fd arrray is full, just fill the new socket to the
			 * last element */
			if(i == MAX_CLIENT_NUM - 1) {
				MODEMD_LOGD("%s: client array is full, just fill %d to client[%d]",
						__FUNCTION__, n, i);
				client_fd[i]=n;
			}
		}
	}
}

static void start_modem(int *para)
{
	pthread_t tid1, tid2;
	int modem = -1;
	char prop[30];
	char modem_enable[5];
        char modem_dev[20];

	if(para != NULL)
		modem = *para;

	if(modem == TD_MODEM) {
		strcpy(prop, TD_MODEM_ENABLE);
	} else if(modem == W_MODEM) {
		strcpy(prop, W_MODEM_ENABLE);
	} else {
		MODEMD_LOGE("Invalid modem type");
		return;
	}

	property_get(prop, modem_enable, "");
	if(!strcmp(modem_enable, "1")) {
		MODEMD_LOGD("%s modem is enabled", modem == TD_MODEM?"TD":"W");

		if(modem == TD_MODEM)
			strcpy(prop, TD_PROC_PRO);
		else if(modem == W_MODEM)
			strcpy(prop, W_PROC_PRO);

		property_get(prop, modem_dev, "");
		if(!strcmp(modem_dev, TD_PROC_DEV)) {
			/*  sipc td modem */
			MODEMD_LOGD("It's td native version");
			start_service(modem, 0, 0);
			pthread_create(&tid1, NULL, (void*)detect_sipc_modem, (void *)para);
		} else if(!strcmp(modem_dev, W_PROC_DEV)) {
			/*  sipc w modem */
			MODEMD_LOGD("It's w native version");
			start_service(modem, 0, 0);
			pthread_create(&tid2, NULL, (void*)detect_sipc_modem, (void *)para);
		} else {
			/*  vlx version, only one modem */
			MODEMD_LOGD("It's vlx version");
			vlx_reboot_init();
			start_service(modem, 1, 0);
			detect_vlx_modem(modem);
		}
	} else {
		MODEMD_LOGD("%s modem is not enabled", modem == TD_MODEM?"TD":"W");
	}
}

int main(int argc, char *argv[])
{
	pthread_t tid;
	struct sigaction action;
	int ret;
	int modem_td = TD_MODEM;
	int modem_w = W_MODEM;

	memset(&action, 0x00, sizeof(action));
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	ret = sigaction (SIGPIPE, &action, NULL);
	if (ret < 0) {
		MODEMD_LOGE("sigaction() failed!\n");
		exit(1);
	}

	pthread_create(&tid, NULL, (void*)modemd_listenaccept_thread, NULL);

	/* for vlx version, there is only one modem, once one of the follow two functions
	*   matched, it will execute forever and never retrun to check the next modem
	*/

	/* start td modem*/
	start_modem(&modem_td);

	/* start w modem*/
	start_modem(&modem_w);

	do {
		pause();
	} while(1);

	return 0;
}
