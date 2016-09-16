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

#ifndef ANDROID_HARDWARE_SPRD_OEM_CAMERA_H
#define ANDROID_HARDWARE_SPRD_OEM_CAMERA_H

#include <sys/types.h>
#include <utils/Log.h>


#include "SprdCameraIfc.h"
#include "SprdCameraHardwareInterface.h"

namespace android {
void camera_assoc_pmem(qdsp_module_type module,
                                  int pmem_fd,
                                  void *addr,
                                  uint32_t length,
                                  int external);
void clear_module_pmem(qdsp_module_type module);
int camera_release_pmem(qdsp_module_type module,
                                   void *addr,
                                   uint32_t size,
                                   uint32_t force);
camera_ret_code_type camera_encode_picture(
        camera_frame_type *frame,
        camera_handle_type *handle,
        camera_cb_f_type callback,
        void *client_data);

camera_ret_code_type camera_init(int32_t camera_id);
void camera_af_init(void);
camera_ret_code_type camera_release_frame(uint32_t index);
camera_ret_code_type camera_set_dimensions (
        uint16_t picture_width,
        uint16_t picture_height,
        uint16_t display_width,
#ifdef FEATURE_CAMERA_V7
        uint16_t display_height,
#endif
        camera_cb_f_type callback,
        void *client_data);
camera_ret_code_type camera_set_encode_properties(
        camera_encode_properties_type *encode_properties);
camera_ret_code_type camera_set_parm(
        camera_parm_type id,
        int32_t          parm,
        camera_cb_f_type callback,
        void            *client_data);
camera_ret_code_type camera_set_position(
        camera_position_type *position,
        camera_cb_f_type      callback,
        void                 *client_data);
camera_ret_code_type camera_set_thumbnail_properties (
                              uint32_t width,
                              uint32_t height,
                              uint32_t quality);
camera_ret_code_type camera_start (
        camera_cb_f_type callback,
        void *client_data
#ifdef FEATURE_NATIVELINUX
        ,int  display_height,
        int  display_width
#endif // FEATURE_NATIVELINUX
        );
camera_ret_code_type camera_start_preview (
        camera_cb_f_type callback,
        void *client_data);
camera_ret_code_type camera_start_focus (
        camera_focus_e_type focus,
        camera_cb_f_type callback,
        void *client_data);
camera_ret_code_type camera_stop_focus (void);
camera_ret_code_type camera_stop(
        camera_cb_f_type callback,
        void *client_data);
camera_ret_code_type camera_stop_preview (void);
camera_ret_code_type camera_take_picture (
        camera_cb_f_type    callback,
        void               *client_data
#if !defined FEATURE_CAMERA_ENCODE_PROPERTIES && defined FEATURE_CAMERA_V7
        ,camera_raw_type camera_raw_mode
#endif // nFEATURE_CAMERA_ENCODE_PROPERTIES && FEATURE_CAMERA_V7
        );
void rex_start();
void rex_shutdown();
uint32_t camera_get_size_align_page(uint32_t size);
uint32_t camera_get_frame_size(uint32_t width, uint32_t height, uint32_t type);
void camera_alloc_zoom_buffer(uint32_t phy_addr, uint8_t *vir_addr,uint32_t size);
void camera_alloc_swap_buffer(uint32_t phy_addr);
};
#endif //ANDROID_HARDWARE_SPRD_OEM_CAMERA_H
