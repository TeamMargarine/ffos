#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/stat.h>

#include <utils/Log.h>
#include <hardware_legacy/power.h>

#include "engopt.h"
#include "engat.h"
#include "eng_cmd4linuxhdlr.h"
#include "eng_sqlite.h"
#include "eng_diag.h"
#include "eng_pcclient.h"
#include "eng_appclient.h"
#include "eng_attok.h"


#define NUM_ELEMS(x) (sizeof(x)/sizeof(x[0]))

#ifdef sp6810a
#define ENG_KEYPAD_PATH	"/sys/devices/platform/sprd-keypad6810/emulate"
#elif  sp8805ga
#define ENG_KEYPAD_PATH	"/sys/devices/platform/sprd-keypad8805ga/emulate"
#elif  sprdop
#define ENG_KEYPAD_PATH	"/sys/devices/platform/sprd-keypad/emulate"
#else
#define ENG_KEYPAD_PATH "/sys/devices/platform/sprd-keypad/emulate"
#endif
extern int eng_atdiag_hdlr(unsigned char *buf,int len, char* rsp);
extern int eng_atdiag_euthdlr(unsigned char *buf,int len,char* rsp,int module_index);
extern void eng_check_factorymode_fornand(void);
extern void eng_check_factorymode_formmc(void);
extern int turnoff_lcd_backlight(void);
static unsigned char g_buffer[ENG_BUFFER_SIZE];
static int eng_linuxcmd_rpoweron(char *req, char *rsp);
static int eng_linuxcmd_keypad(char *req, char *rsp);
static int eng_linuxcmd_vbat(char *req, char *rsp);
static int eng_linuxcmd_stopchg(char *req, char *rsp);
static int eng_linuxcmd_factoryreset(char *req, char *rsp);
static int eng_linuxcmd_getfactoryreset(char *req, char *rsp);
static int eng_linuxcmd_mmitest(char *req, char *rsp);
static int eng_linuxcmd_bttest(char *req, char *rsp);
static int eng_linuxcmd_getbtaddr(char *req, char *rsp);
static int eng_linuxcmd_setbtaddr(char *req, char *rsp);
static int eng_linuxcmd_gsnr(char *req, char *rsp);
static int eng_linuxcmd_gsnw(char *req, char *rsp);
static int eng_linuxcmd_getwifiaddr(char *req, char *rsp);
static int eng_linuxcmd_setwifiaddr(char *req, char *rsp);
static int eng_linuxcmd_writeimei(char *req, char *rsp);
static int eng_linuxcmd_getich(char *req, char *rsp);
static int eng_linuxcmd_simchk(char *req, char *rsp);
static int eng_linuxcmd_atdiag(char *req, char *rsp);
static int eng_linuxcmd_infactorymode(char *req, char *rsp);
static int eng_linuxcmd_fastdeepsleep(char *req, char *rsp);
static int eng_linuxcmd_chargertest(char *req, char *rsp);
static int eng_linuxcmd_bteutmode(char *req,char *rsp);
static int eng_linuxcmd_wifieutmode(char *req,char *rsp);
static int eng_linuxcmd_gpseutmode(char *req,char *rsp);


