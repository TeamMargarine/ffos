#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>

//#include "eng_sqlite.h"
#include "engat.h"
//#include <utils/Log.h>

#include <sys/stat.h>
#include "trout_genpskey.h"

#include "poll.h"

#include <android/log.h>
#include "cutils/properties.h"
#define BT_DBG_PRINT

#ifdef BT_DBG_PRINT
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "pskey_bt", __VA_ARGS__)
#else
#define LOGD  printf
#endif
// pskey file structure default value
BT_PSKEY_CONFIG_T bt_para_setting={
0,
0,
0,
0,
26000000,
0x001f00,
30,
{0x0000, 0x0000,0x0000,0x0000,0x0000,0x0000},
{0xB502,
 0xBBC3, 0xC943,
 0xCE41, 0xD243},
{0xFF, 0xFF, 0x8D, 0xFE, 0x9B, 0xFF, 0x79, 0x83,
  0xFF, 0xA7, 0xFF, 0x7F, 0x00, 0xE0, 0xF7, 0x3E},
{0x11, 0xFF, 0x0, 0x22, 0x2D, 0xAE},
0,
1,
5,
14,
1,
1,
0x50000000,
0x50000150,
0x50000150,
0x50000154,
0xFFFFFFFF,
0x100,
0xA0,
220,
32,
0x55,
0x64,
0,
0x8,
0x13C,
0x8CF7,
0xD704,
0x88888888,
0x0,
0x0,
{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000}
};

uint8 bt_mac_invalid = 1;

extern int get_pskey_from_file(BT_PSKEY_CONFIG_T *pskey_file);


static int counter=0;

static void mac_rand(char *btmac);



