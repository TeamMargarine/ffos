
# include aosp base configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)

# sprd telephony
PRODUCT_PACKAGES += \
	Dialer \
	Mms \
	Stk

# graphics modules
PRODUCT_PACKAGES += \
	libEGL_mali.so \
	libGLESv1_CM_mali.so \
	libGLESv2_mali.so \
	libMali.so \
	libboost.so \
	mali.ko

# video modules
PRODUCT_PACKAGES += \
	libstagefright_sprd_soft_mpeg4dec \
	libstagefright_sprd_soft_h264dec \
	libstagefright_sprd_mpeg4dec \
	libstagefright_sprd_mpeg4enc \
	libstagefright_sprd_h264dec \
	libstagefright_sprd_h264enc \
	libstagefright_sprd_vpxdec \
	libstagefright_soft_mjpgdec \
	libstagefright_soft_imaadpcmdec \
	libstagefright_sprd_aacdec \
	libstagefright_sprd_mp3dec \
	libstagefright_sprd_apedec

# default audio
PRODUCT_PACKAGES += \
	audio.a2dp.default \
	audio.usb.default \
	audio.r_submix.default \
	libaudio-resampler

# sprd HAL modules
PRODUCT_PACKAGES += \
	audio.primary.sc8830 \
	audio_policy.sc8830 \
	gralloc.sc8830 \
	camera.sc8830 \
	camera2.sc8830 \
	lights.sc8830 \
	hwcomposer.sc8830 \
	sprd_gsp.sc8830
#	sensors.sc8830

# misc modules
PRODUCT_PACKAGES += \
	sqlite3 \
	charge \
	poweroff_alarm \
	mplayer \
	e2fsck \
	tinymix \
	audio_vbc_eq \
	calibration_init \
	modemd \
	engpc \
	modem_control\
	libengappjni \
	nvitemd \
	batterysrv \
	refnotify \
	wcnd \
	libsprdstreamrecoder \
	libvtmanager  \
	zram.sh \
	bdt

# general configs
PRODUCT_COPY_FILES += \
	$(PLATDIR)/init.sc8830.rc:root/init.sc8830.rc \
	$(PLATDIR)/init.sc8830.usb.rc:root/init.sc8830.usb.rc \
	$(PLATDIR)/ueventd.sc8830.rc:root/ueventd.sc8830.rc \
	$(PLATDIR)/headset-keyboard.kl:system/usr/keylayout/headset-keyboard.kl \
	$(PLATDIR)/sci-keypad.kl:system/usr/keylayout/sci-keypad.kl \
	$(PLATDIR)/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
	$(PLATDIR)/media_codecs.xml:system/etc/media_codecs.xml \
	$(PLATDIR)/media_profiles.xml:system/etc/media_profiles.xml \
        vendor/sprd/open-source/res/spn/spn-conf.xml:system/etc/spn-conf.xml \
	vendor/sprd/open-source/res/apn/apns-conf.xml:system/etc/apns-conf.xml \
	vendor/sprd/open-source/res/productinfo/productinfo.bin:prodnv/productinfo.bin \
	vendor/sprd/open-source/res/productinfo/default_connectivity_configure.ini:system/etc/connectivity_configure.ini \
	vendor/sprd/open-source/res/productinfo/connectivity_calibration.ini:prodnv/connectivity_calibration.ini \
	vendor/sprd/open-source/res/productinfo/connectivity_calibration.ini:system/etc/connectivity_calibration.ini \
	vendor/sprd/open-source/res/CDROM/adb.iso:system/etc/adb.iso \
	vendor/sprd/open-source/libs/mali/egl.cfg:system/lib/egl/egl.cfg \
	vendor/sprd/open-source/apps/scripts/ext_data.sh:system/bin/ext_data.sh \
	vendor/sprd/open-source/apps/scripts/ext_kill.sh:system/bin/ext_kill.sh \
	vendor/sprd/open-source/apps/scripts/inputfreq.sh:system/bin/inputfreq.sh \
	vendor/sprd/open-source/apps/scripts/recoveryfreq.sh:system/bin/recoveryfreq.sh \
	frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
	frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.xml:system/etc/permissions/android.hardware.touchscreen.xml \
	frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
	frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml

PRODUCT_PROPERTY_OVERRIDES += \
        ro.moz.omx.hw.max_width=1920 \
        ro.moz.omx.hw.max_height=1080 \
        ro.moz.wifi.unloaddriver=1 \
        ro.moz.ril.ipv6 = true

ifeq ($(TARGET_BUILD_VARIANT),user)

PRODUCT_PROPERTY_OVERRIDES += \
	persist.sys.sprd.modemreset=1 \
	ro.adb.secure=1 \
	persist.sys.sprd.wcnreset=1 \
        persist.sys.engpc.disable=1

else
PRODUCT_PROPERTY_OVERRIDES += \
	persist.sys.sprd.modemreset=0 \
	ro.adb.secure=0 \
	persist.sys.sprd.wcnreset=0 \
        persist.sys.engpc.disable=0

endif # TARGET_BUILD_VARIANT == user

# add GPS engineer mode apk
PRODUCT_PACKAGES += \
        SGPS

# Gecko/Gaia need below export value
ifeq ($(strip $(GAIA_APP_SRCDIRS)),)
export GAIA_APP_SRCDIRS=apps external-apps outoftree_apps customize_apps/engmode/assets customize_apps/ValidationTools_1.4/assets customize_apps/engSgps
endif

ifeq ($(strip $(PRODUCTION)),)
export PRODUCTION=1
endif

ifeq ($(strip $(MOZILLA_OFFICIAL)),)
export MOZILLA_OFFICIAL=1
endif

ifeq ($(strip $(GAIA_DISTRIBUTION_DIR)),)
export GAIA_DISTRIBUTION_DIR=$(PWD)/device/sprd/scx35_sp7730ec/
endif

ifeq ($(strip $(LOCALE_BASEDIR)),)
export LOCALE_BASEDIR=$(PWD)/gaia-l10n/
endif

ifeq ($(strip $(LOCALES_FILE)),)
export LOCALES_FILE=$(PWD)/device/sprd/scx35/languages.json
endif

ifeq ($(strip $(GAIA_DEFAULT_LOCALE)),)
export GAIA_DEFAULT_LOCALE=en-US
endif

ifeq ($(strip $(GAIA_KEYBOARD_LAYOUTS)),)
export GAIA_KEYBOARD_LAYOUTS=en,bn-Avro,bn-Probhat
endif

ifeq ($(strip $(NOFTU)),)
ifeq ($(TARGET_BUILD_VARIANT), user)
export NOFTU=0
else
export NOFTU=1
endif
endif