static struct eng_linuxcmd_str eng_linuxcmd[] = {
    {CMD_SENDKEY,        CMD_TO_AP,   	"AT+SENDKEY",		eng_linuxcmd_keypad},
    {CMD_GETICH,         CMD_TO_AP,	    "AT+GETICH?",		eng_linuxcmd_getich},
    {CMD_ETSRESET,       CMD_TO_AP, 	"AT+ETSRESET",		eng_linuxcmd_factoryreset},
    {CMD_RPOWERON,       CMD_TO_AP,   	"AT+RPOWERON",		eng_linuxcmd_rpoweron},
    {CMD_GETVBAT,        CMD_TO_AP, 	"AT+GETVBAT",		eng_linuxcmd_vbat},
    {CMD_STOPCHG,        CMD_TO_AP, 	"AT+STOPCHG",		eng_linuxcmd_stopchg},
    {CMD_TESTMMI,        CMD_TO_AP, 	"AT+TESTMMI",		eng_linuxcmd_mmitest},
    {CMD_BTTESTMODE,     CMD_TO_AP,	    "AT+BTTESTMODE",	eng_linuxcmd_bttest},
    {CMD_GETBTADDR,      CMD_TO_AP, 	"AT+GETBTADDR",		eng_linuxcmd_getbtaddr},
    {CMD_SETBTADDR,      CMD_TO_AP, 	"AT+SETBTADDR",		eng_linuxcmd_setbtaddr},
    {CMD_GSNR,           CMD_TO_AP, 	"AT+GSNR",		    eng_linuxcmd_gsnr},
    {CMD_GSNW,           CMD_TO_AP, 	"AT+GSNW",	     	eng_linuxcmd_gsnw},
    {CMD_GETWIFIADDR,    CMD_TO_AP, 	"AT+GETWIFIADDR",	eng_linuxcmd_getwifiaddr},
    {CMD_SETWIFIADDR,    CMD_TO_AP, 	"AT+SETWIFIADDR",	eng_linuxcmd_setwifiaddr},
    {CMD_ETSCHECKRESET,  CMD_TO_AP,	    "AT+ETSCHECKRESET",	eng_linuxcmd_getfactoryreset},
    {CMD_SIMCHK,         CMD_TO_AP,	    "AT+SIMCHK",		eng_linuxcmd_simchk},
    {CMD_INFACTORYMODE,  CMD_TO_AP,	    "AT+FACTORYMODE",	eng_linuxcmd_infactorymode},
    {CMD_FASTDEEPSLEEP,  CMD_TO_APCP,	"AT+SYSSLEEP",	eng_linuxcmd_fastdeepsleep},
    {CMD_CHARGERTEST,    CMD_TO_AP,	    "AT+CHARGERTEST",	eng_linuxcmd_chargertest},
    {CMD_SPBTTEST,       CMD_TO_AP,	    "AT+SPBTTEST",		eng_linuxcmd_bteutmode},
    {CMD_SPWIFITEST,     CMD_TO_AP,	    "AT+SPWIFITEST",	eng_linuxcmd_wifieutmode},
    {CMD_SPGPSTEST,      CMD_TO_AP,	    "AT+SPGPSTEST",		eng_linuxcmd_gpseutmode},
    {CMD_ATDIAG,         CMD_TO_AP,	    "+SPBTWIFICALI",	eng_linuxcmd_atdiag},

};

/** returns 1 if line starts with prefix, 0 if it does not */
static int eng_cmdstartwith(const char *line, const char *prefix)
{
    for ( ; *line != '\0' && *prefix != '\0' ; line++, prefix++) {
        if (*line != *prefix) {
            return 0;
        }
    }
    return *prefix == '\0';
}

int eng_at2linux(char *buf)
{
    int ret=-1;
	int i;

    for (i = 0 ; i < (int)NUM_ELEMS(eng_linuxcmd) ; i++) {
        if (strcasestr(buf, eng_linuxcmd[i].name)!=NULL) {
			ENG_LOG("eng_at2linux %s",eng_linuxcmd[i].name);
            return i;
        }
    }

    return ret;
}

eng_cmd_type eng_cmd_get_type(int cmd)
{
	if (cmd < NUM_ELEMS(eng_linuxcmd))
		return eng_linuxcmd[cmd].type;
	else
		return CMD_INVALID_TYPE;
}

int eng_linuxcmd_hdlr(int cmd, char *req, char* rsp)
{
	if(cmd >= (int)NUM_ELEMS(eng_linuxcmd)) {
		ENG_LOG("%s: no handler for cmd%d",__FUNCTION__, cmd);
		return -1;
	}
	return eng_linuxcmd[cmd].cmd_hdlr(req, rsp);
}