static int get_btmacaddress(char *mac_adr, char *data)
{
	int int1,i,j;
	//char *ptr = data;
	char mac[6][3] = {{0}};

#if 0
	char string1[128]={0};
	char *pstring1 = string1;

	memset(string1, 0, sizeof(string1));
printf("get_btmacaddress, ptr:%s\n", ptr);
	at_tok_start(&ptr);
	at_tok_nextint(&ptr, &int1);
	at_tok_nextstr(&ptr, &pstring1);

	memset(mac, 0 , sizeof(mac));

	for(i=0; i<6; i++) {
		strncpy(mac[i], pstring1+i*2, 2);
	}
#endif

	for(i=0; i<6; i++) {
		strncpy(mac[i], data+i*2, 2);
	}

	//sprintf(mac_adr, "%s:%s:%s:%s:%s:%s",mac[5],mac[4],mac[3],mac[2],mac[1],mac[0]);
    sprintf(mac_adr, "%s:%s:%s:%s:%s:%s",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	LOGD("%s: mac address is %s",__FUNCTION__,mac_adr);
	printf("%s: mac address is %s\n",__FUNCTION__,mac_adr);

	return 0;

}

// btmac.txt is used to store BT in android file system
static void write_to_randmacfile(char *btmac)
{
    int fd;
	char buf[80] = {0};
    //char wifimac[32] = {0};
    //char *ptr = NULL;
    char btmac_tmp[32] = {0};

	memset(buf, 0, sizeof(buf));
	printf("wite_to_randmac file, btmac:%s\n", btmac);
	get_btmacaddress(btmac_tmp, btmac);
	printf("btmac_tmp: %s\n", btmac_tmp);

	fd = open(BT_MAC_RAND_FILE, O_CREAT|O_TRUNC|O_RDWR);
	if( fd >= 0) {
		read(fd, buf, sizeof(buf));
	    LOGD("%s: read %s %s",__FUNCTION__, BT_MAC_RAND_FILE, buf);
	    printf("%s: read %s, buf:%s\n",__FUNCTION__, BT_MAC_RAND_FILE, buf);
#if 0
	    //ptr = strchr(buf, ';');
		//printf("ptr=%p\n", ptr);
		if(ptr != NULL) {
		    strcpy(wifimac, ptr+1);  // wifi MAC address
			*ptr = '\0';
            sprintf(buf, "%s;%s", btmac_tmp, wifimac);
			printf("%s: read wifimac=%s\n",__FUNCTION__, wifimac);
	    }
#endif
     //   else{
	    	sprintf(buf, "%s;", btmac_tmp);
       // }
         printf("buf:%s, len=%d\n", buf, strlen(buf));
	    //write(fd, buf, sizeof(buf));
          write(fd, buf, strlen(buf));
	    close(fd);
	}

	LOGD("%s: %s fd=%d, data=%s",__FUNCTION__, BT_MAC_RAND_FILE, fd ,buf);
}


static int send_to_modem(int fd, char *buf, int buflen)
{
	LOGD("%s",__FUNCTION__);
	return eng_write(fd, buf, buflen);
}

static int recv_from_modem(int fd, char *buf, int buflen)
{
	int n, counter;
	int readlen=-1;
	fd_set readfds;
	struct timeval timeout;

	timeout.tv_sec=20;
	timeout.tv_usec=0;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	LOGD("%s: Waiting modem response ... fd=%d",__FUNCTION__, fd);
	counter = 0;

	while((n=select(fd+1, &readfds, NULL, NULL, &timeout))<=0) {
		counter++;
		LOGD("%s: select n=%d, retry %d",__FUNCTION__, n, counter);
		if(counter > 3)
			break;
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
	}
	LOGD("%s: Receive %d Modem Response",__FUNCTION__, n);

	if(n > 0)
		readlen=eng_read(fd, buf, buflen);

	return readlen;
}

static int read_btmac_from_modem(char *btmac)
{
    char bt_mac_nv[8] = {0};

    int fd, timeout;
    int bt_ok                        =1;
    char cmdbuf[128]               = {0};
    char *tmp_str_start            = NULL;
    int count = 0;
    counter = 0;

    LOGD("%s",__FUNCTION__);

	printf("read_btmac_from_modem \n");
    fd = engapi_open(0);
    LOGD("%s modem_fd=%d",__FUNCTION__, fd);

    printf("engapi_open fd=%d\n", fd);

    while (fd < 0&&(count < 1000)) {
        count++;
        usleep(100*1000);
	      fd = engapi_open(0);
	      LOGD("%s modem_fd=%d count %d",__FUNCTION__, fd, count);
    }

#if 1
    LOGD("===========BT MAC===========");
    printf("get bt mac addr\n");
    //get bt mac address
    do {
        memset(cmdbuf, 0, sizeof(cmdbuf));
	      sprintf(cmdbuf , "%d,%d,%s",ENG_AT_NOHANDLE_CMD, 1, GET_BTMAC_ATCMD);

	      LOGD("%s: send %s",__FUNCTION__, cmdbuf);
	      send_to_modem(fd, cmdbuf, strlen(cmdbuf));

	      memset(cmdbuf, 0, sizeof(cmdbuf));
	      timeout = recv_from_modem(fd, cmdbuf, sizeof(cmdbuf));

	      printf("recv from modem timeout=%d, cmdbuf=%s\n", timeout, cmdbuf);
				LOGD("%s: BT timeout=%d, response=%s\n", __FUNCTION__,timeout,cmdbuf);
				usleep(100*1000);
				counter++;
    }while((timeout>0)&&(strstr(cmdbuf, "OK") == NULL));

    if( timeout<=0 ) {
        LOGD("%s: Get BT MAC from modem fail",__FUNCTION__);
				engapi_close(fd);
				return 0;
    }

    tmp_str_start = strstr(cmdbuf, ",");

    memcpy(btmac, tmp_str_start+1, 16);  // 6 byte BT address, 2 byte xtal_dac
	printf("bt_mac_nv:%s\n", btmac);
#endif

    //if(strstr(btmac,  MAC_ERROR)!=NULL){   // bt mac is FF:FF:FF:FF:FF:FF
    if(strstr(btmac, "FFFFFFFFFFFF")){
		bt_ok = 0;
    }
    else{
		bt_ok = 1;
    }

    engapi_close(fd);
    return bt_ok;
}

static int read_btpskey_from_nv(char *pskey)
{
    int fd, timeout;
    char cmdbuf[400] = {0};
    char *str_tmp_start  = NULL;
    counter = 0;
	LOGD("%s",__FUNCTION__);
    printf("read_btpskey_from_nv \n");
    fd = engapi_open(0);

    while (fd < 0) {
        usleep(100*1000);
		    fd = engapi_open(0);
				LOGD("%s modem_fd=%d",__FUNCTION__, fd);
    }

    // get bt pskey file
    do {
        memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, GET_BTPSKEY_ATCMD);

				send_to_modem(fd, cmdbuf, strlen(cmdbuf));
				memset(cmdbuf, 0, sizeof(cmdbuf));
				timeout = recv_from_modem(fd, cmdbuf, sizeof(cmdbuf));

				LOGD("%s: BT timeout=%d, response=%s\n", __FUNCTION__,timeout,cmdbuf);
				usleep(100*1000);
				counter++;
    }while((timeout>0)&&(strstr(cmdbuf, "OK") == NULL));

    if(timeout<=0) {
        LOGD("%s: Get BT pskey from modem fail",__FUNCTION__);
        printf("get BT pskey from modem fail\n");
		    engapi_close(fd);
			  return 0;
    }

    printf("get pskey NV success\n");

    str_tmp_start = strstr(cmdbuf, ",");
    if(NULL != str_tmp_start){
        memcpy(pskey, str_tmp_start+1, sizeof(BT_SPRD_NV_PARAM)*2);
        pskey[sizeof(BT_SPRD_NV_PARAM)*2] = '\0';
    }

    printf("bt pskey:%s\n", pskey);

    return 1;
}

