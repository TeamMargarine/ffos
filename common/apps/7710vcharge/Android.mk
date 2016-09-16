ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_LDLIBS += -Idl

LOCAL_SRC_FILES := vcharge.c


LOCAL_MODULE := vcharged7
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_BIN)

LOCAL_STATIC_LIBRARIES := libcutils libc 

include $(BUILD_EXECUTABLE)

endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR

