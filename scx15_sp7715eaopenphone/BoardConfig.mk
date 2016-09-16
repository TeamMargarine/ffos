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

-include device/sprd/scx15/BoardConfigCommon.mk

# emmc fstab
TARGET_RECOVERY_FSTAB := device/sprd/scx15/emmc/recovery.fstab

# board configs
TARGET_BOOTLOADER_BOARD_NAME := scx15_sp7715eaopenphone
UBOOT_DEFCONFIG := sp7715eaopenphone
KERNEL_DEFCONFIG := sp7715eaopenphone-native_defconfig

TARGET_GPU_BASE_FREQ := 256

# select camera 2M,3M,5M,8M
CAMERA_SUPPORT_SIZE := 5M
TARGET_BOARD_FRONT_CAMERA_ROTATION := true
TARGET_BOARD_NO_FRONT_SENSOR := false
TARGET_BOARD_CAMERA_FLASH_CTRL := false

#face detect
TARGET_BOARD_CAMERA_FACE_DETECT := false

TARGET_BOARD_CAMERA_USE_IOMMU := true
TARGET_BOARD_BACK_CAMERA_INTERFACE := ccir
TARGET_BOARD_FRONT_CAMERA_INTERFACE := ccir

#rotation capture
TARGET_BOARD_CAMERA_ROTATION_CAPTURE := true

# select WCN
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_SPRD := true
BOARD_HAVE_FM_TROUT := true
BOARD_USE_SPRD_FMAPP := true

# GPS
BOARD_USE_SPRD_4IN1_GPS := true

# WIFI configs
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_2_1_DEVEL
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_ittiam
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_ittiam
BOARD_WLAN_DEVICE           := ittiam
WIFI_DRIVER_FW_PATH_PARAM   := "/data/misc/wifi/fwpath"
WIFI_DRIVER_FW_PATH_STA     := "sta_mode"
WIFI_DRIVER_FW_PATH_P2P     := "p2p_mode"
WIFI_DRIVER_FW_PATH_AP      := "ap_mode"
WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/ittiam.ko"
WIFI_DRIVER_MODULE_NAME     := "ittiam"

# select sensor
#USE_INVENSENSE_LIB := true
USE_SPRD_SENSOR_LIB := true
BOARD_HAVE_ACC := Lis3dh
BOARD_ACC_INSTALL := 6
BOARD_HAVE_ORI := ST480
BOARD_ORI_INSTALL := 7
BOARD_HAVE_PLS := LTR558ALS

# ext4 partition layout
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 400000000
BOARD_USERDATAIMAGE_PARTITION_SIZE := 3200000000
BOARD_CACHEIMAGE_PARTITION_SIZE := 150000000
BOARD_PRODNVIMAGE_PARTITION_SIZE := 5242880
BOARD_FLASH_BLOCK_SIZE := 4096
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_PRODNVIMAGE_FILE_SYSTEM_TYPE := ext4

TARGET_SYSTEMIMAGES_SPARSE_EXT_DISABLED := true
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := false
TARGET_CACHEIMAGES_SPARSE_EXT_DISABLED := false
TARGET_PRODNVIMAGES_SPARSE_EXT_DISABLED := true
