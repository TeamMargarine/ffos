LOCAL_PATH := $(call my-dir)

RW_DIR_INCLUDE :=

RW_SRC_FILE += libtrw.c phy_rw.c


########################

include $(CLEAR_VARS)
LOCAL_MODULE := libtroutrw
LOCAL_MODULE_TAGS := debug
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := $(RW_SRC_FILE)
LOCAL_C_INCLUDES := $(RW_DIR_INCLUDE)
include $(BUILD_STATIC_LIBRARY)
########################
