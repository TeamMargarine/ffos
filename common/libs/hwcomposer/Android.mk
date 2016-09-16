# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifeq ($(strip $(SPRDROID4.0_DEV)),true)
LOCAL_PATH := $(call my-dir)

# HAL module implemenation stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
ifeq ($(strip $(USE_SPRD_HWCOMPOSER)),true)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog libEGL libbinder libutils libcutils libUMP libGLESv1_CM libhardware libui
LOCAL_SRC_FILES := hwcomposer.cpp \
                   dump_bmp.cpp
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../gralloc \
	$(LOCAL_PATH)/../mali/src/ump/include \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(TOP)/system/core/include/system
LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_CFLAGS:= -DLOG_TAG=\"SPRDhwcomposer\"
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8825)
	LOCAL_CFLAGS += -DSCAL_ROT_TMP_BUF

	LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcamera/sc8825/inc

	LOCAL_SRC_FILES += sc8825/scale_rotate.c

	LOCAL_CFLAGS += -D_PROC_OSD_WITH_THREAD

	LOCAL_CFLAGS += -D_DMA_COPY_OSD_LAYER

	LOCAL_CFLAGS += -D_ALLOC_OSD_BUF
	LOCAL_CFLAGS += -D_SUPPORT_SYNC_DISP
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8810)
	LOCAL_CFLAGS += -DSCAL_ROT_TMP_BUF

	LOCAL_SRC_FILES += sc8810/scale_rotate.c

	LOCAL_CFLAGS += -D_SUPPORT_SYNC_DISP
endif

ifeq ($(strip $(USE_GPU_PROCESS_VIDEO)) , true)
	LOCAL_CFLAGS += -DUSE_GPU_PROCESS_VIDEO
	LOCAL_SRC_FILES += gpu_transform.cpp
endif

ifeq ($(strip $(USE_RGB_VIDEO_LAYER)) , true)
	LOCAL_CFLAGS += -DVIDEO_LAYER_USE_RGB
endif
else #android hwcomposer

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog libEGL libcutils
LOCAL_SRC_FILES := android/hwcomposer.cpp \
                   dump_bmp.cpp
LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_C_INCLUDES := $(TOP)/frameworks/native/include/utils \
                    $(LOCAL_PATH)/android \
                    $(LOCAL_PATH)/../gralloc \
                    $(LOCAL_PATH)/../mali/src/ump/include
LOCAL_CFLAGS:= -DLOG_TAG=\"hwcomposer\"

endif

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
endif