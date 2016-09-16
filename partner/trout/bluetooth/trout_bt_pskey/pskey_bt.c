#include <linux/kernel.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
//#include <linux/gfp.h>


//#include <linux/slab.h>
#if 0
#include <linux/miscdevice.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/syscalls.h>
#endif

#include "trout_genpskey.h"

#include <android/log.h>
#define DBG
#ifdef DBG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "pskey_bt", __VA_ARGS__)
#else
#define LOGD  printf
#endif


#define MAX_BT_TMP_PSKEY_FILE_LEN 2048

typedef unsigned int   UWORD32;
typedef unsigned short UWORD16;
typedef unsigned char  UWORD8;

#define down_bt_is_space(c)	(((c) == '\n') || ((c) == ',') || ((c) == '\r') || ((c) == ' ') || ((c) == '{') || ((c) == '}'))
#define down_bt_is_comma(c)	(((c) == ','))
#define down_bt_is_endc(c)	(((c) == '}')) // indicate end of data

/* Macros to swap byte order */
#define SWAP_BYTE_ORDER_WORD(val) ((((val) & 0x000000FF) << 24) + \
                                   (((val) & 0x0000FF00) << 8)  + \
                                   (((val) & 0x00FF0000) >> 8)   + \
                                   (((val) & 0xFF000000) >> 24))
#define INLINE static __inline

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif

INLINE UWORD32 convert_to_le(UWORD32 val)
{
#ifdef BIG_ENDIAN
    return SWAP_BYTE_ORDER_WORD(val);
#endif /* BIG_ENDIAN */

#ifdef LITTLE_ENDIAN
    return val;
#endif /* LITTLE_ENDIAN */
}

INLINE void hex_dump(UWORD8 *info, UWORD8 *str, UWORD32 len)
{
    if(str == NULL || len == 0)
        return;

    if(1)
    {
		UWORD32  i = 0;
		LOGD("dump %s, len: %d; data:\n",info,len);
		for(i = 0; i<len; i++)
		{
			if(((UWORD8 *)str+i)==NULL)
				break;
			LOGD("%02x ",*((UWORD8 *)str+i));
			if((i+1)%16 == 0)
				LOGD("\n");
		}
		LOGD("\n");
	}
}

static char *down_bt_strstrip(char *s)
{
    size_t size;

    size = strlen(s);
    if (!size)
        return s;

    while (*s && down_bt_is_space(*s))
        s++;

    return s;
}

// find first comma(char ',') then return the next comma'next space ptr.
static char *down_bt_skip_comma(char *s)
{
    size_t size;

    size = strlen(s);
    if (!size)
        return s;

    while(*s)
    {
        if(down_bt_is_comma(*s))
        {
            return ++s;
        }
        if(down_bt_is_endc(*s))
        {
            LOGD("end of buff, str=%s\n",s);
            return s;
        }
        s++;
    }

    return s;
}


static int count_g_u32 = 0;
static int count_g_u16 = 0;
static int count_g_u8 = 0;

//#define get_one_item(buff)  (simple_strtoul(buff, NULL, 0))
#define get_one_item(buff)  (strtoul(buff, NULL, 16))

