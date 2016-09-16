/*
 * =====================================================================================
 *
 *       Filename:  eng_wifi_ptest.c
 *
 *    Description:  Csr wifi production test in engineering mode
 *
 *        Version:  1.0
 *        Created:  10/02/2011 03:25:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Binary Yang <Binary.Yang@spreadtrum.com.cn>
 *        Company:  © Copyright 2010 Spreadtrum Communications Inc.
 *
 * =====================================================================================
 */


#include	<stdint.h>
#include	<stddef.h>
#include	<stdarg.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/socket.h>
#include	<sys/un.h>
#include	<fcntl.h>
#include	<string.h>
#include	<pthread.h>
#include	<errno.h>
#include	"eng_wifi_ptest.h"
#include	"engopt.h"




#define	        WIFI_PTEST_PIPE          "/etc/wifi_ptest"			/* the pipe used to send cmd to synergy ptest */
#define OPEN_WIFI   1
#define CLOSE_WIFI  0
#define OPEN_BT   1
#define CLOSE_BT  0

static PTEST_RES_T wifi_cfm;
static int cli_fd;
static int is_opened = 0;


void set_value(int val){
	is_opened = val;
	close(cli_fd);
}
/*
int open_channel(void){
	//if channel is unix socket
	int i,ret;
	struct sockaddr_un addr;

	ENG_LOG("===========%s",__func__);
	cli_fd = socket(AF_UNIX,SOCK_STREAM,0);
	if ( -1==cli_fd ) {
		ENG_LOG("fail to create socket");
		return -1;
	}

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path,WIFI_PTEST_PIPE);
	
	//try 20 times
	i = 20;	
	while ( i>0 ) {
		i--;
		ret  = connect(cli_fd,(struct sockaddr *)&addr,sizeof(addr));
		if ( 0!=ret ) {
			sleep(1);
			continue;
		}
		return 0;
	}

	ENG_LOG("CAN'T CONNECT TO SERVER");
	return -1;

}*/

//for wifi eut test
PTEST_RES_T *wifi_ptest_init(PTEST_CMD_T *ptest){
	int res = 0;
	char req[32] = {0}; 

	ENG_LOG("===========%s",__func__);
	memset(req, 0, sizeof(req));
	memset(&wifi_cfm, 0, sizeof(PTEST_RES_T));
	wifieut(OPEN_WIFI,req);
	if (!strcmp(req,EUT_WIFI_OK)) {
		wifi_cfm.result = 0;	
		return &wifi_cfm;
	}

/*
	if(!is_opened){
		res = open_channel();
		if(-1==res){
			return NULL;
		}
		is_opened = 1;
	}
	write(cli_fd,ptest,sizeof(PTEST_CMD_T));
	res = read(cli_fd,&wifi_cfm,sizeof(PTEST_RES_T));
	if ( res > 0 ) {
		return &wifi_cfm;
	}
*/
	ENG_LOG("===========%s failed",__func__);
	return NULL;
}

PTEST_RES_T *wifi_ptest_cw(PTEST_CMD_T *ptest){
	int res = 0;
	int channel = 0;
	char cmd[16] = {0};
 
	ENG_LOG("===========%s",__func__);
	memset(cmd, 0, sizeof(cmd));
	memset(&wifi_cfm, 0, sizeof(PTEST_RES_T));

	channel = ptest->channel;
	sprintf(cmd,"wl crusprs %d", channel); 
	system(cmd);
	wifi_cfm.result = 0;	
	return &wifi_cfm;
   
/*
	write(cli_fd,ptest,sizeof(PTEST_CMD_T));
	res = read(cli_fd,&wifi_cfm,sizeof(PTEST_RES_T));
	if ( res > 0 ) {
		return &wifi_cfm;
	}
*/
	ENG_LOG("===========%s failed",__func__);
	return NULL;
}