static void mac_rand(char *btmac)
{
	int fd,i, j, k;
	off_t pos;
	char buf[80];
	char *ptr;
	unsigned int randseed;

	memset(buf, 0, sizeof(buf));
#if 0
	if(access(MAC_RAND_FILE, F_OK) == 0) {
		LOGD("%s: %s exists",__FUNCTION__, MAC_RAND_FILE);
		fd = open(MAC_RAND_FILE, O_RDWR);
		if(fd>=0) {
			read(fd, buf, sizeof(buf));
			LOGD("%s: read %s %s",__FUNCTION__, MAC_RAND_FILE, buf);
			ptr = strchr(buf, ';');
			if(ptr != NULL) {

				if((strstr(wifimac, MAC_ERROR)!=NULL)||(strstr(wifimac, MAC_ERROR_EX)!=NULL)||(strlen(wifimac)==0))
					strcpy(wifimac, ptr+1);

				*ptr = '\0';

				if((strstr(btmac, MAC_ERROR)!=NULL)||(strstr(btmac, MAC_ERROR_EX)!=NULL)||(strlen(btmac)==0))
					strcpy(btmac, buf);

				LOGD("%s: read btmac=%s, wifimac=%s",__FUNCTION__, btmac, wifimac);
				return;
			}
			close(fd);
		}
	}

	usleep(counter*1000);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "dmesg>%s",KMSG_LOG);
	system(buf);
	fd = open(KMSG_LOG, O_RDONLY);
	LOGD("%s: counter=%d, fd=%d",__FUNCTION__, counter, fd);
	if (fd > 0) {
		pos = lseek(fd, -(counter*10), SEEK_END);
		memset(buf, 0, sizeof(buf));
		read(fd, buf, counter);
		LOGD("%s: read %s: %s",__FUNCTION__, KMSG_LOG, buf);
	}
#endif

	k=0;
	for(i=0; i<counter; i++)
		k += buf[i];

	//rand seed
	randseed = (unsigned int) time(NULL) + k*fd*counter + buf[counter-2];
	LOGD("%s: randseed=%d",__FUNCTION__, randseed);
	srand(randseed);

	//FOR BT
	i=rand(); j=rand();
	LOGD("%s:  rand i=0x%x, j=0x%x",__FUNCTION__, i,j);
	sprintf(btmac, "00%02x%02x%02x%02x%02x", \
									   (unsigned char)((i>>8)&0xFF), \
									   (unsigned char)((i>>16)&0xFF), \
									   (unsigned char)((j)&0xFF), \
									   (unsigned char)((j>>8)&0xFF), \
									   (unsigned char)((j>>16)&0xFF));
}



