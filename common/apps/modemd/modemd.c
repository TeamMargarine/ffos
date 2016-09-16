#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <pthread.h>
#include <utils/Log.h>
#include <signal.h>

#define LOG_TAG 	"MODEMD"
#define MODEM_SOCKET_NAME	"modemd"
#define MAX_CLIENT_NUM	10
#define MODEM_PATH	"/dev/vbpipe2"

#define DATA_BUF_SIZE (128)
static char log_data[DATA_BUF_SIZE];
static int  client_fd[MAX_CLIENT_NUM];

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

static void *modemd_listenaccept_thread(void *par)
{
	int sfd, n, i;

	for(i=0; i<MAX_CLIENT_NUM; i++)
		client_fd[i]=-1;

	sfd = socket_local_server(MODEM_SOCKET_NAME,
		ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	if (sfd < 0) {
		ALOGE("%s: cannot create local socket server", __FUNCTION__);
		exit(-1);
	}

	for(; ;){

		ALOGE("%s: Waiting for new connect ...", __FUNCTION__);
		if ( (n=accept(sfd,NULL,NULL)) == -1)
		{
			ALOGE("engserver accept error\n");
			continue;
		}

		ALOGE("%s: accept client n=%d",__FUNCTION__, n);
		for(i=0; i<MAX_CLIENT_NUM; i++) {
			if(client_fd[i]==-1){
				client_fd[i]=n;
				ALOGE("%s: fill %d to client[%d]\n",__FUNCTION__, n, i);
				break;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int cfd, ret,modemfd, i;
	ssize_t numRead;
	char buf[DATA_BUF_SIZE];
	pthread_t t1;
	struct sigaction action;

	memset(&action, 0x00, sizeof(action));
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	ret = sigaction (SIGPIPE, &action, NULL);
	if (ret < 0) {
		ALOGE("sigaction() failed!\n");
		exit(1);
	}

	modemfd = open(MODEM_PATH, O_RDONLY);

	if (modemfd < 0){
		ALOGE("cannot open vbpipe for check modem state\n");
		exit(-1);
	}

	pthread_create(&t1, NULL, (void*)modemd_listenaccept_thread, NULL);

	for(;;) {
		numRead = read(modemfd, buf, DATA_BUF_SIZE);
		if (numRead == 0){
			ALOGE("read nothing from modem state\n");
			/* close and reopen */
			close(modemfd);
			sleep(10);
			modemfd = open(MODEM_PATH, O_RDONLY);
			if (modemfd < 0){
				ALOGE("cannot open modem state pipe\n");
				exit(-1);
			}
			continue;
		} else if (numRead < 0) {
			ALOGE("numRead %d is lower than 0\n", numRead);
                        continue;
		}
		ALOGE("modem assert happen\n");
		print_log_data(buf, numRead);

		for(i=0; i<MAX_CLIENT_NUM; i++) {
			ALOGE("client_fd[%d]=%d\n",i, client_fd[i]);
			if(client_fd[i] > 0) {
				ret = write(client_fd[i], buf, numRead);
				ALOGE("write %d bytes to client socket [%d] to reset modem\n", ret, client_fd[i]);
				close(client_fd[i]);
				client_fd[i]=-1;
			}
		}
		/*
		while ((numRead = read(cfd, buf, BUF_SIZE)) > 0){
			write(STDOUT_FILENO, "MODEM: ", 6);
			if (write(STDOUT_FILENO, buf, numRead) != numRead)
				printf("partial/failed write\n");
		}
		if (numRead == -1){
			printf("read error\n");
			exit(-1);
		}
		*/
	}

	ALOGE("MODEM close client socket\n");
	close(cfd);
	return 0;
}