int eng_linuxcmd_rpoweron(char *req, char *rsp)
{
	sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
    sync();
	__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                 LINUX_REBOOT_CMD_RESTART2, "fastsleep");

	return 0;
}

static int eng_get_device_from_path(const char *path, char *device_name)
{
	char device[256];
	char mount_path[256];
	char rest[256];
	FILE *fp;
	char line[1024];

	if (!(fp = fopen("/proc/mounts", "r"))) {
		SLOGE("Error opening /proc/mounts (%s)", strerror(errno));
		return 0;
	}

	while(fgets(line, sizeof(line), fp)) {
		line[strlen(line)-1] = '\0';
		sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
		if (!strcmp(mount_path, path)) {
			strcpy(device_name,device);
			fclose(fp);
			return 1;
		}

	}

	fclose(fp);
	return 0;
}

int eng_linuxcmd_factoryreset(char *req, char *rsp)
{	
	int ret = 1;
	char cmd[]="--wipe_data";
	int fd;
	char device_name[256];
	char convert_name[256];
	char format_cmd[1024];
	const char* externalStorage = getenv("SECONDARY_STORAGE");
	static char MKDOSFS_PATH[] = "/system/bin/newfs_msdos";

	mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

	ENG_LOG("Call %s\n",__FUNCTION__);

#ifdef CONFIG_EMMC
	/*format internal sd card. code from vold*/
	memset(device_name,0,256);
	if (eng_get_device_from_path(externalStorage,device_name)){
		memset(convert_name,0,256);
		sprintf(convert_name,"/dev/block/mmcblk0p%d",atoi(strchr(device_name,':')+1));
		memset(format_cmd,0,1024);
		sprintf(format_cmd,"%s -F 32 -O android %s",MKDOSFS_PATH,convert_name);
		system(format_cmd);
	} else {
		LOGE("do not format /mnt/internal");
	}
#endif
	//delete files in ENG_RECOVERYDIR
	system("rm -r /cache/recovery");

	//mkdir ENG_RECOVERYDIR
	if(mkdir(ENG_RECOVERYDIR, mode) == -1) {
		ret = 0;
		ENG_LOG("%s: mkdir %s fail [%s]\n",__FUNCTION__, ENG_RECOVERYDIR, strerror(errno));
		goto out;
	}

	//create ENG_RECOVERYCMD
	fd = open(ENG_RECOVERYCMD, O_CREAT|O_RDWR, 0666);
	
	if(fd < 0){
		ret = 0;
		ENG_LOG("%s: open %s fail [%s]\n",__FUNCTION__, ENG_RECOVERYCMD, strerror(errno));
		goto out;
	}
	ret = write(fd, cmd, strlen(cmd));
	if(ret < 0) {
		ret = 0;
                close(fd);
		ENG_LOG("%s: write %s fail [%s]\n",__FUNCTION__, ENG_RECOVERYCMD, strerror(errno));

		goto out;
	}
	if (eng_sql_string2string_set("factoryrst", "DONE")==-1) {
		ret = 0;
                close(fd);
		ENG_LOG("%s: set factoryrst fail\n",__FUNCTION__);
		goto out;
	}

	sync();
        close(fd);
	__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                 LINUX_REBOOT_CMD_RESTART2, "recovery");
out:
	if(ret==1) 
		sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
	else 
		sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
	
	return 0;
}

int eng_linuxcmd_getfactoryreset(char *req, char *rsp)
{
	sprintf(rsp, "%s%s%s%s", eng_sql_string2string_get("factoryrst"), \
		ENG_STREND, SPRDENG_OK, ENG_STREND);
	ENG_LOG("%s: rsp=%s\n",__FUNCTION__, rsp);
	
	return 0;
}