PTEST_RES_T *wifi_ptest_tx(PTEST_CMD_T *ptest){
	int res = 0;
	int channel = 0;
	int rate = 0;
	int powerLevel = 0;
	char req[32] = {0}; 

	ENG_LOG("===========%s",__func__);
	channel = ptest->channel;
	rate = ptest->ptest_tx.rate;
	powerLevel = ptest->ptest_tx.powerLevel;

	set_wifi_ratio(rate,req);
	set_wifi_ch(channel,req);
	set_wifi_tx_factor_83780(powerLevel,req);  

	memset(req, 0, sizeof(req));
	memset(&wifi_cfm, 0, sizeof(PTEST_RES_T));
	wifi_tx(OPEN_WIFI,req);
	if (!strcmp(req,EUT_WIFI_OK)) {
		wifi_cfm.result = 0;	
		return &wifi_cfm;
	}
/*	
	write(cli_fd,ptest,sizeof(PTEST_CMD_T));
	res = read(cli_fd,&wifi_cfm,sizeof(PTEST_RES_T));
	if ( res > 0 ) {
		return &wifi_cfm;
	}
*/
	ENG_LOG("===========%s failed",__func__);
	return NULL;
}

PTEST_RES_T *wifi_ptest_rx(PTEST_CMD_T *ptest){
	int res = 0;
	int channel = 0;
	char req[32] = {0};

	ENG_LOG("===========%s",__func__);
	channel = ptest->channel;

	set_wifi_ch(channel,req);
	memset(req, 0, sizeof(req));
	memset(&wifi_cfm, 0, sizeof(PTEST_RES_T));
	wifi_rx(OPEN_WIFI,req);
	if (!strcmp(req,EUT_WIFI_OK)) {
		wifi_cfm.result = 0;	
		return &wifi_cfm;
	}

/*	
	write(cli_fd,ptest,sizeof(PTEST_CMD_T));
	res = read(cli_fd,&wifi_cfm,sizeof(PTEST_RES_T));
	if ( res > 0 ) {
		return &wifi_cfm;
	}
*/
	ENG_LOG("===========%s failed",__func__);
	return NULL;
}

PTEST_RES_T *wifi_ptest_deinit(PTEST_CMD_T *ptest){
	int res = 0;
	char req[32] = {0}; 

	ENG_LOG("===========%s",__func__);
	memset(req, 0, sizeof(req));
	memset(&wifi_cfm, 0, sizeof(PTEST_RES_T));
	wifieut(CLOSE_WIFI,req);
	if (!strcmp(req,EUT_WIFI_OK)) {
		wifi_cfm.result = 0;	
		return &wifi_cfm;
	}

/*	
	write(cli_fd,ptest,sizeof(PTEST_CMD_T));
	res = read(cli_fd,&wifi_cfm,sizeof(PTEST_RES_T));
	if ( res > 0 ) {
		return &wifi_cfm;
	}
*/
	ENG_LOG("===========%s failed",__func__);
	return NULL;
}

//for  bt eut test
int bt_ptest_start(void){
	int res = 0;
	char req[32] = {0}; 

	ENG_LOG("===========%s",__func__);
	memset(req, 0, sizeof(req));
	memset(&wifi_cfm, 0, sizeof(PTEST_RES_T));
	bteut(OPEN_BT, req);
	if (!strcmp(req,EUT_BT_OK)) {
		return 0;
	}

	ENG_LOG("===========%s failed",__func__);
	return 1;
}

int bt_ptest_stop(void){
	int res = 0;
	char req[32] = {0}; 

	ENG_LOG("===========%s",__func__);
	memset(req, 0, sizeof(req));
	memset(&wifi_cfm, 0, sizeof(PTEST_RES_T));
	bteut(CLOSE_BT, req);
	if (!strcmp(req,EUT_BT_OK)) {
		return 0;
	}

	ENG_LOG("===========%s failed",__func__);
	return 1;
}