static int get_one_digit(UWORD8 *ptr, UWORD8 *data)
{
    int count = 0;

    //hex_dump("get_one_digit 10 ptr",ptr,10);
    if(*ptr=='0' && (*(ptr+1)=='x' || *(ptr+1)=='X'))
    {
        memcpy(data, "0x", 2);
        ptr += 2;
        data += 2;

        while(1)
        {
            if((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'a' && *ptr <= 'f') || (*ptr >= 'A' && *ptr <= 'F'))
            {
                *data = *ptr;
                LOGD("char:%c, %c\n", *ptr, *data);
                data++;
                ptr++;
                count++;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        while(1)
        {
            if(*ptr >= '0' && *ptr <= '9')
            {
                *data = *ptr;
                LOGD("char:%c, %c\n", *ptr, *data);
                data++;
                ptr++;
                count++;
            }
            else
            {
                break;
            }
        }
    }

    return count;
}

static UWORD32 get_next_u32(UWORD8 **p_buff)
{
    UWORD32  val_u32  = 0;
    UWORD8 *ptr = 0;
    UWORD8 data[10];
    UWORD32 data_len = 0;
    ++count_g_u32;

    if(p_buff == NULL || *p_buff == NULL)
    {
        if(p_buff)
            LOGD("%s: Error occur! *p_buff == null!\n",__FUNCTION__);
        else
            LOGD("%s: Error occur! p_buff == null!\n",__FUNCTION__);
        return 0;
    }
    ptr = *p_buff;

    ptr = down_bt_strstrip(ptr);

    memset(data,'\0',sizeof(data));
    data_len = get_one_digit(ptr, data);


    LOGD("get_one_item(data) = 0x%x\n",(UWORD32)get_one_item(data));
    val_u32 = get_one_item(data);


    LOGD("%s: here:%x\n",__FUNCTION__,val_u32);

    ptr = down_bt_skip_comma(ptr);
    *p_buff = ptr;

    return val_u32;
}

static UWORD16 get_next_u16(UWORD8 **p_buff)
{
    UWORD32  val_u16  = 0;
    UWORD8 *ptr = 0;
    UWORD8 data[10];
    UWORD32 data_len = 0;
    ++count_g_u16;

    if(p_buff == NULL || *p_buff == NULL)
    {
        if(p_buff)
            LOGD("%s: Error occur! *p_buff == null!\n",__FUNCTION__);
        else
            LOGD("%s: Error occur! p_buff == null!\n",__FUNCTION__);
        return 0;
    }
    ptr = *p_buff;

    ptr = down_bt_strstrip(ptr);

    //LOGD("before parser:%s;\n",ptr);

    memset(data,'\0',sizeof(data));
    data_len = get_one_digit(ptr, data);

    //hex_dump("get_one_digit ok", data, data_len);
/*
    if(!data_len)
    {
        LOGD("get_one_digit error! ptr=%s\n",ptr);
        BUG_ON(1);
    }
    */

    LOGD("get_one_item(data) = 0x%x\n",(UWORD16)get_one_item(data));
    val_u16 = get_one_item(data);

    LOGD("%s: here:%x\n",__FUNCTION__,val_u16);

    ptr = down_bt_skip_comma(ptr);
    *p_buff = ptr;
    //LOGD("after parser:%s;\n",ptr);

    return val_u16;
}


static UWORD8 get_next_u8(UWORD8 **p_buff)
{
    UWORD32  val_u8  = 0;
    UWORD8 *ptr = 0;
    UWORD8 data[10];
    UWORD32 data_len = 0;
    ++count_g_u8;

    if(p_buff == NULL || *p_buff == NULL)
    {
        if(p_buff)
            LOGD("%s: Error occur! *p_buff == null!\n",__FUNCTION__);
        else
            LOGD("%s: Error occur! p_buff == null!\n",__FUNCTION__);
        return 0;
    }
    ptr = *p_buff;

    ptr = down_bt_strstrip(ptr);

    //LOGD("before parser:%s;\n",ptr);

    memset(data,'\0',sizeof(data));
    data_len = get_one_digit(ptr, data);

    //hex_dump("get_one_digit ok", data, data_len);
    /*
    if(!data_len)
    {
        LOGD("get_one_digit error! ptr=%s\n",ptr);
        BUG_ON(1);
    }
    */

    LOGD("get_one_item(data) = 0x%x\n",(UWORD8)get_one_item(data));
    val_u8 = get_one_item(data);

    LOGD("%s: here:%x\n",__FUNCTION__,val_u8);

    ptr = down_bt_skip_comma(ptr);
    *p_buff = ptr;
    //LOGD("after parser:%s;\n",ptr);

    return val_u8;
}

int parser_pskey_info(UWORD8 *buff, BT_PSKEY_CONFIG_T *p_params)
{
    UWORD8 *tmp_buff = buff;
    int i = 0;
    LOGD("%s",__FUNCTION__);
    tmp_buff = strstr(tmp_buff, "}");
    //LOGD("read 1 this: %s", tmp_buff);
    tmp_buff++;
    tmp_buff = strstr(tmp_buff, "{");
    //LOGD("read 2 this: %s", tmp_buff);
    tmp_buff++;

    p_params->g_dbg_source_sink_syn_test_data = get_next_u8(&tmp_buff);
    p_params->g_sys_sleep_in_standby_supported = get_next_u8(&tmp_buff);
    p_params->g_sys_sleep_master_supported = get_next_u8(&tmp_buff);
    p_params->g_sys_sleep_slave_supported = get_next_u8(&tmp_buff);

    p_params->default_ahb_clk = get_next_u32(&tmp_buff);
    p_params->device_class = get_next_u32(&tmp_buff);
    p_params->win_ext = get_next_u32(&tmp_buff);

    for(i=0; i<6; i++)
    {
        p_params->g_aGainValue[i] = get_next_u32(&tmp_buff);
    }

    for(i=0; i<5; i++)
    {
        p_params->g_aPowerValue[i] = get_next_u32(&tmp_buff);
    }

    for(i=0; i<16; i++)
    {
        p_params->feature_set[i] = get_next_u8(&tmp_buff);
    }

    for(i=0; i<6; i++)
    {
        p_params->device_addr[i] = get_next_u8(&tmp_buff);
    }

    p_params->g_sys_sco_transmit_mode = get_next_u8(&tmp_buff);
    p_params->g_sys_uart0_communication_supported = get_next_u8(&tmp_buff);
    p_params->edr_tx_edr_delay = get_next_u8(&tmp_buff);
    p_params->edr_rx_edr_delay = get_next_u8(&tmp_buff);
    p_params->abcsp_rxcrc_supported = get_next_u8(&tmp_buff);
    p_params->abcsp_txcrc_supported = get_next_u8(&tmp_buff);
    p_params->share_memo_rx_base_addr = get_next_u32(&tmp_buff);
    p_params->share_memo_tx_base_addr = get_next_u32(&tmp_buff);
    p_params->share_memo_tx_packet_num_addr = get_next_u32(&tmp_buff);
    p_params->share_memo_tx_data_base_addr = get_next_u32(&tmp_buff);
    p_params->g_PrintLevel = get_next_u32(&tmp_buff);
    p_params->share_memo_tx_block_length = get_next_u16(&tmp_buff);
    p_params->share_memo_rx_block_length = get_next_u16(&tmp_buff);
    p_params->share_memo_tx_water_mark = get_next_u16(&tmp_buff);
    p_params->share_memo_tx_timeout_value = get_next_u16(&tmp_buff);
    p_params->uart_rx_watermark = get_next_u16(&tmp_buff);
    p_params->uart_flow_control_thld = get_next_u16(&tmp_buff);
    p_params->comp_id = get_next_u32(&tmp_buff);
    p_params->uart_clk_divd = get_next_u16(&tmp_buff);
    p_params->pcm_clk_divd = get_next_u16(&tmp_buff);

    p_params->debug_bb_tx_gain = get_next_u16(&tmp_buff);
    p_params->debug_tx_power = get_next_u16(&tmp_buff);
    p_params->tx_channel_compensate = get_next_u32(&tmp_buff);
    p_params->ctrl_reserved = get_next_u16(&tmp_buff);
    p_params->reserved16= get_next_u16(&tmp_buff);

    for(i=0; i<10; i++)
    {
        p_params->reserved[i] = get_next_u32(&tmp_buff);
    }

    LOGD("leave out tmp_buff=%s\n", tmp_buff);
    //hex_dump("p_params",(void *)p_params,sizeof(BT_PSKEY_CONFIG_T));

    return 0;
}


int get_pskey_from_file(BT_PSKEY_CONFIG_T *pskey_file)
{
    int bt_flag=0;
    char* bt_ptr = NULL;
    int bt_pskey_len = 0;
    unsigned char *tmp_buff = NULL;
    int ret = 0;
    int fd  = 0;
    int sz  = 0;

    LOGD("%s",__FUNCTION__);

    tmp_buff = (unsigned char *)malloc(MAX_BT_TMP_PSKEY_FILE_LEN+1);
	if(!tmp_buff)
	{
	    LOGD("%s: unable to alloc tmp_buff! \n", __FUNCTION__);
        return -1;
    }

    memset(tmp_buff, 0x0, MAX_BT_TMP_PSKEY_FILE_LEN+1);

    // read pskey_bt.txt from file
    fd = open(BT_PSKEY_STRUCT_FILE, O_RDONLY, 0644);
    LOGD("open bt pskey structure file fd=%d", fd);
	if(fd > 0) {
		sz = read(fd, tmp_buff, MAX_BT_TMP_PSKEY_FILE_LEN);
        LOGD("buf from pskey file:%d", sz);
        //LOGD("buf = %s", tmp_buff);
		close(fd);
	}
    else {
        LOGD("open BT_PSKEY_STRUCT_FILE fail");
        return -1;
    }

    // parser file struct
    ret = parser_pskey_info(tmp_buff, pskey_file);
    if(ret < 0)
    {
        LOGD("parse pskey info fail");
        goto parse_pskey_error;
    }

    LOGD("parse pskey success, tmp_buff=%p", tmp_buff);

    // free tmp_buff
    if(tmp_buff)
        free(tmp_buff);
    tmp_buff = NULL;

    return 0;

parse_pskey_error:
    if(tmp_buff){
        free(tmp_buff);
        tmp_buff = NULL;
    }

    return -1;
}







