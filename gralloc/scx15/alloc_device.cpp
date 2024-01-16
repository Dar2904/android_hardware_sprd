/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Copyright (C) 2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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

#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <linux/ion.h>
#include <ion/ion.h>
#include <video/ion_sprd.h>

#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"

#define ION_DEVICE "/dev/ion"

#define GRALLOC_ALIGN(value, base) (((value) + ((base) - 1)) & ~((base) - 1))

static int gralloc_alloc_buffer(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle) {
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
    struct ion_handle *ion_hnd;
    void *cpu_ptr;
    int shared_fd;
    int ret;
    int ion_heap_mask = 0;
    int ion_flag = 0;
    private_handle_t *hnd = NULL;

    if (usage & (GRALLOC_USAGE_VIDEO_BUFFER | GRALLOC_USAGE_CAMERA_BUFFER))
        ion_heap_mask = ION_HEAP_ID_MASK_MM;
    else if(usage & GRALLOC_USAGE_OVERLAY_BUFFER)
        ion_heap_mask = ION_HEAP_ID_MASK_OVERLAY;
    else
        ion_heap_mask = ION_HEAP_ID_MASK_SYSTEM;

    if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
        ion_flag = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

    ret = ion_alloc(m->ion_client, size, 0, ion_heap_mask, ion_flag, &ion_hnd);
    if (ret != 0) {
        AERR("Failed to ion_alloc from ion_client:%d", m->ion_client);
        return -1;
    }

    ret = ion_share(m->ion_client, ion_hnd, &shared_fd);
    if (ret != 0) {
        AERR("ion_share(%d) failed", m->ion_client);
        if (0 != ion_free(m->ion_client, ion_hnd))
            AERR("ion_free(%d) failed", m->ion_client);
        return -1;
    }

    // ion_hnd is no longer needed once we acquire shared_fd.
    if (0 != ion_free(m->ion_client, ion_hnd))
        AWAR("ion_free( %d ) failed", m->ion_client);
    ion_hnd = NULL;

    cpu_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);

    if (MAP_FAILED == cpu_ptr) {
        AERR("ion_map(%d) failed", m->ion_client);
        close(shared_fd);
        return -1;
    }

    hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION, usage, size,
            (int)cpu_ptr, private_handle_t::LOCK_STATE_MAPPED);
    if (NULL != hnd) {
        if(ion_heap_mask == ION_HEAP_CARVEOUT_MASK)
            hnd->flags = private_handle_t::PRIV_FLAGS_USES_ION | private_handle_t::PRIV_FLAGS_USES_PHY;
        ALOGV("the flag 0x%x and the vadress is 0x%x and the size is 0x%x", hnd->flags, (int)cpu_ptr, size);
        hnd->share_fd = shared_fd;
        *pHandle = hnd;
        ion_invalidate_fd(m->ion_client, hnd->share_fd);
        return 0;
    } else {
        AERR("Gralloc out of mem for ion_client:%d", m->ion_client);
        close(shared_fd);
        ret = munmap(cpu_ptr, size);
        if (0 != ret)
            AERR("munmap failed for base:%p size: %d", cpu_ptr, size);
        return -1;
    }
}

