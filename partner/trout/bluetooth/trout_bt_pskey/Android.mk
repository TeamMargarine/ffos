#trout_genpskey
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libcutils libhardware_legacy libutils
LOCAL_STATIC_LIBRARIES  :=
LOCAL_LDLIBS        += -Idl
#LOCAL_CFLAGS        += -D$(BOARD_PRODUCT_NAME)

LOCAL_C_INCLUDES    +=  trout_genpskey.h \
	device/sprd/common/apps/engineeringmodel/engcs \

LOCAL_SRC_FILES     :=   trout_genpskey.c \
						pskey_bt.c \

LOCAL_MODULE := trout_genpskey
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)