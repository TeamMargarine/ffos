#ifndef __ENG_RF_NV_CONFIG_H__
#define __ENG_RF_NV_CONFIG_H__

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "engat.h"


#define NV_CAL_TRACE_ENABLE // NV calibration trace enable 

#define INLINE static __inline

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned short uint16;

#define NV_VERSION_NO 4

#define GET_RF_NVCONF_ATCMD			"AT+SNVM=0,415"
#define SET_RF_NVCONF_ATCMD			"AT+SNVM=1,415"
#define GET_RF_NVCONF2_ATCMD		"AT+SNVM=0,416"
#define SET_RF_NVCONF2_ATCMD		"AT+SNVM=1,416"
#define GET_RF_NVCONF3_ATCMD		"AT+SNVM=0,417"
#define SET_RF_NVCONF3_ATCMD		"AT+SNVM=1,417"

#define NOT_UPDATED 0x5A		// need to equal to the same definition in trout2_interface.c
#define ALREADY_UPDATED 0xA5	// need to equal to the same definition in trout2_interface.c
#define NV_BAK_FILE  "/data/nv_bak.txt"
#define LOGD LOGE

#define NPI_CAL_RF_CMD_PATH 		"/sys/kernel/trout_nv_cal/trout_nvcal_cmd"
#define NPI_CAL_UPDATE_CMD_PATH 	"/sys/kernel/trout_nv_cal/trout_nvupdate_cmd"

typedef enum
{
    NOT_READY = 0x29,
    SELFCAL_DEFAULT = 0x30,
    SELFCAL_WAITING = 0x31,
    SELFCAL_DONE    = 0x32,
}SELFCAL_TYPE;

typedef struct 
{
    unsigned char val[8];
}Long_pad_ST;

/* use for read/write nv */
typedef struct
{
    // nv struct part 1
	Long_pad_ST nv_ver;
	Long_pad_ST cal_flag;
	Long_pad_ST updated_nv_flag; // use for rf self calibration
	Long_pad_ST antenna_switch;
	Long_pad_ST pa_id;
	Long_pad_ST lna_id;
    Long_pad_ST wifi_imb[2];
	Long_pad_ST wifi_LOFT[4];
	Long_pad_ST DAC_scale_chnnl[14];
	Long_pad_ST PA_table[32];
	Long_pad_ST tx_gain[2];
	Long_pad_ST PAD_table[32];
	Long_pad_ST BT_LOFT[4];
	Long_pad_ST Pout_self_check[18];
	Long_pad_ST reserved[8]; // 122 long
    // nv struct part 2
	// use for rf self calibration
	Long_pad_ST btImb[2];
	Long_pad_ST btPaTbl[6];
	Long_pad_ST btPadTbl[6];
	Long_pad_ST wifiDcoc[64];
	Long_pad_ST btDcoc[32]; // 110 long
    // nv struct part 3
	Long_pad_ST reserved2[50]; // 110+60 long
} NV_ADJUST_REG_pad_ST;


/* nv struct for driver */
typedef struct nv_adjust_reg_t
{
	uint32 nv_ver;
	uint32 cal_flag;
	uint32 updated_nv_flag; // use for rf self calibration
	uint32 antenna_switch;
	uint32 pa_id;
	uint32 lna_id;
    uint32 wifi_imb[2];
	uint32 wifi_LOFT[4];
	uint32 DAC_scale_chnnl[14];
	uint32 PA_table[32];
	uint32 tx_gain[2];
	uint32 PAD_table[32];
	uint32 BT_LOFT[4];
	uint32 Pout_self_check[18];
	uint32 reserved[8];
	// use for rf self calibration
	uint32 btImb[2];
	uint32 btPaTbl[6];
	uint32 btPadTbl[6];
	uint32 wifiDcoc[64];
	uint32 btDcoc[32];
	uint32 reserved2[50]; //0806 add
} NV_ADJUST_REG_T;


static void hex_dump(char *info, unsigned char *str, unsigned int len)
{
	unsigned int  i = 0;

    if(str == NULL || len == 0)
        return;

	LOGD("dump %s, len: %d; data:\n",info,len);
	for(i = 0; i<len; i++)
	{
		if(((unsigned char *)str+i)==NULL)
			break;
		LOGD("%#x ",*((unsigned char *)str+i));
		if((i+1)%16 == 0)
			LOGD("\n");
	}
	LOGD("\n");
}

/* Macros to swap byte order */
#define SWAP_BYTE_ORDER_WORD(val) ((((val) & 0x000000FF) << 24) + \
                                   (((val) & 0x0000FF00) << 8)  + \
                                   (((val) & 0x00FF0000) >> 8)   + \
                                   (((val) & 0xFF000000) >> 24))

