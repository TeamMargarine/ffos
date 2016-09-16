#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "engopt.h"
#include "eng_audio.h"
#include "eng_diag.h"
#include <ctype.h>
#include "cutils/properties.h"
#include "private/android_filesystem_config.h"

#define DATA_BUF_SIZE (4096*2)
static char log_data[DATA_BUF_SIZE];
#define DATA_EXT_DIAG_SIZE (4096*2)
static char ext_data_buf[DATA_EXT_DIAG_SIZE];
static int ext_buf_len;
//referrenced by eng_diag.c
int audio_fd;
AUDIO_TOTAL_T audio_total[4];


static void print_log_data(int cnt)
{
	int i;

	if (cnt > DATA_BUF_SIZE/2)
		cnt = DATA_BUF_SIZE/2;

	ENG_LOG("vser receive:\n");
	for(i = 0; i < cnt; i++) {
		if (isalnum(log_data[i])){
			ENG_LOG("%c ", log_data[i]);
		}else{
			ENG_LOG("%2x ", log_data[i]);
		}
	}
	ENG_LOG("\n");
}

void init_user_diag_buf(void)
{
	memset(ext_data_buf,0,DATA_EXT_DIAG_SIZE);
	ext_buf_len = 0;	
}

int get_user_diag_buf(char* buf,int len)
{
	int i;
	int is_find = 0;
	for(i=0;i<len;i++){
		ENG_LOG("%s: %x\n",__FUNCTION__, buf[i]);
		
		if (buf[i] == 0x7e && ext_buf_len ==0){ //start
			ext_data_buf[ext_buf_len]=buf[i];
			ext_buf_len ++;
		}
		else if (ext_buf_len >0 && ext_buf_len < DATA_EXT_DIAG_SIZE){
			ext_data_buf[ext_buf_len]=buf[i];
			if ( buf[i] == 0x7e ){
				is_find = 1;
				break;
			}
			else {
				ext_buf_len ++;
			}
		}
	}

	if ( is_find ) {
		for(i=0; i<ext_buf_len; i++) {
			ENG_LOG("0x%x, ",ext_data_buf[i]);
		}
	}
	return is_find;
}


int check_audio_para_file_size(char *config_file)
{
    int fileSize = 0;
    int tmpFd;

    ENG_LOG("%s: enter",__FUNCTION__);
    tmpFd = open(config_file, O_RDONLY);
    if (tmpFd < 0) {
        ENG_LOG("%s: open error",__FUNCTION__);
        return -1;
    }
    fileSize = lseek(tmpFd, 0, SEEK_END);
    if (fileSize <= 0) {
        ENG_LOG("%s: file size error",__FUNCTION__);
        return -1;
    }
    close(tmpFd);
    ENG_LOG("%s: check OK",__FUNCTION__);
    return 0;
}

int ensure_audio_para_file_exists(char *config_file)
{
    char buf[2048];
    int srcfd, destfd;
    struct stat sb;
    int nread;
    int ret;

    ENG_LOG("%s: enter",__FUNCTION__);
    ret = access(config_file, R_OK|W_OK);
    if ((ret == 0) || (errno == EACCES)) {
        if ((ret != 0) &&
            (chmod(config_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)) {
            ALOGE("Cannot set RW to \"%s\": %s", config_file, strerror(errno));
            return -1;
        }
        if (0 == check_audio_para_file_size(config_file)) {
            ENG_LOG("%s: ensure OK",__FUNCTION__);
            return 0;
        }
    } else if (errno != ENOENT) {
        ALOGE("Cannot access \"%s\": %s", config_file, strerror(errno));
        return -1;
    }

    srcfd = open((char *)(ENG_AUDIO_PARA), O_RDONLY);
    if (srcfd < 0) {
        ALOGE("Cannot open \"%s\": %s", (char *)(ENG_AUDIO_PARA), strerror(errno));
        return -1;
    }

    destfd = open(config_file, O_CREAT|O_RDWR, 0660);
    if (destfd < 0) {
        close(srcfd);
        ALOGE("Cannot create \"%s\": %s", config_file, strerror(errno));
        return -1;
    }

    ENG_LOG("%s: start copy",__FUNCTION__);
    while ((nread = read(srcfd, buf, sizeof(buf))) != 0) {
        if (nread < 0) {
            ALOGE("Error reading \"%s\": %s",(char *)(ENG_AUDIO_PARA) , strerror(errno));
            close(srcfd);
            close(destfd);
            unlink(config_file);
            return -1;
        }
        write(destfd, buf, nread);
    }

    close(destfd);
    close(srcfd);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(config_file, 0660) < 0) {
        ALOGE("Error changing permissions of %s to 0660: %s",
             config_file, strerror(errno));
        unlink(config_file);
        return -1;
    }

    if (chown(config_file, AID_SYSTEM, AID_SYSTEM) < 0) {
        ALOGE("Error changing group ownership of %s to %d: %s",
             config_file, AID_SYSTEM, strerror(errno));
        unlink(config_file);
        return -1;
    }
    ENG_LOG("%s: ensure done",__FUNCTION__);
    return 0;
}


