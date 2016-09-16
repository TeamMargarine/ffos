ifeq ($(strip $(SPRDROID4.0_DEV)),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_C_INCLUDES += \
		$(JNI_H_INCLUDE) \
		$(TOP)/system/core/include \
		$(LOCAL_PATH)/include

LOCAL_SRC_FILES:= \
		src/sprd_dm.c \
		src/spdm_task.c \
		src/sprd_dm_md5_b64.c \
		src/sprd_dm_parsexml.c \
		src/sprd_dm_hander.c \
		src/dm_jni.c

#common_C_INCLUDES += \
#		$(LOCAL_PATH)/include


LOCAL_C_INCLUDES += $(common_C_INCLUDES)

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libcutils libc

LOCAL_MODULE := libsprddm

#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_SHARED_LIBRARY)
endif
