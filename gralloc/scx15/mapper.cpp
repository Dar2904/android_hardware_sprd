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
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/ion.h>
#include <ion/ion.h>
#include <video/ion_sprd.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "alloc_device.h"
#include "framebuffer_device.h"

static pthread_mutex_t s_map_lock = PTHREAD_MUTEX_INITIALIZER;

int gralloc_register_buffer(gralloc_module_t const *module, buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0) {
        AERR("Registering invalid buffer 0x%x, returning error", (int)handle);
        return -EINVAL;
    }

    // if this handle was created in this process, then we keep it as is.
    private_handle_t *hnd = (private_handle_t *)handle;

    int retval = -EINVAL;

    pthread_mutex_lock(&s_map_lock);

    hnd->pid = getpid();

    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
        AERR("Can't register buffer 0x%x as it is a framebuffer", (unsigned int)handle);
    else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
        AERR("Gralloc does not support UMP. Unable to register UMP memory for handle 0x%x", (unsigned int)hnd);
    else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION) {
        int ret;
        unsigned char *mappedAddress;
        size_t size = hnd->size;
        hw_module_t *pmodule = NULL;
        private_module_t *m = NULL;

        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&pmodule) == 0)
            m = reinterpret_cast<private_module_t *>(pmodule);
        else {
            AERR("Could not get gralloc module for handle: 0x%x", (unsigned int)hnd);
            retval = -errno;
            goto cleanup;
        }

        /* the test condition is set to m->ion_client <= 0 here, because:
         * 1) module structure are initialized to 0 if no initial value is applied
         * 2) a second user process should get a ion fd greater than 0.
         */
        if (m->ion_client <= 0) {
            /* a second user process must obtain a client handle first via ion_open before it can obtain the shared ion buffer*/
            m->ion_client = ion_open();

            if (m->ion_client < 0) {
                AERR("Could not open ion device for handle: 0x%x", (unsigned int)hnd);
                retval = -errno;
                goto cleanup;
            }
        }

        mappedAddress = (unsigned char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, hnd->share_fd, 0);

        if (MAP_FAILED == mappedAddress) {
            AERR("mmap(share_fd:%d) failed with %s",  hnd->share_fd, strerror(errno));
            retval = -errno;
            goto cleanup;
        }

        hnd->base = intptr_t(mappedAddress) + hnd->offset;
        pthread_mutex_unlock(&s_map_lock);
        return 0;
    } else {
        AERR("registering non-UMP buffer not supported. flags = %d", hnd->flags);
    }

cleanup:
    pthread_mutex_unlock(&s_map_lock);
    return retval;
}

int gralloc_unregister_buffer(gralloc_module_t const *module, buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0) {
        AERR("unregistering invalid buffer 0x%x, returning error", (int)handle);
        return -EINVAL;
    }

    private_handle_t *hnd = (private_handle_t *)handle;

    AERR_IF(hnd->lockState & private_handle_t::LOCK_STATE_READ_MASK,
            "[unregister] handle %p still locked (state=%08x)", hnd, hnd->lockState);

    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
        AERR("Can't unregister buffer 0x%x as it is a framebuffer", (unsigned int)handle);
    else if (hnd->pid == getpid()) { // never unmap buffers that were not registered in this process
        pthread_mutex_lock(&s_map_lock);

        if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
            AERR("Can't unregister UMP buffer for handle 0x%x. Not supported", (unsigned int)handle);
        else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION) {
            void *base = (void *)hnd->base;
            size_t size = hnd->size;

            if (munmap(base, size) < 0)
                AERR("Could not munmap base:0x%x size:%d '%s'", (unsigned int)base, size, strerror(errno));
        } else {
            AERR("Unregistering unknown buffer is not supported. Flags = %d", hnd->flags);
        }

        hnd->base = 0;
        hnd->lockState = 0;
        hnd->writeOwner = 0;
        pthread_mutex_unlock(&s_map_lock);
    } else {
        AERR("Trying to unregister buffer 0x%x from process %d that was not created in current process: %d",
                (unsigned int)hnd, hnd->pid, getpid());
    }

    return 0;
}

int gralloc_lock(
        gralloc_module_t const *module,
        buffer_handle_t handle,
        int usage,
        int l,
        int t,
        int w,
        int h,
        void **vaddr)
{
    if (private_handle_t::validate(handle) < 0) {
        AERR("Locking invalid buffer 0x%x, returning error", (int)handle);
        return -EINVAL;
    }

    private_handle_t *hnd = (private_handle_t *)handle;

    if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP
            || hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
        hnd->writeOwner = usage & GRALLOC_USAGE_SW_WRITE_MASK;

    if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK)) {
        *vaddr = (void *)hnd->base;
        hw_module_t *pmodule = NULL;
        private_module_t *m = NULL;

        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&pmodule) == 0) {
            m = reinterpret_cast<private_module_t *>(pmodule);
            ion_invalidate_fd(m->ion_client, hnd->share_fd);
        } else {
            AERR("lock couldnot get gralloc module for handle 0x%x\n", (unsigned int)handle);
        }
    }

    return 0;
}

int gralloc_lock_ycbcr(
        struct gralloc_module_t const* module,
        buffer_handle_t handle,
        int usage,
        int l,
        int t,
        int w,
        int h,
        struct android_ycbcr *ycbcr)
{
    if (private_handle_t::validate(handle) < 0) {
        AERR("Locking invalid buffer 0x%x, returning error", (int)handle);
        return -EINVAL;
    }

    private_handle_t* hnd = (private_handle_t*)handle;
    int ystride;
    int err=0;

    switch (hnd->format) {
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            ystride = GRALLOC_ALIGN(hnd->width, 16);
            ycbcr->y  = (void*)hnd->base;
            ycbcr->cr = (void*)(hnd->base + ystride * hnd->height);
            ycbcr->cb = (void*)(hnd->base + ystride * hnd->height + 1);
            ycbcr->ystride = ystride;
            ycbcr->cstride = ystride;
            ycbcr->chroma_step = 2;
            memset(ycbcr->reserved, 0, sizeof(ycbcr->reserved));
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            ystride = GRALLOC_ALIGN(hnd->width, 16);
            ycbcr->y  = (void*)hnd->base;
            ycbcr->cb = (void*)(hnd->base + ystride * hnd->height);
            ycbcr->cr = (void*)(hnd->base + ystride * hnd->height + 1);
            ycbcr->ystride = ystride;
            ycbcr->cstride = ystride;
            ycbcr->chroma_step = 2;
            memset(ycbcr->reserved, 0, sizeof(ycbcr->reserved));
            break;
        default:
            ALOGD("%s: Invalid format passed: 0x%x", __FUNCTION__, hnd->format);
            err = -EINVAL;
    }

    return err;
}

int gralloc_unlock(gralloc_module_t const* module, buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0) {
        AERR("Unlocking invalid buffer 0x%x, returning error", (int)handle);
        return -EINVAL;
    }

    private_handle_t *hnd = (private_handle_t *)handle;
    int32_t current_value;
    int32_t new_value;
    int retry;

    if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP && hnd->writeOwner)
        AERR("Buffer 0x%x is UMP type but it is not supported", (unsigned int)hnd);
    else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION && hnd->writeOwner) {
        hw_module_t *pmodule = NULL;
        private_module_t *m = NULL;

        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&pmodule) == 0) {
            m = reinterpret_cast<private_module_t *>(pmodule);
            ion_sync_fd(m->ion_client, hnd->share_fd);
        } else {
            AERR("Unlock couldnot get gralloc module for handle 0x%x\n", (unsigned int)handle);
        }
    }

    return 0;
}
