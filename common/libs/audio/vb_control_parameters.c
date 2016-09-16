#include <utils/Log.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "aud_enha.h"

//#ifdef __cplusplus
//extern "c"
//{
//#endif


//#define MY_DEBUG

#ifdef MY_DEBUG
#define MY_TRACE    ALOGW
#else
#define MY_TRACE(...)
#endif

#define VBC_CMD_TAG   "VBC"

#define READ_PARAS(type, exp)    if (s_vbpipe_fd > 0 && paras_ptr != NULL) { \
        exp = read(s_vbpipe_fd, paras_ptr, sizeof(type)); \
        }

#define ENG_AUDIO_PGA       "/sys/class/vbc_param_config/vbc_pga_store"

static uint32_t android_cur_device = 0x0;   // devices value the same as audiosystem.h
static pthread_t s_vbc_ctrl_thread = 0;
static int s_vbpipe_fd = -1;
static int s_is_exit = 0;
static int s_is_active = 0;
static int android_sim_num = 0;


/* vbc control parameters struct here.*/
typedef struct Paras_Mode_Gain
{
    unsigned short  is_mode;
    unsigned short  is_volume;

    unsigned short  mode_index;
    unsigned short  volume_index;

    unsigned short  dac_set;
    unsigned short  adc_set;

    unsigned short  dac_gain;
    unsigned short  adc_gain;

    unsigned short  path_set;
    unsigned short  pa_setting;

    unsigned short  reserved[16];
}paras_mode_gain_t;

typedef struct Switch_ctrl
{
    unsigned int  is_switch; /* switch vbc contrl to dsp.*/
}switch_ctrl_t;

typedef struct Set_Mute
{
    unsigned int  is_mute;
}set_mute_t;

typedef struct Device_ctrl
{
    unsigned short  	is_open; /* if is_open is true, open device; else close device.*/
    unsigned short  	is_headphone;
	unsigned int 		is_downlink_mute;
	unsigned int 		is_uplink_mute;
	paras_mode_gain_t 	paras_mode;
}device_ctrl_t;

typedef struct Open_hal
{
    unsigned int  sim_card;   /*sim card number*/
}open_hal_t;


typedef struct {
	unsigned short adc_pga_gain_l;
	unsigned short adc_pga_gain_r;
    uint32_t fm_pga_gain_l;
    uint32_t fm_pga_gain_r;
	uint32_t dac_pga_gain_l;
	uint32_t dac_pga_gain_r;
	uint32_t devices;
	uint32_t mode;
}pga_gain_nv_t;

/* list vbc cmds */
enum VBC_CMD_E
{
    VBC_CMD_NONE = 0,
/* current mode and volume gain parameters.*/
    VBC_CMD_SET_MODE = 1,
    VBC_CMD_RSP_MODE = 2,
    VBC_CMD_SET_GAIN = 3,
    VBC_CMD_RSP_GAIN = 4,
/* whether switch vb control to dsp parameters.*/
    VBC_CMD_SWITCH_CTRL = 5,
    VBC_CMD_RSP_SWITCH = 6,
/* whether mute or not.*/
    VBC_CMD_SET_MUTE = 7,
    VBC_CMD_RSP_MUTE = 8,
/* open/close device parameters.*/
    VBC_CMD_DEVICE_CTRL = 9,
    VBC_CMD_RSP_DEVICE = 10,

	VBC_CMD_HAL_OPEN = 11,
	VBC_CMD_RSP_OPEN  =12,

	VBC_CMD_HAL_CLOSE = 13,
	VBC_CMD_RSP_CLOSE = 14,

    VBC_CMD_MAX
};

typedef struct
{
    char        tag[4];   /* "VBC" */
    unsigned int    cmd_type;
    unsigned int    paras_size; /* the size of Parameters Data, unit: bytes*/
}parameters_head_t;

/* Transfer packet by vbpipe, packet format as follows.*/
/************************************************
----------------------------
|Paras Head |  Paras Data  |
----------------------------
************************************************/

/*
 * local functions declaration.
 */
static int  ReadParas_Head(int fd_pipe,  parameters_head_t *head_ptr);

static int  WriteParas_Head(int fd_pipe,  parameters_head_t *head_ptr);

static int  ReadParas_DeviceCtrl(int fd_pipe, device_ctrl_t *paras_ptr);

static int  ReadParas_ModeGain(int fd_pipe,  paras_mode_gain_t *paras_ptr);

static int  ReadParas_SwitchCtrl(int fd_pipe,  switch_ctrl_t *paras_ptr);

static int  ReadParas_Mute(int fd_pipe,  set_mute_t *paras_ptr);

void *vbc_ctrl_thread_routine(void *args);

/*
 * local functions definition.
 */
static int  ReadParas_Head(int fd_pipe,  parameters_head_t *head_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && head_ptr != NULL) {
        ret = read(fd_pipe, head_ptr, sizeof(parameters_head_t));
    }
    return ret;
}