static void write_btpskey2file(BT_SPRD_NV_PARAM *bt_sprd_nv_param)
{
    int fd;
    char buf[400] = {0};
    int file_ok= 0;

    printf("write_2 pskey file\n");
    printf("nv_parm.device_class=0x%x\n", (*bt_sprd_nv_param).device_class);
		printf("nv_parm.g_PrintLevel=0x%x\n", (*bt_sprd_nv_param).g_PrintLevel);

		bt_para_setting.device_class        = (*bt_sprd_nv_param).device_class;
		bt_para_setting.g_PrintLevel        = (*bt_sprd_nv_param).g_PrintLevel;

		bt_para_setting.g_dbg_source_sink_syn_test_data = (*bt_sprd_nv_param).config0 & 0x00000001;
		bt_para_setting.g_sys_sleep_in_standby_supported = ((*bt_sprd_nv_param).config0 & 0x00000002)>>1;
	  bt_para_setting.g_sys_sleep_master_supported = ((*bt_sprd_nv_param).config0 & 0x00000004)>>2;
	  bt_para_setting.g_sys_sleep_slave_supported = ((*bt_sprd_nv_param).config0 & 0x00000008)>>3;
	  bt_para_setting.g_sys_sco_transmit_mode= ((*bt_sprd_nv_param).config0 & 0x00000010)>>4;
	  bt_para_setting.g_sys_uart0_communication_supported = ((*bt_sprd_nv_param).config0 & 0x00000020)>>5;
		bt_para_setting.abcsp_rxcrc_supported = ((*bt_sprd_nv_param).config0 & 0x00000040)>>6;
		bt_para_setting.abcsp_txcrc_supported = ((*bt_sprd_nv_param).config0 & 0x00000080)>>7;

		bt_para_setting.ctrl_reserved = ((*bt_sprd_nv_param).config0& 0xFFFF0000) >>16;
		bt_para_setting.edr_tx_edr_delay = (*bt_sprd_nv_param).config1& 0x000000FF;
		bt_para_setting.edr_rx_edr_delay = ((*bt_sprd_nv_param).config1& 0x0000FF00)>>8;
		bt_para_setting.uart_rx_watermark  = ((*bt_sprd_nv_param).config1& 0x00FF0000)>>16;
		bt_para_setting.uart_flow_control_thld   = ((*bt_sprd_nv_param).config1 & 0xFF000000)>>24;

		bt_para_setting.tx_channel_compensate = (*bt_sprd_nv_param).tx_channel_compensate;
	  bt_para_setting.win_ext = (*bt_sprd_nv_param).win_ext;

	  bt_para_setting.uart_clk_divd = (*bt_sprd_nv_param).config3& 0x0000FFFF;
		bt_para_setting.pcm_clk_divd = ((*bt_sprd_nv_param).config3& 0xFFFF0000)>>16;

		bt_para_setting.debug_bb_tx_gain  = (*bt_sprd_nv_param).config2& 0x0000FFFF;
		bt_para_setting.debug_tx_power  = ((*bt_sprd_nv_param).config2& 0xFFFF0000)>>16;

		memcpy((uint8*)(bt_para_setting.g_aGainValue), (uint8*)(*bt_sprd_nv_param).g_aGainValue,6*4);

		memcpy((uint8*)bt_para_setting.g_aPowerValue, (uint8*)(*bt_sprd_nv_param).g_aPowerValue,5*4);

		memcpy((uint8*)bt_para_setting.feature_set, (uint8*)((*bt_sprd_nv_param).feature_set),4*4);

		memcpy((uint8*)bt_para_setting.reserved, (uint8*)((*bt_sprd_nv_param).reserved),10*4);

		fd = open(BT_PSKEY_FILE, O_CREAT|O_RDWR|O_TRUNC, 0644);

		if(fd > 0) {
			write(fd, (char *)&bt_para_setting, sizeof(BT_PSKEY_CONFIG_T));
			close(fd);
	  }

    printf("write pskey file complete\n");
}

uint8 ConvertHexToBin(
			uint8        *hex_ptr,     // in: the hexadecimal format string
			uint16       length,       // in: the length of hexadecimal string
			uint8        *bin_ptr      // out: pointer to the binary format string
			){
    uint8        *dest_ptr = bin_ptr;
    uint32        i = 0;
    uint8        ch;

    for(i=0; i<length; i+=2){
		    // the bit 8,7,6,5
				ch = hex_ptr[i];
				// digital 0 - 9
				if (ch >= '0' && ch <= '9')
				    *dest_ptr =(uint8)((ch - '0') << 4);
				// a - f
				else if (ch >= 'a' && ch <= 'f')
				    *dest_ptr = (uint8)((ch - 'a' + 10) << 4);
				// A - F
				else if (ch >= 'A' && ch <= 'F')
				    *dest_ptr = (uint8)((ch -'A' + 10) << 4);
				else{
				    return 0;
				}

				// the bit 1,2,3,4
				ch = hex_ptr[i+1];
				// digtial 0 - 9
				if (ch >= '0' && ch <= '9')
				    *dest_ptr |= (uint8)(ch - '0');
				// a - f
				else if (ch >= 'a' && ch <= 'f')
				    *dest_ptr |= (uint8)(ch - 'a' + 10);
				// A - F
				else if (ch >= 'A' && ch <= 'F')
				    *dest_ptr |= (uint8)(ch -'A' + 10);
				else{
			            return 0;
				}

				dest_ptr++;
	  }

    return 1;
}