INLINE uint32 u32_convert_to_le(uint32 val)
{
    return SWAP_BYTE_ORDER_WORD(val);
}
INLINE uint32 u32_convert_to_be(uint32 val)
{
    return SWAP_BYTE_ORDER_WORD(val);
}

INLINE int conv_str2hex(uint8 ch, uint8 *out)
{
    int ret = 0;
    
    // digital 0 - 9
    if (ch >= '0' && ch <= '9')
        *out = (ch - '0');
    // a - f
    else if (ch >= 'a' && ch <= 'f')
        *out = (ch - 'a' + 10);
    // A - F
    else if (ch >= 'A' && ch <= 'F')
        *out = (ch - 'A' + 10);
    else
        ret = -1;

    return ret;
}

INLINE uint8 ConvertStrToU32Hex(
			uint8        *u32_hex_ptr,  // in: the short hexadecimal format string
			uint16       length,        // in: the length of hexadecimal string 
			uint32       *bin_ptr       // out: pointer to the binary format string
			)
{
    uint32       *dest_ptr = bin_ptr;
    uint32       i = 0;
    uint32       j = 0;
    uint32       k = 0;
    uint32       a = 0;
    uint8        ch;
    uint8        ch_bin;
    int          ret;

    //LOGD("before convert str = [%s], len = [%d]\n", u32_hex_ptr, length);
    
    for(i=0; i<length; i+=8)
    {
        *dest_ptr = 0;
        j = i;
        a = 1;

        // 8 CHAR = 1 U32
        for(k=0; k<8; k++)
        {
            //LOGD("before j = %d, convert %c\n",j,u32_hex_ptr[j]);
            
            ch = u32_hex_ptr[j++];
            ret = conv_str2hex(ch, &ch_bin);
            if(ret<0)
            {
                LOGD("%s: error of this hex = [%#x].\n",__FUNCTION__,ch);
                return 0;
            }
            //LOGD("convert ch_bin %#x, a=%d (%d), %#x, %#x\n",ch_bin, a, 4*a, (uint32)(ch_bin << (a*4)),((uint32)ch_bin << (a*4)));
            *dest_ptr |= ((uint32)ch_bin << (32 - a*4));
            a++;
            //LOGD("%s: ch_bin = [%#x], dest_ptr = [%#x].\n",__FUNCTION__,ch_bin,*dest_ptr);
        }

    	#ifdef NV_CAL_TRACE_ENABLE
        //LOGD("%s: u32 [%03d] [%#x].\n",__FUNCTION__, dest_ptr - bin_ptr, *dest_ptr);
        #endif
		
        dest_ptr++;
    }

    return 1;
}


INLINE int conv_hex2str(uint8 hex, uint8 *str)
{
    unsigned char c;

    if(str == NULL)
    {
        LOGD("%s: error!  str null.\n",__FUNCTION__);
        return -1;
    }

    c = ((hex >> 4) & 0xf);
    //if((0x0 <= c) && (c <= 0x9))
    if((c <= 0x9))
    {
        c += '0';
    }
    else if((0xa <= c) && (c <= 0xf))
    {
        c = c - 0xa + 'a';
    }
    *str = c;

    c = ((hex) & 0xf);
    //if((0x0 <= c) && (c <= 0x9))
    if((c <= 0x9))
    {
        c += '0';
    }
    else if((0xa <= c) && (c <= 0xf))
    {
        c = c - 0xa + 'a';
    }
    *(str+1) = c;

	#ifdef NV_CAL_TRACE_ENABLE
        //LOGD("%s: hex=%#x, str = %c-%c\n",__FUNCTION__, hex, *str, *(str+1));
	#endif
    return 0;
}
INLINE uint8 ConvertU32HexToStr(
			uint8        *bin_ptr,              // in 
			uint16        bin_ptr_len,
			uint8        *hex_str_ptr,          // out
			uint16        hex_str_ptr_len
			)
{
    uint32       i = 0;
	uint32       len = 0;

    for(i=0; i<bin_ptr_len; i++,len+=2)
    {
        conv_hex2str(bin_ptr[i], (hex_str_ptr + len));
    }

    return 0;
}


INLINE int nvcal_send_to_modem(int fd, char *buf, int buflen)
{
	LOGD("%s\n",__FUNCTION__);
	return eng_write(fd, buf, buflen);
}