static int  WriteParas_Head(int fd_pipe,  parameters_head_t *head_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && head_ptr != NULL) {
        ret = write(fd_pipe, head_ptr, sizeof(parameters_head_t));
    }
    return ret;
}

static int  ReadParas_OpenHal(int fd_pipe, open_hal_t *hal_open_param)
{
    int ret = 0;
    if (fd_pipe > 0 && hal_open_param != NULL) {
        ret = read(fd_pipe, hal_open_param, sizeof(open_hal_t));
    }
    return ret;
}


static int  ReadParas_DeviceCtrl(int fd_pipe, device_ctrl_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read(fd_pipe, paras_ptr, sizeof(device_ctrl_t));
    }
    return ret;
}

static int  ReadParas_ModeGain(int fd_pipe,  paras_mode_gain_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read(fd_pipe, paras_ptr, sizeof(paras_mode_gain_t));
    }
    return ret;
}

static int  ReadParas_SwitchCtrl(int fd_pipe,  switch_ctrl_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read(fd_pipe, paras_ptr, sizeof(switch_ctrl_t));
    }
    return ret;
}

static int  ReadParas_Mute(int fd_pipe,  set_mute_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read(fd_pipe, paras_ptr, sizeof(set_mute_t));
    }
    return ret;
}

int Write_Rsp2cp(int fd_pipe, unsigned int cmd)
{
	int ret = 0;
	int count = 0;
	parameters_head_t write_common_head;
    memset(&write_common_head, 0, sizeof(parameters_head_t));
	memcpy(&write_common_head.tag[0], VBC_CMD_TAG, 3);
    write_common_head.cmd_type = cmd+1;
    write_common_head.paras_size = 0;
	if(fd_pipe < 0){
		ALOGE("%s vbpipe has not open...",__func__);
		return -1;
	}
	WriteParas_Head(fd_pipe, &write_common_head);
	MY_TRACE("%s: send  cmd(%d) to cp .",__func__,write_common_head.cmd_type);
	return 0;
}

unsigned short GetCall_Cur_Device()
{
    return android_cur_device;
}

static int32_t GetAudio_mode_number_from_device(struct tiny_audio_device *adev)
{
    int32_t lmode;
    if(((adev->devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) && (adev->devices & AUDIO_DEVICE_OUT_SPEAKER))
			|| ((adev->devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) && (adev->devices & AUDIO_DEVICE_OUT_SPEAKER))){
		lmode = 1;  //headfree
	}else if(adev->devices & AUDIO_DEVICE_OUT_EARPIECE){
		lmode = 2;  //handset
	}else if((adev->devices & AUDIO_DEVICE_OUT_SPEAKER) || (adev->devices & AUDIO_DEVICE_OUT_FM_SPEAKER)){
		lmode = 3;  //handsfree
	}else if((adev->devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (adev->devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) 
	        || (adev->devices & AUDIO_DEVICE_OUT_FM_HEADSET) || (adev->devices & AUDIO_DEVICE_IN_WIRED_HEADSET)){
		lmode = 0;  //headset
    }else{
		ALOGW("%s device(0x%x) is not support, set default:handsfree \n",__func__,adev->devices);
        lmode = 3;
	}
    return lmode;
}

static int GetAudio_fd_from_nv()
{
    int fd = -1;
    off_t offset = 0;

    fd = open(ENG_AUDIO_PARA_DEBUG, O_RDONLY);
    if (-1 == fd) {
        ALOGW("%s, file %s open failed:%s\n",__func__,ENG_AUDIO_PARA_DEBUG,strerror(errno));
        fd = open(ENG_AUDIO_PARA,O_RDONLY);
        if(-1 == fd){
            ALOGE("%s, file %s open error:%s\n",__func__,ENG_AUDIO_PARA,strerror(errno));
            return -1;
        }
    }else{
    //check the size of /data/local/tmp/audio_para
        offset = lseek(fd,-1,SEEK_END);
        if((offset+1) != 4*sizeof(AUDIO_TOTAL_T)){
            ALOGE("%s, file %s size (%d) error \n",__func__,ENG_AUDIO_PARA_DEBUG,offset+1);
            close(fd);
            fd = open(ENG_AUDIO_PARA,O_RDONLY);
            if(-1 == fd){
                ALOGE("%s, file %s open error:%s\n",__func__,ENG_AUDIO_PARA,strerror(errno));
                return -1;
            }
        }
    }
    return fd;
}

static int  GetAudio_pga_nv(struct tiny_audio_device *adev, AUDIO_TOTAL_T *aud_params_ptr, pga_gain_nv_t *pga_gain_nv, uint32_t vol_level)
{
    if((NULL == aud_params_ptr) || (NULL == pga_gain_nv)){
        ALOGE("%s aud_params_ptr or pga_gain_nv is NULL",__func__);
        return -1;
    }
    pga_gain_nv->adc_pga_gain_l = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_CAPTURE_GAIN_INDEX];    //43
    pga_gain_nv->adc_pga_gain_r = pga_gain_nv->adc_pga_gain_l;
    
    pga_gain_nv->dac_pga_gain_l = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[0].arm_volume[vol_level];
    pga_gain_nv->dac_pga_gain_r = pga_gain_nv->dac_pga_gain_l;
    
    pga_gain_nv->fm_pga_gain_l  = (aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_FM_GAINL_INDEX]
        | ((aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_FM_DGAIN_INDEX]<<16) & 0xffff0000));  //18,19
    pga_gain_nv->fm_pga_gain_r  = pga_gain_nv->fm_pga_gain_l;

    pga_gain_nv->devices = adev->devices;

    ALOGW("%s, dac_pga_gain_l:0x%x adc_pga_gain_l:0x%x fm_pga_gain_l:0x%x fm_pga_gain_r:0x%x device:0x%x vol_level:0x%x ",
        __func__,pga_gain_nv->dac_pga_gain_l,pga_gain_nv->adc_pga_gain_l,pga_gain_nv->fm_pga_gain_l,pga_gain_nv->fm_pga_gain_r,pga_gain_nv->devices,vol_level);
    return 0;
}

