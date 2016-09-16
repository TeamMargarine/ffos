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

#ifndef _SPRD_CAMERA_HARDWARE_CAPABILITY_H_
#define _SPRD_CAMERA_HARDWARE_CAPABILITY_H_

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))


struct capability_element{
	const char *const key;
	const char *const value;
};

struct capability_element sprd_front_camera_hardware_capability[] = {
	{"max_width", "640"},
	{"max_height", "480"},
#ifdef sp6810a
	{"sensor_num", "0"}
#elif sp8810
	{"sensor_num", "0"}
#elif sprdop
	{"sensor_num", "0"}
#else //sp8805ga 
	{"sensor_num", "0"}
#endif
};	

struct capability_element sprd_back_camera_hardware_capability[] = {
#ifdef CONFIG_CAMERA_5M
	{"max_width", "2592"},
	{"max_height", "1944"},
#elif CONFIG_CAMERA_2M
	{"max_width", "1600"},
	{"max_height", "1200"},
#else
	{"max_width", "2048"},
	{"max_height", "1536"},
#endif
#ifdef sp6810a
	{"sensor_num", "0"}
#else //sp8805ga 
	{"sensor_num", "0"}
#endif
};	

struct capability_element sprd_common_camera_hardware_capability[] = {
	{"image_description", "Exif_JPEG_420"},
	{"make", "Spreadtrum"},
#ifdef sp6810a
	{"model", "SP6810a"},
#elif   sp8810	
	{"model", "SP8810ga"},
#elif sprdop
	{"model", "OPENPHONE-SC8810"},
#else
	{"model", "SP8805ga"},
#endif
	{"copyright", "Copyright,Spreadtrum,2011"}
};	

const char* get_front_camera_capability(const char *key){
        uint32_t count = 0, i;
        count = ARRAY_SIZE(sprd_front_camera_hardware_capability);
        for(i = 0; i < count; i++){
                if(0 == strcmp(key, sprd_front_camera_hardware_capability[i].key)){
                        return sprd_front_camera_hardware_capability[i].value;
                }
        }
        return NULL;
}
const char* get_back_camera_capability(const char *key){
        uint32_t count = 0, i;
        count = ARRAY_SIZE(sprd_back_camera_hardware_capability);
        for(i = 0; i < count; i++){
                if(0 == strcmp(key, sprd_back_camera_hardware_capability[i].key)){
                        return sprd_back_camera_hardware_capability[i].value;
                }
        }
        return NULL;
}
const char* get_common_camera_capability(const char *key){
        uint32_t count = 0, i;
        count = ARRAY_SIZE(sprd_common_camera_hardware_capability);
        for(i = 0; i < count; i++){
                if(0 == strcmp(key, sprd_common_camera_hardware_capability[i].key)){
                        return sprd_common_camera_hardware_capability[i].value;
                }
        }
        return NULL;
}
#endif //_SPRD_CAMERA_HARDWARE_CAPABILITY_H_