void wirte_btmac2nv(char * bt_mac_rand, uint8 * bt_mac_bin)
{
	BT_NV_PARAM bt_mac_nv;

    int fd, timeout;
    char cmdbuf[128]               = {0};
    char bt_mac_rand_tmp[16] = {0};

    counter = 0;

    fd = engapi_open(0);
    LOGD("%s modem_fd=%d",__FUNCTION__, fd);

    printf("engapi_open fd=%d\n", fd);

    while (fd < 0) {
        usleep(100*1000);
	fd = engapi_open(0);
	LOGD("%s modem_fd=%d",__FUNCTION__, fd);
    }

	do {
		memset(cmdbuf, 0, sizeof(cmdbuf));

		sprintf(cmdbuf, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, SET_BTMAC_ATCMD);

		LOGD("%s: send %s",__FUNCTION__, cmdbuf);
		printf("write data cmdbuf:%s\n", cmdbuf);
		send_to_modem(fd, cmdbuf, strlen(cmdbuf));
		memset(cmdbuf, 0, sizeof(cmdbuf));
		timeout = recv_from_modem(fd, cmdbuf, sizeof(cmdbuf));
		printf("recv from modem cmdbuf=%s\n", cmdbuf);
		LOGD("%s: BT timeout=%d, response=%s\n", __FUNCTION__,timeout,cmdbuf);
		usleep(100*1000);
		counter++;
	}while((timeout>0)&&(strstr(cmdbuf, ">") == NULL));

	if(timeout<=0) {
		LOGD("%s: set bt mac fail",__FUNCTION__);
		printf("set bt mac fail\n");
		engapi_close(fd);
		return;
	}

	printf("set bt mac nv success\n");
	printf("write data to NV\n");

    {
        int i =0;
        char mac[6][3] = {{0}};
        for(i=0; i<6; i++) {
            strncpy(mac[i], bt_mac_rand+i*2, 2);
        }

        sprintf(bt_mac_rand_tmp, "%s%s%s%s%s%s",mac[5],mac[4],mac[3],mac[2],mac[1],mac[0]);
    }


	// write BT mac data
	do {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		sprintf(cmdbuf, "%d,%d,%s%02x%02x", ENG_AT_SET_VALUE,1, bt_mac_rand_tmp, bt_mac_bin[6], bt_mac_bin[7]);
		printf("wirte BT mac data: %s\n", cmdbuf);
		send_to_modem(fd, cmdbuf, strlen(cmdbuf));
		memset(cmdbuf, 0, sizeof(cmdbuf));
		timeout = recv_from_modem(fd, cmdbuf, sizeof(cmdbuf));

		LOGD("%s: BT timeout=%d, response=%s\n", __FUNCTION__,timeout,cmdbuf);
		usleep(100*1000);
		counter++;
	}while((timeout>0)&&(strstr(cmdbuf, "OK") == NULL));

	if(timeout<=0) {
		LOGD("%s: write data fail",__FUNCTION__);
		printf("write data fail\n");
		engapi_close(fd);
		return;
	}
}

#define BT_MAC_FILE_PATH			"/data/btmac.txt"