static int GetAudio_gain_by_devices(struct tiny_audio_device *adev, pga_gain_nv_t *pga_gain_nv, uint32_t vol_level)
{
    int ret = 0;
    int fd = -1;
    int32_t lmode = 0;
    AUDIO_TOTAL_T * aud_params_ptr = NULL;
    char * dev_name = NULL;
    lmode = GetAudio_mode_number_from_device(adev);
    fd = GetAudio_fd_from_nv();
    if(fd < 0) {
        ALOGE("%s, get audio fd(%d) error ",__func__,fd);
        return -1;
    }
    aud_params_ptr = (AUDIO_TOTAL_T *)mmap(0, 4*sizeof(AUDIO_TOTAL_T),PROT_READ,MAP_SHARED,fd,0);
    if ( NULL == aud_params_ptr ) {
        ALOGE("%s, mmap failed %s",__func__,strerror(errno));
        close(fd);
        return -1;
    }
    //get music gain from nv
    ret = GetAudio_pga_nv(adev, &aud_params_ptr[lmode], pga_gain_nv, vol_level);
    if(ret < 0){
        munmap((void *)aud_params_ptr, 4*sizeof(AUDIO_TOTAL_T));
        close(fd);
        return -1;
    }
    //close fd
    munmap((void *)aud_params_ptr, 4*sizeof(AUDIO_TOTAL_T));
    close(fd);
    return 0;
}

static int SetAudio_gain_by_devices(struct tiny_audio_device *adev, pga_gain_nv_t *pga_gain_nv)
{
    if(NULL == pga_gain_nv){
        ALOGE("%s pga_gain_nv NULL",__func__);
        return -1;
    }
    if(pga_gain_nv->devices & AUDIO_DEVICE_OUT_EARPIECE){
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"earpiece");
    }
    if((pga_gain_nv->devices & AUDIO_DEVICE_OUT_SPEAKER) && ((pga_gain_nv->devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE))){
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"headphone-spk-l");
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"headphone-spk-r");
    }else{
        if(pga_gain_nv->devices & AUDIO_DEVICE_OUT_SPEAKER){
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"speaker-l");
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"speaker-r");
        }
        if((pga_gain_nv->devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)){
    	    audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"headphone-l");
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"headphone-r");
        }
    }
    if(pga_gain_nv->devices & AUDIO_DEVICE_OUT_FM_HEADSET){
        audio_pga_apply(adev->pga,pga_gain_nv->fm_pga_gain_l,"linein-hp-l");
        audio_pga_apply(adev->pga,pga_gain_nv->fm_pga_gain_r,"linein-hp-r");
    }else if(pga_gain_nv->devices & AUDIO_DEVICE_OUT_FM_SPEAKER){
        audio_pga_apply(adev->pga,pga_gain_nv->fm_pga_gain_l,"linein-spk-l");
        audio_pga_apply(adev->pga,pga_gain_nv->fm_pga_gain_r,"linein-spk-r");
    }else if((pga_gain_nv->devices & AUDIO_DEVICE_IN_BUILTIN_MIC) || (pga_gain_nv->devices & AUDIO_DEVICE_IN_BACK_MIC) || (pga_gain_nv->devices & AUDIO_DEVICE_IN_WIRED_HEADSET)){
        audio_pga_apply(adev->pga,pga_gain_nv->adc_pga_gain_l,"capture-l");
	    audio_pga_apply(adev->pga,pga_gain_nv->adc_pga_gain_r,"capture-r");
    }
    ALOGW("%s out, devices:0x%x ",__func__,pga_gain_nv->devices);
    return 0;
}

static void SetAudio_gain_route(struct tiny_audio_device *adev, uint32_t vol_level)
{
    int ret = 0;
    pga_gain_nv_t pga_gain_nv;
    memset(&pga_gain_nv,0,sizeof(pga_gain_nv_t));
    ret = GetAudio_gain_by_devices(adev,&pga_gain_nv,vol_level);
    if(ret < 0){
        return;
    }
    ret = SetAudio_gain_by_devices(adev,&pga_gain_nv);
    if(ret < 0){
        return;
    }    
}

