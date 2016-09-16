LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	modemd.c

LOCAL_SHARED_LIBRARIES := \
	libcutils

LOCAL_MODULE := modemd

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
