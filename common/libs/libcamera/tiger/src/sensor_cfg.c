/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#include <linux/i2c.h>
//#include <linux/gpio.h>
//#include <linux/delay.h>
//#include <mach/hardware.h>
//#include <asm/io.h>
//#include <linux/list.h>
#include "sensor_drv_u.h"
#include "sensor_cfg.h"

/**---------------------------------------------------------------------------*
 **                         extend Variables and function                     *
 **---------------------------------------------------------------------------*/
extern SENSOR_INFO_T g_OV7675_yuv_info;
extern SENSOR_INFO_T g_OV7670_yuv_info;
extern SENSOR_INFO_T g_OV9655_yuv_info;
extern SENSOR_INFO_T g_OV2640_yuv_info;
extern SENSOR_INFO_T g_OV2655_yuv_info;
extern SENSOR_INFO_T g_GC0306_yuv_info;
extern SENSOR_INFO_T g_SIV100A_yuv_info;
extern SENSOR_INFO_T g_SIV100B_yuv_info;
extern SENSOR_INFO_T g_OV3640_yuv_info;
extern SENSOR_INFO_T g_mt9m112_yuv_info;
extern SENSOR_INFO_T g_OV9660_yuv_info;
extern SENSOR_INFO_T g_OV7690_yuv_info;
extern SENSOR_INFO_T g_OV7675_yuv_info;
extern SENSOR_INFO_T g_GT2005_yuv_info;
extern SENSOR_INFO_T g_GC0309_yuv_info;
extern SENSOR_INFO_T g_ov5640_yuv_info;
extern SENSOR_INFO_T g_OV7660_yuv_info;
extern SENSOR_INFO_T g_nmi600_yuv_info;//atv:nmi bonnie

/**---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/
const SENSOR_INFO_T* main_sensor_infor_tab[]=
{
	&g_ov5640_yuv_info,
	//&g_OV7675_yuv_info,
	//&g_OV2655_yuv_info,
	//&g_OV7675_yuv_info,
	//&g_OV2640_yuv_info,
	PNULL
};

const SENSOR_INFO_T* sub_sensor_infor_tab[]=
{
	//&g_GC0309_yuv_info,
	//g_OV7690_yuv_info,
	PNULL
};


const SENSOR_INFO_T* atv_infor_tab[]=
{
    //&g_nmi600_yuv_info, //&g_tlg1120_yuv_info,  bonnie
	PNULL
};

/*****************************************************************************/
//  Description:    This function is used to get sensor information table    
//  Author:         Liangwen.Zhen
//  Note:           
/*****************************************************************************/
SENSOR_INFO_T ** Sensor_GetInforTab(SENSOR_ID_E sensor_id)
{
	SENSOR_INFO_T * sensor_infor_tab_ptr=NULL;

	switch(sensor_id)
	{
		case SENSOR_MAIN:
		{
			sensor_infor_tab_ptr=(SENSOR_INFO_T*)&main_sensor_infor_tab;
			break;
		}
		case SENSOR_SUB:
		{
			sensor_infor_tab_ptr=(SENSOR_INFO_T*)&sub_sensor_infor_tab;
			break;
		}
		case SENSOR_ATV: 
		{
			sensor_infor_tab_ptr=(SENSOR_INFO_T*)&atv_infor_tab;
			break;
		}
		default:
			break;
	}

	return (SENSOR_INFO_T **)sensor_infor_tab_ptr;
}

/*****************************************************************************/
//  Description:    This function is used to get sensor information table    
//  Author:         Liangwen.Zhen
//  Note:           
/*****************************************************************************/
uint32_t Sensor_GetInforTabLenght(SENSOR_ID_E sensor_id)
{
	uint32_t tab_lenght = 0;

	switch(sensor_id)
	{
		case SENSOR_MAIN:
		{
			tab_lenght=(sizeof(main_sensor_infor_tab)/sizeof(SENSOR_INFO_T*));
			break;
		}
		case SENSOR_SUB:
		{
			tab_lenght=(sizeof(sub_sensor_infor_tab)/sizeof(SENSOR_INFO_T*));
			break;
		}
		case SENSOR_ATV: 
		{
			tab_lenght=(sizeof(atv_infor_tab)/sizeof(SENSOR_INFO_T*));
			break;
		}
		default:
			break;
	}

	return tab_lenght;
}


#if 0
static LIST_HEAD(main_sensor_info_list);	/*for back camera */
static LIST_HEAD(sub_sensor_info_list);	/*for front camera */
static LIST_HEAD(atv_info_list);	/*for atv */
static DEFINE_MUTEX(sensor_mutex);


int dcam_register_sensor_drv(struct sensor_drv_cfg *cfg)
{
	//printk(KERN_INFO "Sensor driver is %s.\n", cfg->sensor_name);
	mutex_lock(&sensor_mutex);
	if (cfg->sensor_pos == 1) {
		list_add_tail(&cfg->list, &main_sensor_info_list);
	} else if (cfg->sensor_pos == 2) {
		list_add_tail(&cfg->list, &sub_sensor_info_list);
	} else if (cfg->sensor_pos == 3) {
		list_add_tail(&cfg->list, &main_sensor_info_list);
		list_add_tail(&cfg->list, &sub_sensor_info_list);
	} else {
		list_add_tail(&cfg->list, &atv_info_list);
	}
	mutex_unlock(&sensor_mutex);

	return 0;
}

struct list_head *Sensor_GetList(SENSOR_ID_E sensor_id)
{
	struct list_head *sensor_list = 0;

	pr_debug("sensor cfg:Sensor_GetList,id=%d.\n", sensor_id);
	switch (sensor_id) {
	case SENSOR_MAIN:
		sensor_list = &main_sensor_info_list;
		break;
	case SENSOR_SUB:
		sensor_list = &sub_sensor_info_list;
		break;
	case SENSOR_ATV:
		sensor_list = &atv_info_list;
		break;
	default:
		printk("sensor cfg:Sensor_GetList fail!\n");
		break;
	}

	return sensor_list;
}
#endif
