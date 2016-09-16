#
# Copyright (C) 2011 The Android Open-Source Project
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

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_CORTEX_CACHE_LINE_32 := true

TARGET_BOARD_PLATFORM := sc8810
TARGET_BOOTLOADER_BOARD_NAME := sp6821a

# config u-boot
TARGET_NO_BOOTLOADER := false
UBOOT_DEFCONFIG := sp6821a

# config kernel
ifeq ($(TARGET_BUILD_VARIANT), user)
KERNEL_DEFCONFIG := sp6821a-vlx-user_defconfig
else
KERNEL_DEFCONFIG := sp6821a-vlx_defconfig
endif
USES_UNCOMPRESSED_KERNEL := true
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_CMDLINE := console=ttyS1,115200n8

# use default init.rc
TARGET_PROVIDES_INIT_RC := true

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_TROUT := true

BOARD_WLAN_DEVICE			:= itm
BOARD_WPA_SUPPLICANT_DRIVER 		:= NL80211
BOARD_HOSTAPD_DRIVER 			:= NL80211
WPA_SUPPLICANT_VERSION			:= VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB	:= lib_driver_cmd_trout
BOARD_HOSTAPD_PRIVATE_LIB		:= lib_driver_cmd_trout
WIFI_DRIVER_FW_PATH_STA			:= "/system/lib/modules/itm_sta.ko"
WIFI_DRIVER_FW_PATH_P2P			:= ""
WIFI_DRIVER_FW_PATH_AP			:= "/system/lib/modules/itm_ap.ko"
WIFI_DRIVER_FW_PATH_PARAM		:= ""
WIFI_DRIVER_MODULE_PATH         	:= ""
WIFI_DRIVER_MODULE_NAME         	:= "itm_wifi"
HAVE_WLAN_CMCC_FEATURE      := true

Trout_FM := true

# board specific modules
BOARD_USES_TINYALSA_AUDIO := true
BOARD_USES_LINE_CALL := false
BOARD_USE_VETH := true
BOARD_SPRD_RIL := true
BOARD_SAMSUNG_RIL := false

# ext4 partition layout
#TARGET_USERIMAGES_USE_EXT4 := true
#BOARD_SYSTEMIMAGE_PARTITION_SIZE := 300000000
#BOARD_USERDATAIMAGE_PARTITION_SIZE := 2000000000
#BOARD_CACHEIMAGE_PARTITION_SIZE := 150000000
#BOARD_FLASH_BLOCK_SIZE := 4096
#BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4

USE_OPENGL_RENDERER := true

USE_CAMERA_STUB := true
CAMERA_DISP_ION := true

#select 50W, 130W, 2M,3M or 5M
CAMERA_SUPPORT_SIZE := 130W

TARGET_BOARD_NO_FRONT_SENSOR := true
TARGET_BOARD_CAMERA_ROTATION_CAPTURE := false

BOARD_USES_GENERIC_AUDIO := false
#FFOS can not utilize the hwcomposer now
#USE_SPRD_HWCOMPOSER :=true
USE_GPU_PROCESS_VIDEO :=true
USE_RGB_VIDEO_LAYER :=true

USE_BOOT_AT_DIAG := true

USE_ENG_UART_USB_AUTO := true

#light sensor
BOARD_HAVE_TMD2771 := true

