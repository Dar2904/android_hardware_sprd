/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Copyright (C) 2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "alloc_device.h"
#include "framebuffer_device.h"

extern int gralloc_register_buffer(gralloc_module_t const *module, buffer_handle_t handle);

extern int gralloc_unregister_buffer(gralloc_module_t const *module, buffer_handle_t handle);

extern int gralloc_lock(
        gralloc_module_t const *module,
        buffer_handle_t handle,
        int usage,
        int l,
        int t,
        int w,
        int h,
        void **vaddr);

extern int gralloc_lock_ycbcr(
        struct gralloc_module_t const* module,
        buffer_handle_t handle,
        int usage,
        int l,
        int t,
        int w,
        int h,
        struct android_ycbcr *ycbcr);

extern int gralloc_unlock(gralloc_module_t const* module, buffer_handle_t handle);

static int gralloc_device_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
    int status = -EINVAL;

    if (!strcmp(name, GRALLOC_HARDWARE_GPU0))
        status = alloc_device_open(module, name, device);
    else if (!strcmp(name, GRALLOC_HARDWARE_FB0))
        status = framebuffer_device_open(module, name, device);

    return status;
}

static struct hw_module_methods_t gralloc_module_methods = {
    .open = gralloc_device_open
};

private_module_t::private_module_t()
{
#define INIT_ZERO(obj) (memset(&(obj),0,sizeof((obj))))

    base.common.tag = HARDWARE_MODULE_TAG;
    base.common.version_major = 1;
    base.common.version_minor = 0;
    base.common.id = GRALLOC_HARDWARE_MODULE_ID;
    base.common.name = "Graphics Memory Allocator Module";
    base.common.author = "ARM Ltd.";
    base.common.methods = &gralloc_module_methods;
    base.common.dso = NULL;
    INIT_ZERO(base.common.reserved);

    base.registerBuffer = gralloc_register_buffer;
    base.unregisterBuffer = gralloc_unregister_buffer;
    base.lock = gralloc_lock;
    base.lock_ycbcr = gralloc_lock_ycbcr;
    base.unlock = gralloc_unlock;
    INIT_ZERO(base.reserved_proc);

    framebuffer = NULL;
    fbFormat = 0;
    flags = 0;
    numBuffers = 0;
    bufferMask = 0;
    pthread_mutex_init(&(lock), NULL);
    pthread_mutex_init(&(fd_lock), NULL);
    currentBuffer = NULL;
    INIT_ZERO(info);
    INIT_ZERO(finfo);
    xdpi = 0.0f;
    ydpi = 0.0f;
    fps = 0.0f;

#undef INIT_ZERO
};

struct private_module_t HAL_MODULE_INFO_SYM;

