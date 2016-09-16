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

TARGET_PLATFORM := sc8810
TARGET_BOARD := sp6821a_gonk
BOARDDIR := device/sprd/$(TARGET_BOARD)
EXPORT_DEVICE_PREFS := device/sprd/$(TARGET_BOARD)

DEVICE_PACKAGE_OVERLAYS := $(BOARDDIR)/overlay

PRODUCT_AAPT_CONFIG := hdpi

PRODUCT_PROPERTY_OVERRIDES := \
	keyguard.no_require_sim=true \
	zram.support=true \
	ro.com.android.dataroaming=false \
	persist.msms.phone_count=2 \
	persist.msms.phone_default=0 \
	persist.sys.sprd.modemreset=0

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mass_storage

PRODUCT_PACKAGES := \
	VoiceDialer \
	MsmsPhone \
    	framework2 \
	Settings \
        MsmsStk \
        Stk1

PRODUCT_COPY_FILES := \
	$(BOARDDIR)/sprd-keypad.kl:system/usr/keylayout/sprd-keypad.kl \
	$(BOARDDIR)/ms-msg21xx.idc:system/usr/idc/ms-msg21xx.idc

$(call inherit-product, frameworks/base/build/phone-hdpi-dalvik-heap.mk)

# include classified configs
$(call inherit-product, $(BOARDDIR)/base.mk)
$(call inherit-product, $(BOARDDIR)/proprietories.mk)
$(call inherit-product, device/sprd/common/res/boot/boot_res.mk)

# include standard configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/telephony.mk)

# Overrides
PRODUCT_NAME := sp6821a_gonk
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := sp6821a
PRODUCT_BRAND := Spreadtrum
PRODUCT_MANUFACTURER := Spreadtrum

PRODUCT_LOCALES := zh_CN zh_TW en_US

# use woff font for ROM size
MOZ_PRODUCT_COMPRESS_FONTS := true

# Gecko/Gaia need below export value
# Using Mozilla API key to support MLS
export MOZILLA_MLS_KEY_FILE=$(ANDROID_BUILD_TOP)/$(BOARDDIR)/mls.key
export GAIA_DISTRIBUTION_DIR=$(PWD)/device/sprd/sp6821a_gonk/
export GAIA_APP_SRCDIRS=apps external-apps outoftree_apps customize_apps/engmode/assets customize_apps/ValidationTools/assets
export PRODUCTION=1
export MOZILLA_OFFICIAL=1
export NOFTU=1
export LOCALE_BASEDIR=$(PWD)/gaia-l10n/
export LOCALES_FILE=$(PWD)/device/sprd/sp6821a_gonk/languages.json
export GAIA_DEFAULT_LOCALE=en-US
export GAIA_KEYBOARD_LAYOUTS=en,bn-Avro,bn-Probhat