static void SetCall_ModePara(struct tiny_audio_device *adev,paras_mode_gain_t *mode_gain_paras)
{
    int i = 0;
	unsigned short switch_earpice = 0;
	unsigned short switch_headset = 0;
	unsigned short switch_speaker = 0;
	unsigned short switch_mic0 = 0;
	unsigned short switch_mic1 = 0;
	unsigned short switch_hp_mic = 0;
    unsigned short switch_table[5] = {0};
    uint32_t switch_device[] = {AUDIO_DEVICE_OUT_EARPIECE,AUDIO_DEVICE_OUT_SPEAKER,AUDIO_DEVICE_IN_BUILTIN_MIC,AUDIO_DEVICE_IN_WIRED_HEADSET,AUDIO_DEVICE_OUT_WIRED_HEADSET};

	MY_TRACE("%s path_set:0x%x .android_cur_device:0x%x ",__func__,mode_gain_paras->path_set,android_cur_device);
	switch_earpice = (mode_gain_paras->path_set & 0x0040)>>6;
	switch_headset = mode_gain_paras->path_set & 0x0001;
	switch_speaker = (mode_gain_paras->path_set & 0x0008)>>3;
	switch_mic0 = (mode_gain_paras->path_set & 0x0400)>>10;
	switch_mic1 = (mode_gain_paras->path_set & 0x0800)>>11;
	switch_hp_mic = (mode_gain_paras->path_set & 0x1000)>>12;

    switch_table[0] = switch_earpice;
    switch_table[1] = switch_speaker;
    switch_table[2] = switch_mic0;
    switch_table[3] = switch_hp_mic;
    switch_table[4] = switch_headset;

//At present, switch of pa cannot handle mulit-device
    android_cur_device = 0;
    if(switch_earpice){
        android_cur_device |= AUDIO_DEVICE_OUT_EARPIECE;
    }
    if(switch_speaker){
        android_cur_device |= AUDIO_DEVICE_OUT_SPEAKER;
    }
    if(switch_headset){
        android_cur_device |= AUDIO_DEVICE_OUT_WIRED_HEADSET;
    }
    if(switch_mic0 | switch_mic1){
        android_cur_device |= AUDIO_DEVICE_IN_BUILTIN_MIC;
    }
    if(switch_hp_mic){
        android_cur_device |= AUDIO_DEVICE_IN_WIRED_HEADSET;
    }

	for(i=0; i<(sizeof(switch_table)/sizeof(unsigned short));i++)
    {
        if(switch_table[i]){
            set_call_route(adev,switch_device[i],1);
        }
    }
    for(i=0; i<(sizeof(switch_table)/sizeof(unsigned short));i++)
    {
        if(!switch_table[i]){
        #ifdef _DSP_CTRL_CODEC      //if dsp control codec, we cann't close headset.
            if(i == 4 || i == 3){
                continue;
            }
        #endif
            set_call_route(adev,switch_device[i],0);
        }
    }
    /*
        we need to wait for codec here before call connected, maybe driver needs to fix this problem.
    */
    if(!adev->call_connected){
        usleep(1000*100);
    }
	ALOGW("%s successfully, device: earpice(%s), headphone(%s), speaker(%s), Mic(%s), hp_mic(%s) devices(0x%x)"
				,__func__,switch_earpice ? "Open":"Close",switch_headset ? "Open":"Close",switch_speaker ? "Open":"Close",
				switch_mic0 ? "Open":"Close",switch_hp_mic ? "Open":"Close",android_cur_device);
}

static void SetCall_VolumePara(struct tiny_audio_device *adev,paras_mode_gain_t *mode_gain_paras)
{
	int ret = 0;
	pga_gain_nv_t pga_gain_nv;
	memset(&pga_gain_nv,0,sizeof(pga_gain_nv_t));
	if(NULL == mode_gain_paras){
		ret = -1;
		ALOGE("%s mode paras is NULL!!",__func__);
		return;
	}
	pga_gain_nv.devices = android_cur_device;
	pga_gain_nv.mode = adev->mode;
	pga_gain_nv.adc_pga_gain_l= mode_gain_paras->adc_gain & 0x00ff;
	pga_gain_nv.adc_pga_gain_r= (mode_gain_paras->adc_gain & 0xff00) >> 8;
	pga_gain_nv.dac_pga_gain_l= mode_gain_paras->dac_gain & 0x000000ff;
	pga_gain_nv.dac_pga_gain_r= (mode_gain_paras->dac_gain & 0x0000ff00) >> 8;

	ret = SetAudio_gain_by_devices(adev,&pga_gain_nv);
    if(ret < 0){
        return;
    }
	ALOGW("%s successfully ,dac_pga_gain_l:0x%x ,dac_pga_gain_r:0x%x ,adc_pga_gain_l:0x%x ,adc_pga_gain_r:0x%x ,devices:0x%x ,mode:%d ",
		__func__,pga_gain_nv.dac_pga_gain_l,pga_gain_nv.dac_pga_gain_r,pga_gain_nv.adc_pga_gain_l,pga_gain_nv.adc_pga_gain_r,pga_gain_nv.devices,adev->mode);
}

