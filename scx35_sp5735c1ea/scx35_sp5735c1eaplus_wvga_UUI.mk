include device/sprd/scx35_sp5735c1ea/scx35_sp5735c1eaplus.mk

PRODUCT_THEME_PACKAGES := SimpleStyle
PRODUCT_THEME_FLAGS := shrink

PRODUCT_NAME := scx35_sp5735c1eaplus_wvga_UUI

DEVICE_PACKAGE_OVERLAYS := $(PLATDIR)/overlay_full $(BOARDDIR)/overlay $(PLATDIR)/overlay

$(call inherit-product-if-exists, vendor/sprd/UniverseUI/ThemeRes/universeui.mk)
$(call inherit-product, vendor/sprd/partner/shark/bluetooth/device-shark-bt.mk)

# SprdLauncher1
PRODUCT_PACKAGES += \
         SprdLauncher1 \
         SplitLauncher

PRODUCT_LOCALES := zh_CN zh_TW en_US

ifeq ($(MULTILANGUAGE_SUPPORT),true)
PRODUCT_PROPERTY_OVERRIDES += \
        ro.multiwindow.support=1 \
        ro.device.support.geocode=false

PRODUCT_LOCALES :=en_US en_AU en_CA en_GB en_IE en_IN en_NZ en_SG en_ZA zh_CN zh_TW ar_EG ar_IL fa_AF fa_IR ru_RU fr_BE fr_CA fr_CH fr_FH ro_RO ms_MY bn_BD pt_BR pt_PT sw_KE sw_TZ tl_PH th_TH tr_TR es_ES es_US hi_IN in_ID id_ID vi_VN ha_GH ha_NE ha_NG my_MM ce_PH te_IN ta_IN ug_CN ur_IN ur_PK am_ET pl_PL bo_CN bo_IN da_DK de_AT de_CH de_DE de_LI gu_IN nl_BE nl_NL km_KH cs_CZ lo_LA pa_IN pa_PK sl_SI uk_UA el_GR it_CH it_IT ka_CE kk_KZ ko_KR hr_HR lv_LV

#Default language when first Boot
PRODUCT_PROPERTY_OVERRIDES += ro.product.locale.language=en
PRODUCT_PROPERTY_OVERRIDES += ro.product.locale.region=US
endif

