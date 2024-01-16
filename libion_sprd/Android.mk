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

include $(CLEAR_VARS)

LOCAL_MODULE := libion_sprd

LOCAL_ADDITIONAL_DEPENDENCIES += \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/kernel-headers \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \

LOCAL_EXPORT_C_INCLUDE_DIRS := \
	$(LOCAL_C_INCLUDES) \

LOCAL_SRC_FILES := \
	ion.c \

LOCAL_MODULE_TAGS := \
	optional \

LOCAL_SHARED_LIBRARIES := \
	liblog \

LOCAL_POST_INSTALL_CMD := \
	$(hide) mkdir -p $(TARGET_OUT_SHARED_LIBRARIES); \
	ln -sf $(LOCAL_MODULE).so $(TARGET_OUT_SHARED_LIBRARIES)/libion.so

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := iontest_sprd

LOCAL_ADDITIONAL_DEPENDENCIES += \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/kernel-headers \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \

LOCAL_SRC_FILES := \
	ion.c \
	ion_test.c \

LOCAL_MODULE_TAGS := \
	optional \
	tests \

LOCAL_SHARED_LIBRARIES := \
	liblog \

include $(BUILD_EXECUTABLE)
