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

# SPRD: Add LowCost switcher for cutting rom size
ifneq ($(filter scx35_sp8830ga%, $(TARGET_PRODUCT)),)
  TARGET_LOWCOST_SUPPORT := true
endif

PRODUCT_MAKEFILES := \
	$(LOCAL_DIR)/scx35_sp8830gabase.mk \
	$(LOCAL_DIR)/scx35_sp8830gaplus.mk \
	$(LOCAL_DIR)/scx35_sp8830gacmccspecAplus_UUI.mk \
	$(LOCAL_DIR)/scx35_sp8830gacmccspec5plus_UUI.mk \
  $(LOCAL_DIR)/scx35_sp8830gacmccspec4plus_UUI.mk

