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

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <unistd.h>

#include <hardware/gralloc.h>
#include <cutils/native_handle.h>
#include <alloc_device.h>
#include <utils/Log.h>

/* NOTE:
 * If your framebuffer device driver is integrated with dma_buf, you will have to
 * change this IOCTL definition to reflect your integration with the framebuffer
 * device.
 * Expected return value is a structure filled with a file descriptor
 * backing your framebuffer device memory.
 */
struct fb_dmabuf_export {
    __u32 fd;
    __u32 flags;
};
//#define FBIOGET_DMABUF    _IOR('F', 0x21, struct fb_dmabuf_export)

#ifdef USE_3_FRAMEBUFFER
#define NUM_FB_BUFFERS 3
#else
#define NUM_FB_BUFFERS 2
#endif

typedef enum {
    MALI_YUV_NO_INFO,
    MALI_YUV_BT601_NARROW,
    MALI_YUV_BT601_WIDE,
    MALI_YUV_BT709_NARROW,
    MALI_YUV_BT709_WIDE,
} mali_gralloc_yuv_info;

enum {
    HAL_PIXEL_FORMAT_YCbCr_420_P    = 0x13,
    HAL_PIXEL_FORMAT_YCbCr_420_SP   = 0x19,
};

enum {
    GRALLOC_USAGE_OVERLAY_BUFFER    = 0x01000000,
    GRALLOC_USAGE_VIDEO_BUFFER      = 0x02000000,
    GRALLOC_USAGE_CAMERA_BUFFER     = 0x04000000,
};

struct private_handle_t;

struct private_module_t {
    gralloc_module_t base;

    private_handle_t *framebuffer;
    uint32_t fbFormat;
    uint32_t flags;
    uint32_t numBuffers;
    uint32_t bufferMask;
    pthread_mutex_t lock;
    pthread_mutex_t fd_lock;
    buffer_handle_t currentBuffer;
    int ion_client;
    struct fb_var_screeninfo info;
    struct fb_fix_screeninfo finfo;
    float xdpi;
    float ydpi;
    float fps;

    enum {
        // flag to indicate we'll post this buffer
        PRIV_USAGE_LOCKED_FOR_POST = 0x80000000
    };

    /* default constructor */
    private_module_t();
};

#ifdef __cplusplus
struct private_handle_t : public native_handle {
#else
struct private_handle_t {
    struct native_handle nativeHandle;
#endif /* __cplusplus */

    enum {
        PRIV_FLAGS_FRAMEBUFFER  = 0x00000001,
        PRIV_FLAGS_USES_UMP     = 0x00000002,
        PRIV_FLAGS_USES_ION     = 0x00000004,
        PRIV_FLAGS_USES_PHY     = 0x00000008,
        PRIV_FLAGS_NOT_OVERLAY  = 0x00000010,
#ifdef SPRD_DITHER_ENABLE
        PRIV_FLAGS_SPRD_DITHER  = 0x80000000,
#endif
    };

    enum {
        LOCK_STATE_WRITE        = 1 << 31,
        LOCK_STATE_MAPPED       = 1 << 30,
        LOCK_STATE_READ_MASK    = 0x3FFFFFFF,
    };

    // ints
    /*shared file descriptor for dma_buf sharing*/
    int share_fd;
    int magic;
    int flags;
    int usage;
    int size;
    int width;
    int height;
    int format;
    int stride;
    int base;
    int lockState;
    int writeOwner;
    int pid;

    mali_gralloc_yuv_info yuv_info;

    // Following members is for framebuffer only
    int fd;
    int offset;

    int phyaddr;

    int __ion_client_padding;
    struct ion_handle *ion_hnd;

#ifdef __cplusplus
    /*
     * We track the number of integers in the structure. There are 11 unconditional
     * integers (magic - pid, yuv_info, fd and offset). The GRALLOC_ARM_XXX_NUM_INTS
     * variables are used to track the number of integers that are conditionally
     * included.
     */
    int __sNumInts_padding;
    static const int sNumFds = 1;
    static const int sMagic = 0x3141592;

    private_handle_t(int flags, int usage, int size, int base, int lock_state) :
        share_fd(-1),
        magic(sMagic),
        flags(flags),
        usage(usage),
        size(size),
        width(0),
        height(0),
        format(0),
        stride(0),
        base(base),
        lockState(lock_state),
        writeOwner(0),
        pid(getpid()),
        yuv_info(MALI_YUV_NO_INFO),
        fd(0),
        offset(0),
        ion_hnd(NULL)
    {
        version = sizeof(native_handle);
        numFds = sNumFds;
        numInts = (sizeof(private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds;
    }

    private_handle_t(int flags, int usage, int size, int base, int lock_state, int fb_file, int fb_offset) :
        share_fd(-1),
        magic(sMagic),
        flags(flags),
        usage(usage),
        size(size),
        width(0),
        height(0),
        format(0),
        stride(0),
        base(base),
        lockState(lock_state),
        writeOwner(0),
        pid(getpid()),
        yuv_info(MALI_YUV_NO_INFO),
        fd(fb_file),
        offset(fb_offset),
        ion_hnd(NULL)
    {
        version = sizeof(native_handle);
        numFds = sNumFds;
        numInts = (sizeof(private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds;
    }

    ~private_handle_t()
    {
        magic = 0;
    }

    bool usesPhysicallyContiguousMemory()
    {
        return (flags & (PRIV_FLAGS_FRAMEBUFFER | PRIV_FLAGS_USES_PHY));
    }

    static int validate(const native_handle *h)
    {
        const private_handle_t *hnd = (const private_handle_t *)h;

        if (!hnd || hnd->version != sizeof(native_handle) || hnd->magic != sMagic)
            return -EINVAL;

        int numFds = sNumFds;
        int numInts = (sizeof(private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds;

        if (hnd->numFds != numFds || hnd->numInts != numInts)
            return -EINVAL;

        return 0;
    }

    static private_handle_t *dynamicCast(const native_handle *in)
    {
        if (validate(in) == 0)
            return (private_handle_t *) in;
        return NULL;
    }
#endif /* __cplusplus */
};

#endif /* GRALLOC_PRIV_H_ */