static int gralloc_alloc_framebuffer_locked(alloc_device_t* dev, size_t size,
                                            int usage, buffer_handle_t* pHandle)
{
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);

    // allocate the framebuffer
    if (m->framebuffer == NULL) {
        // initialize the framebuffer, the framebuffer is mapped once and forever.
        int err = init_frame_buffer_locked(m);
        if (err < 0)
            return err;
    }

    uint32_t bufferMask = m->bufferMask;
    const uint32_t numBuffers = m->numBuffers;
    const size_t bufferSize = m->finfo.line_length * m->info.yres;
    if (numBuffers == 1) {
        // If we have only one buffer, we never use page-flipping. Instead,
        // we return a regular buffer which will be memcpy'ed to the main
        // screen when post is called.
        int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
        AERR("fallback to single buffering. Virtual Y-res too small %d", m->info.yres);
        return gralloc_alloc_buffer(dev, bufferSize, newUsage, pHandle);
    }

    if (bufferMask >= ((1LU << numBuffers) - 1)) {
        // We ran out of buffers, reset bufferMask.
        bufferMask = 0;
        m->bufferMask = 0;
    }

    int vaddr = m->framebuffer->base;
    // find a free slot
    for (uint32_t i = 0; i < numBuffers; i++) {
        if ((bufferMask & (1LU << i)) == 0) {
            m->bufferMask |= (1LU << i);
            break;
        }
        vaddr += bufferSize;
    }

    // The entire framebuffer memory is already mapped, now create a buffer object for parts of this memory
    private_handle_t* hnd = new private_handle_t(
            private_handle_t::PRIV_FLAGS_FRAMEBUFFER,
            usage,
            size,
            vaddr,
            0,
            dup(m->framebuffer->fd),
            vaddr - m->framebuffer->base);
#ifdef FBIOGET_DMABUF
    struct fb_dmabuf_export fb_dma_buf;

    if (ioctl(m->framebuffer->fd, FBIOGET_DMABUF, &fb_dma_buf) == 0) {
        AINF("framebuffer accessed with dma buf (fd 0x%x)\n", (int)fb_dma_buf.fd);
        hnd->share_fd = fb_dma_buf.fd;
    }
#endif

    *pHandle = hnd;

    return 0;
}

static int gralloc_alloc_framebuffer(alloc_device_t* dev, size_t size, int usage,
                                        buffer_handle_t* pHandle)
{
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
    pthread_mutex_lock(&m->lock);
    int err = gralloc_alloc_framebuffer_locked(dev, size, usage, pHandle);
    pthread_mutex_unlock(&m->lock);
    return err;
}

static int alloc_device_alloc(alloc_device_t* dev, int w, int h, int format,
                                int usage, buffer_handle_t* pHandle, int* pStride)
{
    ALOGV("%s w:%d, h:%d, format:%d usage:0x%x start", __FUNCTION__, w, h, format, usage);
    if (!pHandle || !pStride)
        return -EINVAL;
    
    if(w < 1 || h < 1)
        return -EINVAL;

    size_t size;
    size_t stride;
    if (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED 
            || format == HAL_PIXEL_FORMAT_YCbCr_420_SP
            || format == HAL_PIXEL_FORMAT_YCrCb_420_SP
            || format == HAL_PIXEL_FORMAT_YV12) {
        switch (format) {
            case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            case HAL_PIXEL_FORMAT_YV12:
            case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
                stride = GRALLOC_ALIGN(w, 16);
                size = h * (stride + GRALLOC_ALIGN(stride / 2, 16));
                break;
            default:
                return -EINVAL;
        }
    } else if(format == HAL_PIXEL_FORMAT_BLOB) {
        stride = GRALLOC_ALIGN(w, 16);
        size = w * h;
    } else {
        int align = 8;
        int bpp = 0;
        switch (format) {
            case HAL_PIXEL_FORMAT_RGBA_8888:
            case HAL_PIXEL_FORMAT_RGBX_8888:
            case HAL_PIXEL_FORMAT_BGRA_8888:
                bpp = 4;
                break;
            case HAL_PIXEL_FORMAT_RGB_888:
                bpp = 3;
                break;
            case HAL_PIXEL_FORMAT_RGB_565:
                bpp = 2;
                break;
            default:
                return -EINVAL;
        }
        size_t bpr = GRALLOC_ALIGN(w * bpp, 8);
        size = bpr * h;
        stride = bpr / bpp;
    }
    int err;
    if (usage & GRALLOC_USAGE_HW_FB)
        err = gralloc_alloc_framebuffer(dev, size, usage, pHandle);
    else {
        err = gralloc_alloc_buffer(dev, size, usage, pHandle);
         if(err >= 0){
            const native_handle_t *p_nativeh = *pHandle;
            private_handle_t *hnd = (private_handle_t*)p_nativeh;
            hnd->format = format;
            hnd->width = stride;
            hnd->height = h;
        }
    }

    ALOGV("%s handle:0x%x end err is %d", __FUNCTION__, (unsigned int)*pHandle, err);
    if (err < 0)
        return err;

    /* match the framebuffer format */
    if (usage & GRALLOC_USAGE_HW_FB)
        format = HAL_PIXEL_FORMAT_RGBA_8888;

    private_handle_t *hnd = (private_handle_t *)*pHandle;
    int private_usage = usage & (GRALLOC_USAGE_PRIVATE_0 | GRALLOC_USAGE_PRIVATE_1);

    switch (private_usage) {
        case 0:
            hnd->yuv_info = MALI_YUV_BT601_NARROW;
            break;
        case GRALLOC_USAGE_PRIVATE_1:
            hnd->yuv_info = MALI_YUV_BT601_WIDE;
            break;
        case GRALLOC_USAGE_PRIVATE_0:
            hnd->yuv_info = MALI_YUV_BT709_NARROW;
            break;
        case (GRALLOC_USAGE_PRIVATE_0 | GRALLOC_USAGE_PRIVATE_1):
            hnd->yuv_info = MALI_YUV_BT709_WIDE;
            break;
    }

    hnd->width = w;
    hnd->height = h;
    hnd->format = format;
    hnd->stride = stride;

    *pStride = stride;
    return 0;
}