int eng_linuxcmd_keypad(char *req, char *rsp)
{
	int ret = 0, fd;
	char *keycode;
	ENG_LOG("%s: req=%s\n",__FUNCTION__, req);

	keycode = strchr(req, '=');

	if(keycode == NULL) {
		sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
		ret = -1;
	} else {
		keycode++;
		ENG_LOG("%s: keycode = %s\n",__FUNCTION__, keycode);
		fd = open(ENG_KEYPAD_PATH, O_RDWR);
		if(fd >= 0) {
			ENG_LOG("%s: send keycode to emulator\n",__FUNCTION__);
			write(fd, keycode, strlen(keycode));
		} else {
			ENG_LOG("%s: open %s fail [%s]\n",__FUNCTION__, ENG_KEYPAD_PATH, strerror(errno));
		}
		if(fd >= 0)
			close(fd);
		sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
		ret = 0;
	}
	
	return ret;
}

int eng_linuxcmd_vbat(char *req, char *rsp)
{
	int fd, ret = 1;
	int voltage;
	float vol;
	char buffer[16];

	fd = open(ENG_BATVOL, O_RDONLY);
	if(fd < 0){
		ENG_LOG("%s: open %s fail [%s]",__FUNCTION__, ENG_BATVOL, strerror(errno));
		ret = 0;
	}

	if(ret==1) {
		memset(buffer, 0, sizeof(buffer));
		read(fd, buffer, sizeof(buffer));
		voltage = atoi(buffer);
		ENG_LOG("%s: buffer=%s; voltage=%d\n",__FUNCTION__, buffer, voltage);
		vol = ((float) voltage) * 0.001;
		sprintf(rsp, "%.3g%s%s%s", vol, ENG_STREND, SPRDENG_OK, ENG_STREND);
		
	} else {
		sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
	}

	if(fd >= 0)
		close(fd);

	ENG_LOG("%s: rsp=%s\n",__FUNCTION__,rsp);
	return 0;
}

int eng_linuxcmd_stopchg(char *req, char *rsp)
{
	int fd;
	int ret = 1;
	char ok[]="1";
	fd = open(ENG_STOPCHG, O_WRONLY);

	if(fd < 0){
		ENG_LOG("%s: open %s fail [%s]",__FUNCTION__, ENG_BATVOL, strerror(errno));
		ret = 0;
	}

	if(ret == 1) {
		ret=write(fd, ok, strlen(ok));
		if (ret < 0)
			sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
		else
			sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
	} else {
		sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
	}

	if(fd >= 0)
		close(fd);
	ENG_LOG("%s: rsp=%s\n",__FUNCTION__,rsp);
	return 0;
}

int eng_linuxcmd_mmitest(char *req, char *rsp)
{
	int rc;

	rc = eng_sql_string2int_get("result");

	if(rc == ENG_SQLSTR2INT_ERR)
		sprintf(rsp, "%d%s%s%s", rc, ENG_STREND, SPRDENG_ERROR, ENG_STREND);
	else
		sprintf(rsp, "%d%s%s%s", rc, ENG_STREND, SPRDENG_OK, ENG_STREND);
	
	return 0;

}

int eng_linuxcmd_bttest(char *req, char *rsp)
{
	system("chown system system /dev/ttyS0");
	system("bccmd -t bcsp -d /dev/ttyS0 psload -r /etc/ps.psr");
	system("hciattach -s 921600 /dev/ttyS0 bcsp 921600");
	system("hciconfig hci0 up");
	system("hcitool cmd 0x03 0x0005 0x02 0x00 0x02");
	system("hcitool cmd 0x03 0x001A 0x03");
	system("hcitool cmd 0x06 0x0003");
	
	sprintf(rsp, "%s%s", SPRDENG_OK,ENG_STREND);
	return 0;
}

int eng_linuxcmd_setbtaddr(char *req, char *rsp)
{
	char address[64];
	char *ptr=NULL;
	char *endptr=NULL;
	int length;

	ENG_LOG("%s: req=%s\n",__FUNCTION__, req);

	ptr = strchr(req, '"');
	if(ptr == NULL){
		ENG_LOG("%s: req %s ERROR no start \"\n",__FUNCTION__, req);
		return -1;
	}

	ptr++;

	endptr = strchr(ptr, '"');
	if(endptr == NULL){
		ENG_LOG("%s: req %s ERROR no end \"\n",__FUNCTION__, req);
		return -1;
	}

	length = endptr - ptr + 1;

	memset(address, 0, sizeof(address));
	snprintf(address, length, "%s", ptr);
	
	ENG_LOG("%s: bt address is %s; length=%d\n",__FUNCTION__, address, length);
	
	eng_sql_string2string_set("btaddr", address);
	sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
	return 0;
}