int SetParas_OpenHal_Incall(int fd_pipe)	//Get open hal cmd and sim card
{
    int ret = 0;
    open_hal_t hal_open_param;
    parameters_head_t read_common_head;
    memset(&hal_open_param,0,sizeof(open_hal_t));
    memset(&read_common_head, 0, sizeof(parameters_head_t));
    MY_TRACE("%s in...",__func__);

    ret = Write_Rsp2cp(fd_pipe,VBC_CMD_HAL_OPEN);
    if(ret < 0){
        ALOGE("Error, %s Write_Rsp2cp failed(%d).",__func__,ret);
    }
    ret = ReadParas_OpenHal(fd_pipe,&hal_open_param);
    if (ret <= 0) {
        ALOGE("Error, read %s failed(%d).",__func__,ret);
    }
    ret = Write_Rsp2cp(fd_pipe,VBC_CMD_HAL_OPEN);
    if(ret < 0){
        ALOGE("Error, %s Write_Rsp2cp failed(%d).",__func__,ret);
    }
    android_sim_num = hal_open_param.sim_card;
    MY_TRACE("%s successfully,sim card number(%d)",__func__,android_sim_num);
    return ret;
}

int GetParas_DeviceCtrl_Incall(int fd_pipe,device_ctrl_t *device_ctrl_param)	//open,close
{
	int ret = 0;
	MY_TRACE("%s in... ",__func__);
	ret = Write_Rsp2cp(fd_pipe,VBC_CMD_DEVICE_CTRL);
	if(ret < 0){
		ALOGE("Error, %s Write_Rsp2cp failed(%d).",__func__,ret);
	}
	ret = ReadParas_DeviceCtrl(fd_pipe,device_ctrl_param);
	if (ret <= 0) {
		ALOGE("Error, read %s failed(%d).",__func__,ret);
	}
	if((!device_ctrl_param->paras_mode.is_mode) || (!device_ctrl_param->paras_mode.is_volume)){	//check whether is setDevMode
		ret =-1;
		ALOGE("Error: %s,ReadParas_DeviceCtrl wrong cmd_type.",__func__);
		return ret;
	}
	MY_TRACE("%s successfully ,is_open(%d) is_headphone(%d) is_downlink_mute(%d) is_uplink_mute(%d) volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x) ",__func__,device_ctrl_param->is_open,device_ctrl_param->is_headphone, \
		device_ctrl_param->is_downlink_mute,device_ctrl_param->is_uplink_mute,device_ctrl_param->paras_mode.volume_index,device_ctrl_param->paras_mode.adc_gain,device_ctrl_param->paras_mode.path_set,device_ctrl_param->paras_mode.dac_gain,device_ctrl_param->paras_mode.pa_setting);
	return ret;
}

int GetParas_Route_Incall(int fd_pipe,paras_mode_gain_t *mode_gain_paras)	//set_volume & set_route
{
	int ret = 0;
	parameters_head_t read_common_head;
	memset(&read_common_head, 0, sizeof(parameters_head_t));
	MY_TRACE("%s in...",__func__);

	ret = Write_Rsp2cp(fd_pipe,VBC_CMD_SET_MODE);
	if(ret < 0){
		ALOGE("Error, %s Write_Rsp2cp failed(%d).",__func__,ret);
	}
    ret = ReadParas_ModeGain(fd_pipe,mode_gain_paras);
    if (ret <= 0) {
        ALOGE("Error, read %s failed(%d).",__func__,ret);
        return ret;
    }
	if((!mode_gain_paras->is_mode)){	//check whether is setDevMode
		ret =-1;
		ALOGE("Error: %s ReadParas_ModeGain wrong cmd_type.",__func__);
		return ret;
	}
	MY_TRACE("%s successfully,volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x)",__func__,mode_gain_paras->volume_index,mode_gain_paras->adc_gain, \
		mode_gain_paras->path_set, mode_gain_paras->dac_gain,mode_gain_paras->pa_setting);
	return ret;
}