static int alloc_device_free(alloc_device_t* dev, buffer_handle_t handle)
{
    ALOGV("%s buffer_handle_t:0x%x start",__FUNCTION__,(unsigned int)handle);
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(handle);
    ALOGV("%s buffer_handle_t:0x%x flags:0x%x  start",__FUNCTION__,(unsigned int)handle,hnd->flags);

    // we can't deallocate the memory in case of UNMAP failure
    // because it would give that process access to someone else's
    // surfaces, which would be a security breach.
    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER) {
        // free this buffer
        private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
        const size_t bufferSize = m->finfo.line_length * m->info.yres;
        int index = (hnd->base - m->framebuffer->base) / bufferSize;
        m->bufferMask &= ~(1 << index);
        close(hnd->fd);
    } else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP) {
        AERR("Can't free ump memory for handle:0x%x. Not supported.", (unsigned int)hnd);
    } else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION) {
        private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
        /* Buffer might be unregistered so we need to check for invalid ump handle*/
        if (0 != hnd->base) {
            ALOGV("%s the vadress 0x%x size of 0x%x will be free", __FUNCTION__, hnd->base, hnd->size);
            if (0 != munmap((void*)hnd->base, hnd->size))
                AERR("Failed to munmap handle 0x%x", (unsigned int)hnd);
        }
        close(hnd->share_fd);
        memset((void*)hnd, 0, sizeof(*hnd));
    }

    delete hnd;

    ALOGV("%s end", __FUNCTION__);
    return 0;
}

static int alloc_device_close(struct hw_device_t *device)
{
    alloc_device_t* dev = reinterpret_cast<alloc_device_t*>(device);
    if (dev) {
        private_module_t *m = reinterpret_cast<private_module_t*>(device);
        if (0 != ion_close(m->ion_client))
            AERR("Failed to close ion_client: %d", m->ion_client);
        close(m->ion_client);
        delete dev;
    }
    return 0;
}

int alloc_device_open(hw_module_t const* module, const char* name, hw_device_t** device)
{
    alloc_device_t *dev;

    dev = new alloc_device_t;
    if (NULL == dev)
        return -1;

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = alloc_device_close;
    dev->alloc = alloc_device_alloc;
    dev->free = alloc_device_free;

    private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
    m->ion_client = ion_open();
    if (m->ion_client < 0) {
        AERR("ion_open failed with %s", strerror(errno));
        delete dev;
        return -1;
    }
    
    *device = &dev->common;

    return 0;
}
