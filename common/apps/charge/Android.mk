ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

define _add-charge-image
include $$(CLEAR_VARS)
LOCAL_MODULE := charge_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_img_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_ROOT_OUT)/res/images
include $$(BUILD_PREBUILT)
endef

_img_modules :=
_images :=
$(foreach _img, $(call find-subdir-subdir-files, "images", "*.png"), \
  $(eval $(call _add-charge-image,$(_img))))

include $(CLEAR_VARS)
LOCAL_MODULE := charge_res_images
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_img_modules)
include $(BUILD_PHONY_PACKAGE)

_img_modules :=
_add-charge-image :=
include $(CLEAR_VARS)
commands_recovery_local_path := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
    charge.c \
	battery.c \
	backlight.c \
	power.c \
	log.c \
    ui.c 

ifneq ($(strip $(TARGET_USERIMAGES_USE_EXT4)),true)
LOCAL_CFLAGS += -DCONFIG_SYNC_FILE
endif

ifeq ($(strip $(HAVE_KEYBOARD_BACKLIGHT)),true)
LOCAL_CFLAGS += -DK_BACKLIGHT
endif

LOCAL_MODULE := charge 
LOCAL_MODULE_TAGS := optional

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_BIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_BIN_UNSTRIPPED)

LOCAL_STATIC_LIBRARIES := libliteui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libstdc++ libc libz
LOCAL_REQUIRED_MODULES := charge_res_images

include $(BUILD_EXECUTABLE)


include $(commands_recovery_local_path)/minui/Android.mk

commands_recovery_local_path :=

endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR

