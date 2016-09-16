#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <utils/Log.h>
#include <sys/stat.h>
#include <errno.h>
#include "eng_rf_nv_config.h"

#define LOG_TAG "eng_rf_nv_config"
#define GET_NV_FRM_MODEM_OVERTIME 10*15  //because usleep(100*1000) every time, so here means 15s

int is_read_nv_correct(NV_ADJUST_REG_T* nv_info){
    int i = 0;
    assert(NULL != nv_info);
    if(NV_VERSION_NO != nv_info->nv_ver){
        LOGD("[%s]: Error: nv version in nv %d not equal to expect %d.\n", __FUNCTION__,nv_info->nv_ver, NV_VERSION_NO);
        return -1;
    }
    if((0 == nv_info->antenna_switch) || (0xFFFFFFFF == nv_info->antenna_switch)){
        LOGD("[%s]: Error: antenna_switch == %#x\n", __FUNCTION__,  nv_info->antenna_switch);
        return -1;
    }
    for(i = 0; i<14; i++){
        if(0 == nv_info->DAC_scale_chnnl[i]){
            LOGD("[%s]: Error: DAC_scale_chnnl[%d] == 0\n", __FUNCTION__, i);
            return -1;
        }
    }
    if(0 == nv_info->tx_gain[0]){
        LOGD("[%s]: Error: tx_gain[0] == 0\n", __FUNCTION__);
        return -1;
    }    
    for(i = 0; i<15; i++){
        if(0 == nv_info->reserved2[i]){
            LOGD("[%s]: Error: reserved2[%d] == 0\n", __FUNCTION__, i);
            return -1;
        }
    }    
    return 0;
}

int load_rf_nv_frm_bak_file(char* buf, int length){
    FILE* fd;
    assert((buf != NULL) && (0 != length));
    if(NULL == (fd = fopen(NV_BAK_FILE, "r"))){
        LOGD("[%s] Error: %s\n", __FUNCTION__, strerror(errno));
        return -1;
    }
    if(length != fread(buf, sizeof( char ), length, fd)){
        LOGD("[%s] Error: NV data load from bake file not integrity\n", __FUNCTION__);
        return -1;
    }
    fclose(fd);
    LOGD("[%s] NV data load from bake file OK\n", __FUNCTION__);
    return 0;
}

void get_rf_nvconf(void)
{
    NV_ADJUST_REG_pad_ST nv_rd;
    NV_ADJUST_REG_T nv_send_to_drv;    
    int nv_send_to_drv_word_sz = (sizeof(nv_send_to_drv)/sizeof(uint32));
    unsigned int part1_len = (unsigned int)&(nv_rd.btImb) - (unsigned int)&nv_rd;
    unsigned int part2_len = (unsigned int)&(nv_rd.reserved2) - (unsigned int)&(nv_rd.btImb);
    unsigned int part3_len = sizeof(nv_rd) - part1_len - part2_len;
    int i;
    
    memset((void *)&nv_send_to_drv, 0xFF, sizeof(nv_send_to_drv));

    while(1){
        memset((void *)&nv_rd, 0, sizeof(nv_rd));
        
        if(-1 != read_rf_nvconf_from_modem((char *)&nv_rd, part1_len, (char *)&(nv_rd.btImb), part2_len, (char *)&(nv_rd.reserved2), part3_len)){
            LOGD("[%s]: get nv from modem is OK\n", __FUNCTION__);
            break;
        }
        else{
            //get nv from modem(overtime) -> get nv from bake-up file(fail) -> get nv from modem(forever)     
            if(GET_NV_FRM_MODEM_OVERTIME == i){
                if(0 == load_rf_nv_frm_bak_file((char *)&nv_rd, sizeof(nv_rd))){
                    LOGD("[%s]: get nv from bake-up file OK\n",__FUNCTION__); 
                    break;
                }
                else{
                    LOGD("[%s] Error: get nv from bake-up file failed! retry get nv from modem\n",__FUNCTION__);         
                }
            }
            else{
                LOGD("[%s] Error: get nv from modem failed! retry!\n",__FUNCTION__);
            }
            usleep(100*1000);
            i++;
        }
    }
    
    ConvertStrToU32Hex((uint8 *)&nv_rd, sizeof(nv_rd), (uint32 *)&nv_send_to_drv);

    for(i=0; i<nv_send_to_drv_word_sz; i++)
    {
        #ifdef NV_CAL_TRACE_ENABLE
        //LOGD("before conv le [%d] = [%#x]!\n", i, p_nv_send_to_drv[i]);
        #endif        
        *((uint32 *)&nv_send_to_drv+i) = u32_convert_to_le(*((uint32 *)&nv_send_to_drv+i));
        #ifdef NV_CAL_TRACE_ENABLE
        //LOGD("after  conv le [%d] = [%#x]!\n", i, p_nv_send_to_drv[i]);
        #endif
    }

    if(-1 == is_read_nv_correct(&nv_send_to_drv)){
        LOGD("[%s] Error: read nv is not correct.\n", __FUNCTION__);
        return;
    }
    send_info_to_driver((char *)&nv_send_to_drv, sizeof(nv_send_to_drv));
}

int main(void)
{
    LOGD("[eng_rf_nv_config]: start get rf nv config!\n");
    signal(SIGPIPE, SIG_IGN);
    get_rf_nvconf(); 
    LOGD("[eng_rf_nv_config]: end get rf nv config!\n");
    return 0;
}