INLINE int nvcal_recv_from_modem(int fd, char *buf, int buflen)
{
	int n, counter;
	int readlen=-1;
	fd_set readfds;
	struct timeval timeout;

	timeout.tv_sec=20;
	timeout.tv_usec=0;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	LOGD("%s: Waiting modem response ... fd=%d\n",__FUNCTION__, fd);
	counter = 0;

	while((n=select(fd+1, &readfds, NULL, NULL, &timeout))<=0) {
		counter++;
		LOGD("%s: select n=%d, retry %d\n",__FUNCTION__, n, counter);
		if(counter > 3)
			break;
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
	}
	LOGD("%s: Receive %d Modem Response\n",__FUNCTION__, n);

	if(n > 0)
		readlen=eng_read(fd, buf, buflen);

	return readlen;
}

INLINE int read_rf_nvconf_cmd(char *atcmd, char *rf_nvconf, unsigned int rf_nvconf_len)
{
    int fd, timeout;
    int bt_ok                      = 0;
    char cmdbuf[2048]               = {0};
    char *tmp_str_start            = NULL;

    //LOGD("%s",__FUNCTION__);
    
    LOGD("%s \n", __FUNCTION__);
    fd = engapi_open(0);
    //LOGD("%s modem_fd=%d",__FUNCTION__, fd);

    while (fd < 0) {
        usleep(100*1000);
        fd = engapi_open(0);
        LOGD("%s modem_fd=%d",__FUNCTION__, fd);
    }

    do {
        memset(cmdbuf, 0, sizeof(cmdbuf));
        sprintf(cmdbuf , "%d,%d,%s",ENG_AT_NOHANDLE_CMD, 1, atcmd);

        LOGD("%s: send [%s]",__FUNCTION__, cmdbuf);
        nvcal_send_to_modem(fd, cmdbuf, strlen(cmdbuf));

        memset(cmdbuf, 0, sizeof(cmdbuf));
        timeout = nvcal_recv_from_modem(fd, cmdbuf, sizeof(cmdbuf));

        printf("recv from modem timeout=%d, cmdbuf=[%s]\n", timeout, cmdbuf);
        LOGD("%s:  timeout=%d, response=[%s]\n", __FUNCTION__,timeout,cmdbuf);
        usleep(100*1000);
    }while((timeout>0)&&(strstr(cmdbuf, "OK") == NULL));

    if( timeout<=0 ) {
        LOGD("%s: Get info from modem fail",__FUNCTION__);
        engapi_close(fd);
        return -1;
    }

    tmp_str_start = strstr(cmdbuf, ",");

    memcpy(rf_nvconf, (tmp_str_start+1), rf_nvconf_len);

    engapi_close(fd);
    return bt_ok;
}

INLINE int read_rf_nvconf_from_modem(
    char *conf_part1, unsigned int part1_len, 
    char *conf_part2, unsigned int part2_len,
	char *conf_part3, unsigned int part3_len)
{
	LOGD("%s: part1=[%p,%d]; part2=[%p,%d]; part3=[%p,%d];\n",__FUNCTION__,
        conf_part1,part1_len,
        conf_part2,part2_len,
        conf_part3,part3_len);

	if(-1 == read_rf_nvconf_cmd(GET_RF_NVCONF_ATCMD, conf_part1, part1_len))
	{
		LOGD("%s: read part1 data failed\n",__FUNCTION__);
		return -1;
	}
	if(-1 == read_rf_nvconf_cmd(GET_RF_NVCONF2_ATCMD, conf_part2, part2_len))
	{
		LOGD("%s: read part2 data failed\n",__FUNCTION__);
		return -1;
	}
	if(-1 == read_rf_nvconf_cmd(GET_RF_NVCONF3_ATCMD, conf_part3, part3_len))
	{
		LOGD("%s: read part3 data failed\n",__FUNCTION__);
		return -1;
	}
	
    return 0;
}