#define MAX_OPEN_TIMES  10	
//int main(int argc, char **argv)
void *eng_vdiag_thread(void *x)
{
	int pipe_fd;
	int ser_fd;
	ssize_t r_cnt, w_cnt;
	int res, ret=0;
    int wait_cnt = 0;

	ser_fd = open("/dev/vser",O_RDONLY);
	if(ser_fd < 0) {
		ENG_LOG("cannot open general serial\n");
//		exit(1);
        return NULL;
	}
    do
    {
	    pipe_fd = open("/dev/vbpipe0",O_WRONLY);
	    if(pipe_fd < 0) {
		    ENG_LOG("vdiag cannot open vpipe0, times:%d\n", wait_cnt);
            if(wait_cnt++ >= MAX_OPEN_TIMES)
            {
                return NULL;
            }
            sleep(5);
	    }
    }while(pipe_fd < 0);
	
	memset(&audio_total,0,sizeof(audio_total));
	if(0 == ensure_audio_para_file_exists((char *)(ENG_AUDIO_PARA_DEBUG))){
		audio_fd = open(ENG_AUDIO_PARA_DEBUG,O_RDWR);
	}
	if(audio_fd>0)read(audio_fd,&audio_total,sizeof(audio_total));

	ENG_LOG("put diag data from serial to pipe\n");

	init_user_diag_buf();
	
	while(1) {
        ENG_LOG("DIAGLOG\n");
		r_cnt = read(ser_fd, log_data, DATA_BUF_SIZE/2);
        ENG_LOG("DIAGLOG::1r_cnt=%d\n", r_cnt);
        if (r_cnt == DATA_BUF_SIZE/2) {
    		r_cnt += read(ser_fd, log_data+r_cnt, DATA_BUF_SIZE/2);
            ENG_LOG("DIAGLOG::r_cnt=%d\n", r_cnt);
        }
		if (r_cnt < 0) {
			ENG_LOG("no log data from serial:%d %s\n", r_cnt,
					strerror(errno));
			ENG_LOG("close serial\n");
			close(ser_fd);

			ENG_LOG("reopen serial\n");
			ser_fd = open("/dev/vser",O_RDONLY);
			if(ser_fd < 0) {
				ENG_LOG("cannot open vendor serial\n");
//				exit(1);
                return NULL;
			}

		}
		ret=0;
		
		if (get_user_diag_buf(log_data,r_cnt)){
			ret=eng_diag(ext_data_buf,ext_buf_len);
			init_user_diag_buf();
		}

		if(ret == 1)
			continue;
		
		ENG_LOG("read from diag %d\n", r_cnt);
		//print_log_data(r_cnt);
		w_cnt = write(pipe_fd, log_data, r_cnt);
		if (w_cnt < 0) {
			ENG_LOG("no log data write:%d ,%s\n", w_cnt,
					strerror(errno));
			continue;
		}
		ENG_LOG("read from diag %d, write to pipe%d\n", r_cnt, w_cnt);
	}
out:
	close(audio_fd);
	close(pipe_fd);
	close(ser_fd);
	return 0;
}
