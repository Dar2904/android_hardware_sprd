#
# Copyright (C) 2016 The Android Open Source Project
# Copyright (C) 2016 The CyanogenMod Project
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

LOCAL_PATH := $(call my-dir)

ifneq (,$(filter scx15 sc8830,$(TARGET_BOARD_PLATFORM)))

include_makefiles := \
	$(LOCAL_PATH)/thumbnail/Android.mk \

# TODO Sync sc8830 and scx15 encoder codes

ifeq (sc8830,$(TARGET_BOARD_PLATFORM))
include_makefiles += $(call all-named-subdir-makefiles,sc8830)
endif

ifeq (scx15,$(TARGET_BOARD_PLATFORM))
include_makefiles += $(call all-named-subdir-makefiles,scx15 sc8830/dec)
endif

include $(include_makefiles)

include_makefiles :=

endif