INLINE int write_rf_nvconf_cmd(char *atcmd, char *rf_nvconf, unsigned int rf_nvconf_len)
{
	int ret = 0;
	int fd = 0;
	int count = 0;
	int recv_num = 0;
	char cmdbuf[2048] = {0};
	int i = 0;

	LOGD("%s",__FUNCTION__);
	fd = engapi_open(0);
	while (fd < 0 && count < 100)
	{
		usleep(100*1000);
		fd = engapi_open(0);
		LOGD("%s modem_fd=%d",__FUNCTION__, fd);
		count++;
    }
	if (count >= 100)
	{
		LOGD("engapi open fail, fd is: %d\n", fd);
		return -1;
	}
	do
	{
		memset(cmdbuf, 0, sizeof(cmdbuf));
		sprintf(cmdbuf, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, atcmd);
		LOGD("%s: send [%s]",__FUNCTION__, cmdbuf);
		LOGD("write data cmdbuf:[%s]\n", cmdbuf);
		nvcal_send_to_modem(fd, cmdbuf, strlen(cmdbuf));
		memset(cmdbuf, 0, sizeof(cmdbuf));
		recv_num = nvcal_recv_from_modem(fd, cmdbuf, sizeof(cmdbuf));
		LOGD("recv from modem cmdbuf=[%s]\n", cmdbuf);
		usleep(100*1000);
        if((strstr(cmdbuf, ">") == NULL))
        {
		    LOGD("(strstr(cmdbuf, \">\") == NULL)\n");
        }
        else
        {
		    LOGD("(strstr(cmdbuf, \">\") != NULL)\n");
        }
	}while((recv_num>0)&&(strstr(cmdbuf, ">") == NULL));


	if(recv_num <= 0) {
		LOGD("%s: set wifi  fail",__FUNCTION__);
		LOGD("set wifi  fail\n");
		engapi_close(fd);
		return -1;
	}

	// write  data
	do {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		//sprintf(cmdbuf, "%d,%d,%s", ENG_AT_SET_VALUE,1, rf_nvconf);
		sprintf(cmdbuf, "%d,%d,", ENG_AT_NOHANDLE_CMD,1);
        	strncat(cmdbuf, rf_nvconf, rf_nvconf_len);
		cmdbuf[strlen(cmdbuf)] = 0x1a;
		LOGD("%s: cmd_len[%d], nv_len[%d]\n", __FUNCTION__, strlen(cmdbuf),rf_nvconf_len);
		LOGD("%s: cmd_buf:[%s]\n", __FUNCTION__,cmdbuf);
		nvcal_send_to_modem(fd, cmdbuf, strlen(cmdbuf));
		memset(cmdbuf, 0, sizeof(cmdbuf));
		recv_num = nvcal_recv_from_modem(fd, cmdbuf, sizeof(cmdbuf));
		
		LOGD("%s: wifi recv_num =%d, response=[%s]\n", __FUNCTION__,recv_num,cmdbuf);
		usleep(100*1000);
	}while((recv_num>0)&&(strstr(cmdbuf, "OK") == NULL));
	
	if(recv_num<=0) {
		LOGD("%s: write data fail\n",__FUNCTION__);
		LOGD("write data fail\n"); 
		
		ret = -1;
	}

	engapi_close(fd);

	return ret;
}


INLINE int write_rf_nvconf_to_modem(
    char *conf_part1, unsigned int part1_len, 
    char *conf_part2, unsigned int part2_len,
	char *conf_part3, unsigned int part3_len)
{
	LOGD("%s: part1=[%p,%d]; part2=[%p,%d]; part3=[%p,%d];\n",__FUNCTION__,
        conf_part1,part1_len,
        conf_part2,part2_len,
        conf_part3,part3_len);
	if(-1 == write_rf_nvconf_cmd(SET_RF_NVCONF_ATCMD, conf_part1, part1_len))
	{
		LOGD("%s: write part1 data failed\n",__FUNCTION__);
		return -1;
	}
	
	if(-1 == write_rf_nvconf_cmd(SET_RF_NVCONF2_ATCMD, conf_part2, part2_len))
	{
		LOGD("%s: write part2 data failed\n",__FUNCTION__);
		return -1;
	}
	if(-1 == write_rf_nvconf_cmd(SET_RF_NVCONF3_ATCMD, conf_part3, part3_len))
	{
		LOGD("%s: write part3 data failed\n",__FUNCTION__);
		return -1;
	}
	
    return 0;
}


INLINE int send_info_to_driver(char *info, unsigned int info_len)
{
    int dfd = 0;
    int len = 0;
	char *path = NPI_CAL_RF_CMD_PATH;

    do
    {
        dfd = open(path, O_RDWR);
        if(dfd < 0)
        {
            LOGD("%s: open %s fail.\n",__FUNCTION__,path);
            usleep(300);
            continue;
        }
    }while((dfd < 0));

    len = write(dfd, info, info_len);
    if(len <= 0)
    {
        LOGD("%s: write info error! \n", __FUNCTION__);
        goto out;
    }
    
    close(dfd);
    return 0;

out:
    close(dfd);
    return -1;
}

INLINE int get_info_from_driver(char *info, unsigned int info_len)
{
    int dfd = 0;
    int len = 0;
    char ret_buff[8];
	char *path = NPI_CAL_UPDATE_CMD_PATH;
    
    do
    {
        dfd = open(path, O_RDWR);
        if(dfd < 0)
        {
            LOGD("%s: open %s fail.\n",__FUNCTION__,path);
            usleep(300);
            continue;
        }
    }while((dfd < 0));


    do
    {
        len = read(dfd, info, info_len);
        if(len <= 0)
        {
            LOGD("%s: Waiting response for 100us!\n", __FUNCTION__);
            usleep(100);
            continue;
        }
    }while((len <= 0));
    
	
    close(dfd);
    return len;
}


#endif /* __ENG_RF_NV_CONFIG_H__ */
