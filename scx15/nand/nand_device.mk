PRODUCT_COPY_FILES += \
	$(PLATDIR)/nand/fstab.scx15:root/fstab.scx15
# SPRD:add for mount cache to sdcard @{
PRODUCT_PACKAGES += \
    mke2fs \
    mkcached.sh \
	busybox
# @}

PRODUCT_PROPERTY_OVERRIDES += \
	ro.storage.flash_type=1
