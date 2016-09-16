PRODUCT_COPY_FILES += \
	$(PLATDIR)/nand/fstab.sc8830:root/fstab.sc8830
# SPRD:add for mount cache to sdcard @{
PRODUCT_PACKAGES += \
    mke2fs \
    mkcached.sh \
	busybox
# @}

PRODUCT_PROPERTY_OVERRIDES += \
	ro.storage.flash_type=1