int eng_linuxcmd_getbtaddr(char *req, char *rsp)
{
	sprintf(rsp, "%s%s%s%s", eng_sql_string2string_get("btaddr"), ENG_STREND, SPRDENG_OK, ENG_STREND);
	ENG_LOG("%s: rsp=%s\n",__FUNCTION__, rsp);
	
	return 0;
}

int eng_linuxcmd_gsnr(char *req, char *rsp)
{
	sprintf(rsp, "%s%s%s%s", eng_sql_string2string_get("gsn"), ENG_STREND, SPRDENG_OK, ENG_STREND);
	ENG_LOG("%s: rsp=%s\n",__FUNCTION__, rsp);
	return 0;
}

int eng_linuxcmd_gsnw(char *req, char *rsp)
{
	char address[64];
	char *ptr=NULL;
	char *endptr=NULL;
	int length;

	ENG_LOG("%s: req=%s\n",__FUNCTION__, req);

	ptr = strchr(req, '"');
	if(ptr == NULL){
		ENG_LOG("%s: req %s ERROR no start \"\n",__FUNCTION__, req);
		return -1;
	}

	ptr++;

	endptr = strchr(ptr, '"');
	if(endptr == NULL){
		ENG_LOG("%s: req %s ERROR no end \"\n",__FUNCTION__, req);
		return -1;
	}

	length = endptr - ptr + 1;

	memset(address, 0, sizeof(address));
	snprintf(address, length, "%s", ptr);
	
	ENG_LOG("%s: GSN is %s; length=%d\n",__FUNCTION__, address, length);
	
	eng_sql_string2string_set("gsn", address);
	sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
	return 0;
}

int eng_linuxcmd_getwifiaddr(char *req, char *rsp)
{
	sprintf(rsp, "%s%s%s%s", eng_sql_string2string_get("wifiaddr"), \
		ENG_STREND, SPRDENG_OK, ENG_STREND);
	ENG_LOG("%s: rsp=%s\n",__FUNCTION__, rsp);
	
	return 0;
}

int eng_linuxcmd_setwifiaddr(char *req, char *rsp)
{
	char address[64];
	char *ptr=NULL;
	char *endptr=NULL;
	int length;

	ENG_LOG("%s: req=%s\n",__FUNCTION__, req);

	ptr = strchr(req, '"');
	if(ptr == NULL){
		ENG_LOG("%s: req %s ERROR no start \"\n",__FUNCTION__, req);
		return -1;
	}

	ptr++;

	endptr = strchr(ptr, '"');
	if(endptr == NULL){
		ENG_LOG("%s: req %s ERROR no end \"\n",__FUNCTION__, req);
		return -1;
	}

	length = endptr - ptr + 1;

	memset(address, 0, sizeof(address));
	snprintf(address, length, "%s", ptr);
	
	ENG_LOG("%s: wifi address is %s; length=%d\n",__FUNCTION__, address, length);
	
	eng_sql_string2string_set("wifiaddr", address);
	sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
	return 0;
}

int eng_linuxcmd_writeimei(char *req, char *rsp)
{
	return eng_diag_writeimei(req,rsp);
}

