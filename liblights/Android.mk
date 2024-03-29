#
# Copyright (C) 2008 The Android Open Source Project
#
# Copyright (C) 2016 The CyanogenMod Project
#
# Copyright (C) 2018 The LineageOS Project
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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := lights.cpp
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE := lights.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

# Set LIGHT_BACKLIGHT, LIGHT_BUTTONS, LIGHT_KEYBOARD, LIGHT_NOTIFICATIONS, LIGHT_ATTENTION
# for your board if it is supported here.

ifeq (scx15,$(TARGET_BOARD_PLATFORM))
LOCAL_CFLAGS += -DLIGHT_BACKLIGHT=\"/sys/class/backlight/panel/brightness\"
endif

ifeq (sc8810,$(TARGET_BOARD_PLATFORM))
LOCAL_CFLAGS += -DLIGHT_BACKLIGHT=\"/sys/class/backlight/panel/brightness\"
endif

ifeq (sc8830,$(TARGET_BOARD_PLATFORM))
LOCAL_CFLAGS += -DLIGHT_BACKLIGHT=\"/sys/class/backlight/panel/brightness\"
endif

include $(BUILD_SHARED_LIBRARY)