int GetParas_Volume_Incall(int fd_pipe,paras_mode_gain_t *mode_gain_paras)	//set_volume & set_route
{
	int ret = 0;
	parameters_head_t read_common_head;
	memset(&read_common_head, 0, sizeof(parameters_head_t));
	MY_TRACE("%s in...",__func__);

	ret = Write_Rsp2cp(fd_pipe,VBC_CMD_SET_GAIN);
	if(ret < 0){
		ALOGE("Error, %s Write_Rsp2cp failed(%d).",__func__,ret);
	}
    ret = ReadParas_ModeGain(fd_pipe,mode_gain_paras);
    if (ret <= 0) {
        ALOGE("Error, read %s failed(%d).",__func__,ret);
    }
	if((!mode_gain_paras->is_volume)){	//check whether is setDevMode
		ret =-1;
		ALOGE("Error: %s ReadParas_ModeGain wrong cmd_type.",__func__);
		return ret;
	}
	MY_TRACE("%s successfully,volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x)",__func__,mode_gain_paras->volume_index,mode_gain_paras->adc_gain, \
		mode_gain_paras->path_set, mode_gain_paras->dac_gain, mode_gain_paras->pa_setting);
	return ret;
}

int GetParas_Switch_Incall(int fd_pipe,switch_ctrl_t *swtich_ctrl_paras)	/* switch vbc contrl to dsp.*/
{
	int ret = 0;
	parameters_head_t read_common_head;
	memset(&read_common_head, 0, sizeof(parameters_head_t));
	MY_TRACE("%s in...",__func__);

	ret = Write_Rsp2cp(fd_pipe,VBC_CMD_SWITCH_CTRL);
	if(ret < 0){
		ALOGE("Error, %s Write_Rsp2cp failed(%d)",__func__,ret);
	}
	ret = ReadParas_SwitchCtrl(fd_pipe,swtich_ctrl_paras);
	if (ret <= 0) {
	    ALOGE("Error, read ReadParas_SwitchCtrl failed(%d)",ret);
	}
	MY_TRACE("%s successfully ,is_switch(%d) ",__func__,swtich_ctrl_paras->is_switch);
	return ret;
}

int SetParas_Route_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
	int ret = 0;
	unsigned short switch_earpice = 0;
	unsigned short switch_headset = 0;
	unsigned short switch_speaker = 0;
	unsigned short switch_mic0 = 0;
	unsigned short switch_mic1 = 0;
	unsigned short switch_hp_mic = 0;
	paras_mode_gain_t mode_gain_paras;
	memset(&mode_gain_paras,0,sizeof(paras_mode_gain_t));
	MY_TRACE("%s in.....",__func__);
	ret = GetParas_Route_Incall(fd_pipe,&mode_gain_paras);
	if(ret < 0){
		return ret;
	}
	SetCall_ModePara(adev,&mode_gain_paras);
	SetCall_VolumePara(adev,&mode_gain_paras);
	Write_Rsp2cp(fd_pipe,VBC_CMD_SET_MODE);
	MY_TRACE("%s send rsp to cp...",__func__);
	return ret;
}

int SetParas_Volume_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
	int ret = 0;
	paras_mode_gain_t mode_gain_paras;
	memset(&mode_gain_paras,0,sizeof(paras_mode_gain_t));
	MY_TRACE("%s in.....",__func__);
	ret = GetParas_Volume_Incall(fd_pipe,&mode_gain_paras);
	if(ret < 0){
		return ret;
	}
	SetCall_VolumePara(adev,&mode_gain_paras);
	Write_Rsp2cp(fd_pipe,VBC_CMD_SET_GAIN);
	MY_TRACE("%s send rsp to cp...",__func__);
	return ret;
}

int SetParas_Switch_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
	int ret = 0;
	parameters_head_t write_common_head;
	switch_ctrl_t swtich_ctrl_paras;
	memset(&swtich_ctrl_paras,0,sizeof(swtich_ctrl_paras));
	MY_TRACE("%s in...",__func__);
	ret = GetParas_Switch_Incall(fd_pipe,&swtich_ctrl_paras);
	if(ret < 0){
		return ret;
	}
    mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, !swtich_ctrl_paras.is_switch);
	ALOGW("%s, VBC %s dsp...",__func__,(swtich_ctrl_paras.is_switch)?"Switch control to":"Get control back from");
	Write_Rsp2cp(fd_pipe,VBC_CMD_SWITCH_CTRL);
	MY_TRACE("%s send rsp to cp...",__func__);
	return ret;
}

int SetParas_DeviceCtrl_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
	int ret = 0;
	device_ctrl_t device_ctrl_paras;
	memset(&device_ctrl_paras,0,sizeof(device_ctrl_t));
	MY_TRACE("%s in.....",__func__);

	//because of codec,we should set headphone on first if codec is controlled by dsp
#ifdef _DSP_CTRL_CODEC
	set_call_route(adev, AUDIO_DEVICE_OUT_WIRED_HEADSET, 1);
#endif


	ret =GetParas_DeviceCtrl_Incall(fd_pipe,&device_ctrl_paras);
	if(ret < 0){
		return ret;
	}

	//set arm mode paras
	if(device_ctrl_paras.is_open){
		SetCall_ModePara(adev,&device_ctrl_paras.paras_mode);
		SetCall_VolumePara(adev,&device_ctrl_paras.paras_mode);
	}else{
		ALOGW("%s close device...",__func__);
	}
	Write_Rsp2cp(fd_pipe,VBC_CMD_DEVICE_CTRL);
	MY_TRACE("%s send rsp to cp...",__func__);
	return ret;
}