int eng_linuxcmd_getich(char *req, char *rsp)
{
	int fd, ret = 1;
	int current;
	char buffer[16];

	fd = open(ENG_CURRENT, O_RDONLY);
	if(fd < 0){
		ENG_LOG("%s: open %s fail [%s]",__FUNCTION__, ENG_BATVOL, strerror(errno));
		ret = 0;
	}

	if(ret==1) {
		memset(buffer, 0, sizeof(buffer));
		read(fd, buffer, sizeof(buffer));
		current = atoi(buffer);
		ENG_LOG("%s: buffer=%s; current=%d\n",__FUNCTION__, buffer, current);
		sprintf(rsp, "%dmA%s%s%s", current, ENG_STREND, SPRDENG_OK, ENG_STREND);
		
	} else {
		ENG_LOG("%s: ERROR\n",__FUNCTION__);
		sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
	}

	if(fd >= 0)
		close(fd);

	ENG_LOG("%s: rsp=%s\n",__FUNCTION__,rsp);
	return 0;
}

static int eng_simtest_checksim_euicc(int type)
{
	int fd, ret = 0;
	char simstatus;
	char cmd[32];

	ENG_LOG("%s: type=%d\n",__FUNCTION__, type);

	fd = eng_get_csclient_fd();

	do{
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%d,%d",ENG_AT_EUICC,0);
		eng_at_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		eng_at_read(fd, cmd, sizeof(cmd));
		ENG_LOG("%s: response=%s\n", __FUNCTION__, cmd);
	}while(strstr(cmd, "err")!=NULL);

	simstatus = cmd[0];

	if(simstatus=='2') {
		ret=0;
	} else if((simstatus=='0')||(simstatus=='1')) {
		ret = 1;
	}

	return ret;
}


int eng_linuxcmd_simchk(char *req, char *rsp)
{
	int sim1=0;

	sim1 = eng_simtest_checksim_euicc(0);

	sprintf(rsp, "%d%s%s%s", sim1, ENG_STREND, SPRDENG_OK, ENG_STREND);

	return 0;
}

int eng_ascii2hex(char *inputdata, unsigned char *outputdata, int inputdatasize)
{
    int i,j,tmp_len;
    unsigned char tmpData;
    char *ascii_data = inputdata;
    unsigned char *hex_data = outputdata;
    for(i = 0; i < inputdatasize; i++){
        if ((ascii_data[i] >= '0') && (ascii_data[i] <= '9')){
            tmpData = ascii_data[i] - '0';
        }
        else if ((ascii_data[i] >= 'A') && (ascii_data[i] <= 'F')){ //A....F
            tmpData = ascii_data[i] - 'A' + 10;
        }
        else if((ascii_data[i] >= 'a') && (ascii_data[i] <= 'f')){ //a....f
            tmpData = ascii_data[i] - 'a' + 10;
        }
        else{
            break;
        }
        ascii_data[i] = tmpData;
    }

    for(tmp_len = 0,j = 0; j < i; j+=2) {
        outputdata[tmp_len++] = (ascii_data[j]<<4) | ascii_data[j+1];
    }

	ENG_LOG("%s: length=%d",__FUNCTION__, tmp_len);
	for(i=0; i<tmp_len; i++)
		ENG_LOG("%s [%d]: 0x%x",__FUNCTION__, i, hex_data[i]);

    return tmp_len;
}
int eng_linuxcmd_atdiag(char *req, char *rsp)
{
	int len, ret;
	char tmp[3];
	char *data = req;
	char *ptr = NULL;

	ENG_LOG("Call %s", __FUNCTION__);

	if (strstr(req, "DEVT"))
		ret = test_dev(req, rsp);
	else {
		memset(g_buffer, 0, sizeof(g_buffer));
		at_tok_start(&data);
		at_tok_nextstr(&data, &ptr);

		len = eng_ascii2hex(ptr, g_buffer, strlen(ptr));
		ret = eng_atdiag_hdlr(g_buffer, len, rsp);
	}
	return ret;
}

int eng_linuxcmd_bteutmode(char *req, char *rsp)
{
	int ret,len;
	ALOGI("Call %s     Command is  %s",__FUNCTION__,req);
	ret = eng_atdiag_euthdlr(req,len,rsp,BT_MODULE_INDEX);
	return ret;
}