int main(void)
{
    int bt_mac_ok=0;
    char bt_mac[30] = {0};
    char bt_mac_tmp[20] = {0};
    char bt_rand_mac[20] = {0};
    char bt_rand_mac_bin[32] = {0};
    char *bt_ptr = NULL;
    uint8 bt_mac_bin[32]     = {0};
    int fd                   = 0;
    int sz                   = 0;
    char buf[3]              = {0};
    char buf_pskey[182]      = {0};
    BT_PSKEY_CONFIG_T bt_para_tmp;
    int ret  = 0;
    char bt_prop[PROPERTY_VALUE_MAX] ={0};
    char tmp[80] = {0};
    char tmp_mac[20] = {0};
    char tmp_mac_bin[20] = {0};
     int count_timer = 0;
    int i =0;

    LOGD("set BT mac\n");

    LOGD("main, bt_mac from nv:%s\n", bt_mac);


    char bt_pskey_nv_param[sizeof(BT_SPRD_NV_PARAM)*2+1] = {0};
    char bt_pskey_nv_param_bin[sizeof(BT_SPRD_NV_PARAM)+1] = {0};

    memset(bt_pskey_nv_param, 0x0, sizeof(bt_pskey_nv_param));
    memset(bt_pskey_nv_param_bin, 0x0, sizeof(bt_pskey_nv_param_bin));

#if 0
    bt_mac_ok = read_btmac_from_modem(bt_mac);

    LOGD("main, bt_mac from nv:%s\n", bt_mac);
    ConvertHexToBin(bt_mac, strlen(bt_mac), bt_mac_bin);

    // BT mac is invalid
    if(bt_mac_ok==0){
        mac_rand(bt_rand_mac);
        LOGD("bt_mac_ok == 0,bt_rand_mac: %s\n", bt_rand_mac);

        // write BT mac to NV
        //wirte_btmac2nv(bt_rand_mac, bt_mac_bin);

        memcpy(bt_mac, bt_rand_mac, strlen(bt_rand_mac));
        bt_mac[strlen(bt_rand_mac)] = '\0';

        ConvertHexToBin(bt_rand_mac, strlen(bt_rand_mac), bt_rand_mac_bin);

        memcpy(bt_mac_bin, bt_rand_mac_bin, 6);
        bt_mac_bin[strlen(bt_rand_mac_bin)] = '\0';
        printf("bt_rand_mac bin len=%d\n", strlen(bt_rand_mac_bin));

        // revert BT MAC
        {
            int j = 0;

	        for(j=0; j<6; j++){
	            bt_para_setting.device_addr[j] = bt_mac_bin[5-j];
	            LOGD("%x  ", bt_para_setting.device_addr[j]);
	        }
       }
    }
    else{
        memcpy(bt_para_setting.device_addr, bt_mac_bin, 6);
    }

    bt_mac[12] = '\0';
#endif

#ifdef PSKEY_FROM_ANDROID

    ret = get_pskey_from_file(&bt_para_tmp);
    LOGD("ret get pskey from file =%d", ret);
    if(ret != 0){
        LOGD("get pskey fail !!!");
        exit(1);
    }

        while((access(BT_MAC_FILE_PATH, F_OK)!=0)){
        count_timer++;
        usleep(1000*10);
    }
       LOGD("data/btmac.txt does not exist %d ", count_timer);
     usleep((1000*100));
     fd = open(BT_MAC_FILE_PATH, O_RDONLY);
     if(fd > 0){
      read(fd, tmp, sizeof(tmp));
       close(fd);
     for(i=0; i<6; i++){
        tmp_mac[i*2] = tmp[3*i];
        tmp_mac[i*2+1] = tmp[3*i+1];
     }
   }

    LOGD("tmp_mac:%s", tmp_mac);
    ConvertHexToBin(tmp_mac, strlen(tmp_mac), tmp_mac_bin);
	 // revert BT MAC
    {
        int j = 0;

        for(j=0; j<6; j++){
	        bt_para_tmp.device_addr[j] = tmp_mac_bin[5-j];
	       LOGD("%x  ", bt_para_tmp.device_addr[j]);
	   }
    }

    //memcpy(bt_para_tmp.device_addr, tmp_mac_bin, sizeof(bt_para_tmp.device_addr));

    fd = open(BT_PSKEY_FILE, O_CREAT|O_RDWR|O_TRUNC, 0644);
	  if(fd > 0) {
		    write(fd, (char *)&bt_para_tmp, sizeof(BT_PSKEY_CONFIG_T));
		    close(fd);
    }
#else
    // update  pskey param to pskey.txt
    read_btpskey_from_nv(bt_pskey_nv_param);

    if(ConvertHexToBin(bt_pskey_nv_param, strlen(bt_pskey_nv_param), bt_pskey_nv_param_bin)){
        write_btpskey2file((BT_SPRD_NV_PARAM *) bt_pskey_nv_param_bin);
    }
    else{
        printf("convert fail\n");
    }

    LOGD("nv_param_bin:%s\n", bt_pskey_nv_param_bin);
    LOGD("bt_mac: %s\n", bt_mac);
    printf("bt_mac: %s\n", bt_mac);
#endif
#if 0
    LOGD("property ro.mac.bluetooth=%s",bt_mac);

    if(bt_mac_ok == 1){
        char tmp_mac[13] = {0};
        int k = 0;

        for(k=0; k<6; k++){
            tmp_mac[2*k] = bt_mac[2*(5-k)];
            tmp_mac[2*k+1] = bt_mac[2*(5-k)+1];
        }
        get_btmacaddress(bt_mac_tmp, tmp_mac);
    }
    else{
        get_btmacaddress(bt_mac_tmp, bt_mac);
    }

    property_set("ro.mac.bluetooth",bt_mac_tmp);
#endif
    return 0;
}
