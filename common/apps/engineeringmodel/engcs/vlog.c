#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "engopt.h"
#include "cutils/properties.h"

#define ENG_CARDLOG_PROPERTY	"persist.sys.cardlog"
#define DATA_BUF_SIZE (4096 * 4)
static char log_data[DATA_BUF_SIZE];
static int vser_fd = 0;

int is_sdcard_exist=1;
int pipe_fd;

int get_vser_fd(void)
{
	return vser_fd;
}

int restart_vser(void)
{
	ENG_LOG("close usb serial:%d\n", vser_fd);
	close(vser_fd);

	vser_fd = open("/dev/vser",O_WRONLY);
	if(vser_fd < 0) {
		ENG_LOG("cannot open general serial\n");
//		exit(1);
        return NULL;
	}
	ENG_LOG("reopen usb serial:%d\n", vser_fd);
    return 0;
}

#define MAX_OPEN_TIMES  10
//int main(int argc, char **argv)
void *eng_vlog_thread(void *x)
{
	int ser_fd;
	int sdcard_fd;
	int card_log_fd = -1;
	ssize_t r_cnt, w_cnt;
	int res,n;
	char cardlog_property[8];
	char log_name[40];
	time_t now;
    int wait_cnt = 0;
	struct tm *timenow;

	ENG_LOG("open usb serial\n");
	ser_fd = open("/dev/vser",O_WRONLY);
	if(ser_fd < 0) {
		ALOGE("cannot open general serial:%s\n",strerror(errno));
//		exit(1);
        return NULL;
	}
	vser_fd = ser_fd;
	
	ENG_LOG("open vitual pipe\n");
    do
    {
	    pipe_fd = open("/dev/vbpipe0",O_RDONLY);
	    if(pipe_fd < 0) {
		    ALOGE("vlog cannot open vpipe0, times:%d\n", wait_cnt);
            if(wait_cnt++ >= MAX_OPEN_TIMES)
            {
                return NULL;
            }
            sleep(5);
	    }
    }while(pipe_fd < 0);

	sdcard_fd = open("/dev/block/mmcblk0",O_RDONLY);
	if ( sdcard_fd < 0 ) {
		is_sdcard_exist = 0;
		ENG_LOG("No sd card!!!");
	}else{
		is_sdcard_exist = 1;
	}
	close(sdcard_fd);

	ENG_LOG("put log data from pipe to serial\n");
	while(1) {

		if ( is_sdcard_exist ) {
			memset(cardlog_property, 0, sizeof(cardlog_property));
			property_get(ENG_CARDLOG_PROPERTY, cardlog_property, "");
			n = atoi(cardlog_property);

			if ( 1==n ){
				sleep(1);
			       	continue;
			}
		}

		r_cnt = read(pipe_fd, log_data, DATA_BUF_SIZE);
		if (r_cnt < 0) {
			ENG_LOG("no log data :%d\n", r_cnt);
			continue;
		}

		
		ENG_LOG("read %d\n", r_cnt);
		w_cnt = write(ser_fd, log_data, r_cnt);
		if (w_cnt < 0) {
			ENG_LOG("no log data write:%d ,%s\n", w_cnt,
					strerror(errno));


			ENG_LOG("close usb serial:%d\n", ser_fd);
			close(ser_fd);

			ser_fd = open("/dev/vser",O_WRONLY);
			if(ser_fd < 0) {
				ALOGE("cannot open general serial\n");
//				exit(1);
                return NULL;
			}
			vser_fd = ser_fd;
			ENG_LOG("reopen usb serial:%d\n", ser_fd);
		}
		ENG_LOG("read %d, write %d\n", r_cnt, w_cnt);
	}
out:
	close(pipe_fd);
	close(ser_fd);
	return 0;
}