int eng_linuxcmd_wifieutmode(char * req, char * rsp)
{
	int ret,len;
	ALOGI("Call %s     Command is  %s",__FUNCTION__,req);
	ret = eng_atdiag_euthdlr(req,len,rsp,WIFI_MODULE_INDEX);
	return ret;
}

int eng_linuxcmd_gpseutmode(char * req, char * rsp)
{
	int ret,len;
	ALOGI("Call %s     Command is  %s",__FUNCTION__,req);
	ret = eng_atdiag_euthdlr(req,len,rsp,GPS_MODULE_INDEX);
	return ret;
}

int eng_linuxcmd_infactorymode(char *req, char *rsp)
{
	char *ptr;
	int status;
	int length=strlen(req);
	ENG_LOG("%s: req=%s\n",__FUNCTION__, req);
	if((ptr=strchr(req, '?'))!= NULL){
		status = eng_sql_string2int_get(ENG_TESTMODE);
		if(status==ENG_SQLSTR2INT_ERR)
			status = 1;
		sprintf(rsp, "%d%s%s%s", status, ENG_STREND,SPRDENG_OK,ENG_STREND);
	} else if ((ptr=strchr(req, '='))!= NULL) {
		ptr++;
		if(ptr <= (req+length)) {
			status = atoi(ptr);
			ENG_LOG("%s: status=%d\n",__FUNCTION__, status);
			if(status==0||status==1) {
				eng_sql_string2int_set(ENG_TESTMODE, status);
			#ifdef CONFIG_EMMC
				eng_check_factorymode_formmc();
			#else
				eng_check_factorymode_fornand();
			#endif
				sprintf(rsp, "%s\r\n", SPRDENG_OK);
			} else {
				sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
			}
		} else {
			sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
		}
	} else {
		sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
	}
	ENG_LOG("%s: rsp=%s\n",__FUNCTION__, rsp);
	
	return 0;
}


#define DEEP_WAIT_TIME 15
void *thread_fastsleep(void *para)
{
     ALOGE("##: delay 3 seconds to wait AT command has been sent to modem...\n");
    sleep(3);
    ALOGE("##: Going to sleep mode!\n");
    turnoff_lcd_backlight();
    set_screen_state(0);
    ALOGE("##: Waiting for a while...\n");
	return NULL;
}

int eng_linuxcmd_fastdeepsleep(char *req, char *rsp)
{
    pthread_t thread_id;
    int ret;



#if 1
    ret = pthread_create(&thread_id, NULL, thread_fastsleep, NULL);
    if (0 != ret) {
        ALOGE("##: Can't create thread[thread_fastsleep]!\n");
        sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
    }
    else {
        ALOGE("##: Ccreate thread[thread_fastsleep] sucessfully!\n");
        sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
    }
#endif
    return 0;
}

int eng_linuxcmd_chargertest(char *req, char *rsp)
{
	char *ptr;
	int status, fd;
	int length=strlen(req);
	
	ENG_LOG("%s: req=%s\n",__FUNCTION__, req);
	if ((ptr=strchr(req, '='))!= NULL) {
		ptr++;
		if(ptr <= (req+length)) {
			status = atoi(ptr);
			ENG_LOG("%s: status=%d\n",__FUNCTION__, status);
			if(status==1) {
				ENG_LOG("%s: Create %s",__FUNCTION__,ENG_CHARGERTEST_FILE); 
				fd=open(ENG_CHARGERTEST_FILE, O_RDWR|O_CREAT|O_TRUNC);
				if(fd >= 0)
					close(fd);
				sprintf(rsp, "%s\r\n", SPRDENG_OK);

			} else if(status==0){
				ENG_LOG("%s: Delete %s",__FUNCTION__,ENG_CHARGERTEST_FILE); 
				remove(ENG_CHARGERTEST_FILE);
				sprintf(rsp, "%s\r\n", SPRDENG_OK);
			} else {
				sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
			}
		} else {
			sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
		}
	} else {
		sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
	}

	ENG_LOG("%s: rsp=%s\n",__FUNCTION__, rsp);

	return 0;
}



