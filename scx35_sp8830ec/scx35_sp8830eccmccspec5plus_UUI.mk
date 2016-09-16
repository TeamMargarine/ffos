-include vendor/sprd/operator/cmcc/specA/res/boot/boot_res_qhd.mk

# export original make file
LOCAL_ORIGINAL_PRODUCT_MAKEFILE := device/sprd/scx35_sp8830ec/scx35_sp8830ecplus.mk

include $(LOCAL_ORIGINAL_PRODUCT_MAKEFILE)
# Rename product name and we can lunch it
PRODUCT_NAME := scx35_sp8830eccmccspec5plus_UUI

PRODUCT_PROPERTY_OVERRIDES += \
        ro.homekey.physical=true \
        persist.sys.support.vt=false \
        persist.surpport.50ksearch=0

PRODUCT_PACKAGES += \
    SprdLauncher1

DEVICE_PACKAGE_OVERLAYS := $(PLATDIR)/overlay_full $(BOARDDIR)/overlay $(PLATDIR)/overlay

include vendor/sprd/UniverseUI/ThemeRes/universeui.mk
include vendor/sprd/operator/cmcc/spec5.mk

$(call inherit-product, vendor/sprd/partner/shark/bluetooth/device-shark-bt.mk)

#security support config
$(call inherit-product-if-exists, vendor/sprd/open-source/security_support.mk)

