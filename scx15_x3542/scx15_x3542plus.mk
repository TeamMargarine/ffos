#
# Copyright (C) 2007 The Android Open Source Project
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
#

TARGET_PLATFORM := scx15
PLATDIR := device/sprd/$(TARGET_PLATFORM)

TARGET_BOARD := scx15_x3542
BOARDDIR := device/sprd/$(TARGET_BOARD)

# include general common configs
$(call inherit-product, $(BOARDDIR)/device.mk)
$(call inherit-product, $(PLATDIR)/nand/nand_device.mk)
$(call inherit-product, $(PLATDIR)/proprietories.mk)

DEVICE_PACKAGE_OVERLAYS := $(BOARDDIR)/overlay $(PLATDIR)/overlay

PRODUCT_AAPT_CONFIG := mdpi

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mass_storage

ENABLE_LIBRECOVERY := true
RECOVERY_EXTERNAL_STORAGE := /sdcard
SYSTEM_FS_TYPE := ubifs
SYSTEM_PARTITION_TYPE := UBI
SYSTEM_LOCATION := /dev/ubi0_system

PRODUCT_PROPERTY_OVERRIDES += \
	keyguard.no_require_sim=true \
	ro.com.android.dataroaming=false \
	ro.msms.phone_count=2 \
	persist.msms.phone_count=2 \
	persist.msms.phone_default=0 \
	persist.msmslt=0 \
	ro.modem.w.count=2 \
        persist.sys.modem.diag=,gser \
        sys.usb.gser.count=4 \
        wcdma.sim.slot.cfg=true \
        persist.support.oplpnn=true \
        persist.support.cphsfirst=false \
	ro.sf.lcd_density=160 \
	ro.sf.lcd_width=54 \
	ro.sf.lcd_height=96 \
	lmk.autocalc=false \
	ro.moz.ril.query_icc_count=true \
	ro.moz.mute.call.to_ril=true \
	ro.moz.ril.numclients=2 \
        ro.moz.ril.data_reg_on_demand=true\
        ro.moz.ril.radio_off_wo_card=true\
        ro.moz.ril.0.network_types = gsm,wcdma\
        ro.moz.ril.1.network_types = gsm

# board-specific modules
PRODUCT_PACKAGES += \
        sensors.$(TARGET_PLATFORM) \
	fm.$(TARGET_PLATFORM) \
        ValidationTools

# Remove video wallpaper application and resources
PRODUCT_VIDEO_WALLPAPERS := none
# for Gecko to support bluedroid stack
PRODUCT_PACKAGES += \
	bluetooth.default

# Additional product packages
-include vendor/sprd/open-source/common_packages.mk
-include vendor/sprd/open-source/plus_special_packages.mk

#	libmllite.so \
#	libmplmpu.so \
#	libinvensense_hal
#
# board-specific files
PRODUCT_COPY_FILES += \
	$(BOARDDIR)/init.recovery.board.rc:root/init.recovery.board.rc \
	$(BOARDDIR)/init.board.rc:root/init.board.rc \
	$(BOARDDIR)/audio_params/tiny_hw.xml:system/etc/tiny_hw.xml \
	$(BOARDDIR)/audio_params/codec_pga.xml:system/etc/codec_pga.xml \
	$(BOARDDIR)/audio_params/audio_hw.xml:system/etc/audio_hw.xml \
	$(BOARDDIR)/audio_params/audio_para:system/etc/audio_para \
	$(BOARDDIR)/audio_params/audio_policy.conf:system/etc/audio_policy.conf \
	$(BOARDDIR)/focaltech_ts.idc:system/usr/idc/focaltech_ts.idc \
	frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
	frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
	frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
	frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml

PRODUCT_COPY_FILES += $(BOARDDIR)/volume.cfg:system/etc/volume.cfg

$(call inherit-product, vendor/sprd/open-source/res/boot/boot_res_x3542.mk)
$(call inherit-product, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)
$(call inherit-product, vendor/sprd/gps/CellGuide_2351/device-sprd-gps.mk)
$(call inherit-product, vendor/sprd/partner/shark/bluetooth/device-shark-bt.mk)
$(call inherit-product, vendor/sprd/open-source/res/productinfo/connectivity_configure_7715.mk)

# Overrides
PRODUCT_NAME := scx15_x3542plus
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := GoFox F15
PRODUCT_BRAND := Symphony
PRODUCT_MANUFACTURER := Sprocomm

PRODUCT_LOCALES := bn_BD en_US
