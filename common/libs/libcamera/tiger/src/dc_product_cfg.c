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

#include <utils/Log.h>
#include "dc_product_cfg.h"
#include "jpeg_exif_header.h"
//typedef int8_t int8;

#define SCI_TRUE 1
#define SCI_FALSE 0
LOCAL uint32_t _DC_GetProductCfgInfo(uint32_t param);
LOCAL uint32_t _DC_GetExifPrimaryPriDescInfo(uint32_t param);
LOCAL uint32_t _DC_GetExifSpecUserInfo(uint32_t param);

#define DC_MEM_SIZE (3*1024*1024)
#define DV_MEM_SIZE (3*1024*1024)
#define VT_MEM_SIZE (3*1024*1024)
LOCAL EXIF_PRI_DESC_T s_dc_exif_pri_desc_info = {
	{
	 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}, "Default Date",	/*Date */
	"ImageDescription",	/*ImageDescription */
	"Maker",		/*Make */
	"Model",		/*Model */
	"Software Version v0.0.0",	/*Software */
	"Artist",		/*Artist */
	"CopyRight"		/*Copyright */
};

LOCAL uint8_t  exif_user_comments[20] = {
	"User Comments"
};

LOCAL EXIF_SPEC_USER_T s_dc_exif_spec_user_info = {
	0
};

LOCAL DC_PRODUCT_CFG_T s_dc_product_cfg_info = {
	SCI_TRUE, SCI_TRUE, SCI_FALSE, SCI_FALSE, DC_MEM_SIZE,
	DV_MEM_SIZE, VT_MEM_SIZE, DC_MAX_VIDEO_MODE_CIF,
	DC_PRODUCT_FLASH_TYPE_DISABLE
};

LOCAL DC_PRODUCT_CFG_FUNC_TAB_T s_dc_product_cfg_fun = {
	_DC_GetProductCfgInfo, _DC_GetExifPrimaryPriDescInfo,
	_DC_GetExifSpecUserInfo
};

LOCAL uint32_t _DC_GetProductCfgInfo(uint32_t param)
{
	return (uint32_t) & s_dc_product_cfg_info;
}

LOCAL uint32_t _DC_GetExifPrimaryPriDescInfo(uint32_t param)
{
	EXIF_PRI_DESC_T *exif_ptr = &s_dc_exif_pri_desc_info;

#ifdef KERNEL_TIME
	SCI_DATE_T cur_date = {
		0
	};
	SCI_TIME_T cur_time = {
		0
	};
	*(uint32_t *) & s_dc_exif_pri_desc_info.valid = (uint32_t) 0x7F;
	TM_GetSysDate(&cur_date);
	TM_GetSysTime(&cur_time);
	sprintf((int8 *) exif_ptr->DateTime, "%04d:%02d:%02d %02d:%02d:%02d",
		cur_date.year, cur_date.mon, cur_date.mday, cur_time.hour,
		cur_time.min, cur_time.sec);

#endif /*  */
	/*    sprintf((int8*)s_dc_exif_pri_desc_info.Copyright, "%s", "CopyRight"); */
	/*    sprintf((int8*)s_dc_exif_pri_desc_info.ImageDescription, "%s", "ImageDescription"); */
	sprintf((char *) s_dc_exif_pri_desc_info.Make, "%s", "Spreadtrum");
	sprintf((char *) s_dc_exif_pri_desc_info.Model, "%s", "SP8810ga");

	/*   sprintf((int8*)s_dc_exif_pri_desc_info.Software, "%s", "Test Version v0.0.0.1"); */
	/*/   sprintf((int8*)s_dc_exif_pri_desc_info.Artist, "%s", "Artist"); */
	sprintf((char *) s_dc_exif_pri_desc_info.Copyright, "%s",
		"CopyRight, Spreadtrum, 2012");
	return (uint32_t) exif_ptr;
}

LOCAL uint32_t _DC_GetExifSpecUserInfo(uint32_t param)
{
	EXIF_SPEC_USER_T *exif_ptr = &s_dc_exif_spec_user_info;

	/*
	   s_dc_exif_spec_user_info.valid.MakerNote = 0;
	   s_dc_exif_spec_user_info.valid.UserComment = 0;
	   s_dc_exif_spec_user_info.UserComment.count = 1;
	   s_dc_exif_spec_user_info.UserComment.count = strlen((char*)exif_user_comments);
	   s_dc_exif_spec_user_info.UserComment.ptr = (void*)exif_user_comments;
	   s_dc_exif_spec_user_info.UserComment.type = EXIF_ASCII;
	   s_dc_exif_spec_user_info.UserComment.size = 1;
	 */
	DC_PRO_CFG_PRINT
	    ("DC_PRODUCT: _DC_GetExifSpecUserInfo, valid.UserComment %d \n",
	     s_dc_exif_spec_user_info.valid.UserComment);
	return (uint32_t) exif_ptr;
}

DC_PRODUCT_CFG_FUNC_TAB_T_PTR DC_GetDcProductCfgFun(void)
{
	return &s_dc_product_cfg_fun;
}
