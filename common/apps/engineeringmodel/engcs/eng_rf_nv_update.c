#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <utils/Log.h>
#include <sys/stat.h>
#include <errno.h>
#include "eng_rf_nv_config.h"

#define LOG_TAG "eng_rf_nv_update"

int save_rf_nv_to_bak_file(const char* buf, int length){
    FILE* fd;
    assert((buf != NULL) && (0 != length));

    if(NULL == (fd = fopen(NV_BAK_FILE, "w"))){
        LOGD("[%s] Error: %s\n", __FUNCTION__, strerror(errno));
        return -1;
    }
    if(length != fwrite(buf, sizeof( char ), length, fd)){
        LOGD("[%s] Error: NV data save to bake file not integrity\n", __FUNCTION__);
        return -1;
    }    
    fclose(fd);
    LOGD("[%s] NV data save to bake file OK\n", __FUNCTION__);
    return 0;
}

int is_write_nv_correct(NV_ADJUST_REG_T* nv_info){
    int i = 0;
    assert(NULL != nv_info);
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
    if(0 == nv_info->wifi_imb[0]){
        LOGD("[%s]: Error: wifi_imb[0] == 0\n", __FUNCTION__);
        return -1;
    }
    for(i = 0; i<4; i++){
        if(0 == nv_info->wifi_LOFT[i]){
            LOGD("[%s]: Error: wifi_LOFT[%d] == 0\n", __FUNCTION__, i);
            return -1;
        }
    }
    for(i = 0; i<4; i++){
        if(0 == nv_info->BT_LOFT[i]){
            LOGD("[%s]: Error: BT_LOFT[%d] == 0\n", __FUNCTION__, i);
            return -1;
        }
    }
    if(0 == nv_info->btImb[0]){
        LOGD("[%s]: Error: btImb[0] == 0\n", __FUNCTION__);
        return -1;
    }
    for(i = 0; i<32; i++){
        if(0 == nv_info->wifiDcoc[i]){
            LOGD("[%s]: Error: wifiDcoc[%d] == 0\n", __FUNCTION__, i);
            return -1;
        }
    }
    for(i = 0; i<32; i++){
        if(0 == nv_info->btDcoc[i]){
            LOGD("[%s]: Error: btDcoc[%d] == 0\n", __FUNCTION__, i);
            return -1;
        }
    }
    return 0;
}

void set_rf_nvconf(void)
{
    NV_ADJUST_REG_pad_ST nv_to_wr;
    NV_ADJUST_REG_T nv_frm_drv;
    unsigned int part1_len = (unsigned int)&(nv_to_wr.btImb) - (unsigned int)&nv_to_wr;
    unsigned int part2_len = (unsigned int)&(nv_to_wr.reserved2) - (unsigned int)&(nv_to_wr.btImb);
    unsigned int part3_len = sizeof(nv_to_wr) - part1_len - part2_len;
    char tmp_nv_frm_drv[sizeof(NV_ADJUST_REG_T)+1] = {0,};    

    while(1){
        memset(tmp_nv_frm_drv, 0, sizeof(tmp_nv_frm_drv));
        memset(&nv_to_wr, 0, sizeof(nv_to_wr));
        memset(&nv_frm_drv, 0, sizeof(nv_frm_drv));
        get_info_from_driver(tmp_nv_frm_drv, sizeof(tmp_nv_frm_drv));

        if(SELFCAL_DONE == tmp_nv_frm_drv[0]){
            LOGD("[%s]: self calibration has done, start update nv!\n", __FUNCTION__);
            memcpy(&nv_frm_drv, tmp_nv_frm_drv+1, sizeof(nv_frm_drv));

            if(-1 == is_write_nv_correct(&nv_frm_drv)){
                LOGD("[%s] Error: write nv is not correct.\n", __FUNCTION__);
                return;
            }
            // convert nv_frm_drv to nv_to_wr
            ConvertU32HexToStr((uint8 *)&nv_frm_drv, sizeof(nv_frm_drv), (uint8 *)&nv_to_wr, sizeof(nv_to_wr));
            LOGD("[%s]: nv_to_wr = [%s], len=[%d]\n", __FUNCTION__, (char *)&nv_to_wr, sizeof(nv_to_wr)); 
                
            if(-1 == write_rf_nvconf_to_modem((char *)&nv_to_wr, part1_len, (char *)&(nv_to_wr.btImb), part2_len, (char *)&(nv_to_wr.reserved2), part3_len)){
                LOGD("[%s] Error: NV updated failed!\n",__FUNCTION__);
            }
            else{            
                LOGD("[%s]: NV updated OK!\n",__FUNCTION__);          
                save_rf_nv_to_bak_file((char *)&nv_to_wr, sizeof(nv_to_wr));
            }
           return;
        }
        else if(SELFCAL_DEFAULT == tmp_nv_frm_drv[0]){
            LOGD("[%s]: NV updated flag is 0xA5, don't need to update NV\n",__FUNCTION__);
            return;
        }
        else
        {
             LOGD("[%s]: waiting rf finish self calibration!\n", __FUNCTION__);
        }            
        usleep(500*1000);
    }
}

int main(void)
{
    LOGD("[eng_rf_nv_update]: start eng rf nv update!\n");
    signal(SIGPIPE, SIG_IGN);
    set_rf_nvconf(); 
    LOGD("[eng_rf_nv_update]: end eng rf nv update!\n");
    return 0;
}