int vbc_ctrl_open(struct tiny_audio_device *adev)
{
    if (s_is_active) return (-1);
    int rc;
    MY_TRACE("%s IN.",__func__);
    s_is_exit = 0;
    s_is_active = 1;
    rc = pthread_create(&s_vbc_ctrl_thread, NULL, vbc_ctrl_thread_routine, (void *)adev);
    if (rc) {
        ALOGE("error, pthread_create failed, rc=%d", rc);
        s_is_active = 0;
        return (-1);
    }

    return (0);
}

int vbc_ctrl_close()
{
    if (!s_is_active) return (-1);
    MY_TRACE("%s IN.",__func__);
   
    s_is_exit = 1;
    s_is_active = 0;
    /* close vbpipe.*/
    close(s_vbpipe_fd);
    s_vbpipe_fd = -1;

    /* terminate thread.*/
    //pthread_cancel (s_vbc_ctrl_thread);    
    return (0);
}

void *vbc_ctrl_thread_routine(void *arg)
{
    int ret = 0;
    struct tiny_audio_device *adev;
    parameters_head_t read_common_head;
    parameters_head_t write_common_head;
    adev = (struct tiny_audio_device *)arg;

    memset(&read_common_head, 0, sizeof(parameters_head_t));
    memset(&write_common_head, 0, sizeof(parameters_head_t));
    
    memcpy(&write_common_head.tag[0], VBC_CMD_TAG, 3);
    write_common_head.cmd_type = VBC_CMD_NONE;
    write_common_head.paras_size = 0;
    MY_TRACE("vbc_ctrl_thread_routine in.");
    
RESTART:
    if (s_is_exit) goto EXIT;
    /* open vbpipe to build connection.*/
    if (s_vbpipe_fd == -1) {
        s_vbpipe_fd = open("/dev/vbpipe6", O_RDWR);
        if (s_vbpipe_fd < 0) {
            ALOGE("Error: s_vbpipe_fd(%d) open failed, %s ", s_vbpipe_fd,strerror(errno));
            if(adev->call_start){                  //cp crash during call
                mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, 1);  //switch to arm
                pthread_mutex_lock(&adev->lock);
                force_all_standby(adev);
                pcm_close(adev->pcm_modem_ul);
                pcm_close(adev->pcm_modem_dl);
                adev->call_start = 0;
                adev->call_connected = 0;
                pthread_mutex_unlock(&adev->lock);
            }
            sleep(1);
            goto RESTART;
        } else {
            ALOGW("s_vbpipe_fd(%d) open successfully.", s_vbpipe_fd);
        }
    } else {
        ALOGW("warning: s_vbpipe_fd(%d) NOT closed.", s_vbpipe_fd);
    }

    /* loop to read parameters from vbpipe.*/
    while(!s_is_exit)
    {
    	ALOGW("looping now...");
        /* read parameters common head of the packet.*/
        ret = ReadParas_Head(s_vbpipe_fd, &read_common_head);
        if (ret < 0) {
            ALOGE("Error, %s read head failed(%s), need to read again ",__func__,strerror(errno));
            continue;
        }else if (ret == 0) {   //cp something wrong
            ALOGE("Error, %s read head failed(%s), need to reopen vbpipe ",__func__,strerror(errno));
            if(adev->call_start){                  //cp crash during call
                mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, 1);  //switch to arm
                pthread_mutex_lock(&adev->lock);
                force_all_standby(adev);
                pcm_close(adev->pcm_modem_ul);
                pcm_close(adev->pcm_modem_dl);
                adev->call_start = 0;
                adev->call_connected = 0;
                pthread_mutex_unlock(&adev->lock);
            }
	        sleep(1);
            close(s_vbpipe_fd);
            s_vbpipe_fd = -1;
            goto RESTART;
        }
        ALOGW("%s call start, Get CMD(%d) from cp, paras_size:%d devices:0x%x mode:%d", adev->call_start ? "":"NOT",read_common_head.cmd_type,read_common_head.paras_size,adev->devices,adev->mode);
        if (!memcmp(&read_common_head.tag[0], VBC_CMD_TAG, 3)) {
            switch (read_common_head.cmd_type)
            {
            case VBC_CMD_HAL_OPEN:
            {
                MY_TRACE("VBC_CMD_HAL_OPEN IN.");
                ALOGW("VBC_CMD_HAL_OPEN, try lock");
                pthread_mutex_lock(&adev->lock);
                ALOGW("VBC_CMD_HAL_OPEN, got lock");
                force_all_standby(adev);    /*should standby because MODE_IN_CALL is later than call_start*/
                adev->pcm_modem_dl= pcm_open(s_tinycard, PORT_MODEM, PCM_OUT | PCM_MMAP, &pcm_config_vx);
                if (!pcm_is_ready(adev->pcm_modem_dl)) {
                    ALOGE("cannot open pcm_modem_dl : %s", pcm_get_error(adev->pcm_modem_dl));
                    pcm_close(adev->pcm_modem_dl);
                    s_is_exit = 1;
                }
                adev->pcm_modem_ul= pcm_open(s_tinycard, PORT_MODEM, PCM_IN, &pcm_config_vrec_vx);
                if (!pcm_is_ready(adev->pcm_modem_ul)) {
                    ALOGE("cannot open pcm_modem_ul : %s", pcm_get_error(adev->pcm_modem_ul));
                    pcm_close(adev->pcm_modem_ul);
                    pcm_close(adev->pcm_modem_dl);
                    s_is_exit = 1;
                }
                ALOGW("START CALL,open pcm device...");
                adev->call_start = 1;
                SetParas_OpenHal_Incall(s_vbpipe_fd);   //get sim card number
                pthread_mutex_unlock(&adev->lock);
                MY_TRACE("VBC_CMD_HAL_OPEN OUT.");
            }
            break;
            case VBC_CMD_HAL_CLOSE:
            {
                MY_TRACE("VBC_CMD_HAL_CLOSE IN.");
                adev->call_prestop = 1;
                write_common_head.cmd_type = VBC_CMD_RSP_CLOSE;     //ask cp to read vaudio data, "call_prestop" will stop to write pcm data again.
                WriteParas_Head(s_vbpipe_fd, &write_common_head);
                mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, 1);  //switch vbc to arm
                if(adev->call_start){  //if mediaserver crashed, audio will reopen, "call_start" value is 0, should bypass all the settings.
                    ALOGW("VBC_CMD_HAL_CLOSE, try lock");
                    pthread_mutex_lock(&adev->lock);
                    ALOGW("VBC_CMD_HAL_CLOSE, got lock");
                    force_all_standby(adev);
                    pcm_close(adev->pcm_modem_ul);
                    pcm_close(adev->pcm_modem_dl);
                    adev->call_start = 0;
                    adev->call_connected = 0;
                    ALOGW("END CALL,close pcm device & switch to arm...");
                    pthread_mutex_unlock(&adev->lock);
                }else{
                    ALOGW("VBC_CMD_HAL_CLOSE, call thread restart, we should stop call!!!");
                }
                ReadParas_Head(s_vbpipe_fd,&write_common_head);
                Write_Rsp2cp(s_vbpipe_fd,VBC_CMD_HAL_CLOSE);
                adev->call_prestop = 0;
                MY_TRACE("VBC_CMD_HAL_CLOSE OUT.");
            }
            break;
            case VBC_CMD_SET_MODE:
            {
                MY_TRACE("VBC_CMD_SET_MODE IN.");
                ret = SetParas_Route_Incall(s_vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_SET_MODE SetParas_Route_Incall error.s_is_exit:%d ",s_is_exit);
                    s_is_exit = 1;
                }
                MY_TRACE("VBC_CMD_SET_MODE OUT.");
            }
            break;
            case VBC_CMD_SET_GAIN:
            {
                MY_TRACE("VBC_CMD_SET_GAIN IN.");
                ret = SetParas_Volume_Incall(s_vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_SET_GAIN SetParas_Route_Incall error.s_is_exit:%d ",s_is_exit);
                    s_is_exit = 1;
                }
                MY_TRACE("VBC_CMD_SET_GAIN OUT.");
            }
            break;
            case VBC_CMD_SWITCH_CTRL:
            {
                MY_TRACE("VBC_CMD_SWITCH_CTRL IN.");
                ret = SetParas_Switch_Incall(s_vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_SWITCH_CTRL SetParas_Switch_Incall error.s_is_exit:%d ",s_is_exit);
                    s_is_exit = 1;
                }
                pthread_mutex_lock(&adev->lock);
                adev->call_connected = 1;
                pthread_mutex_unlock(&adev->lock);
                MY_TRACE("VBC_CMD_SWITCH_CTRL OUT.");
            }
            break;
            case VBC_CMD_SET_MUTE:
            {
                MY_TRACE("VBC_CMD_SET_MUTE IN.");

                MY_TRACE("VBC_CMD_SET_MUTE OUT.");
            }
            break;
            case VBC_CMD_DEVICE_CTRL:
            {
                MY_TRACE("VBC_CMD_DEVICE_CTRL IN.");
                ret = SetParas_DeviceCtrl_Incall(s_vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_DEVICE_CTRL SetParas_DeviceCtrl_Incall error.s_is_exit:%d ",s_is_exit);
                    s_is_exit = 1;
                }
                MY_TRACE("VBC_CMD_DEVICE_CTRL OUT.");
            }
            break;
            default:
                ALOGE("Error: %s wrong cmd_type(%d)",__func__,read_common_head.cmd_type);
            break;
            }
        } else {
            ALOGE("Error, (0x%x)NOT match VBC_CMD_TAG, wrong packet.", *((int*)read_common_head.tag));
        }
    }
EXIT:
    ALOGW("vbc_ctrl_thread exit!!!");
    return 0;
}

