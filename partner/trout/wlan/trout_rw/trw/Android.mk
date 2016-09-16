LOCAL_PATH := $(call my-dir)

OBJS_c += main.c obj.c string.c lua_api.c

INCLUDES += $(LOCAL_PATH)/../lib/lua-5.2.2/src $(LOCAL_PATH)/../lib/libtroutrw
########################

include $(CLEAR_VARS)
LOCAL_MODULE := trw
LOCAL_MODULE_TAGS := debug
LOCAL_SHARED_LIBRARIES := libc libcutils 
LOCAL_STATIC_LIBRARIES := liblua libtroutrw
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := $(OBJS_c)
LOCAL_C_INCLUDES := $(INCLUDES)
include $(BUILD_EXECUTABLE)

########################
