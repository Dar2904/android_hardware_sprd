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
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>
/* SPRD: add for apct functions @{ */
#include <sys/time.h>
#include <time.h>
/* @} */
#include <GLES/gl.h>

#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"

#ifdef SPRD_DITHER_ENABLE
#include <image_dither.h>
#endif

// numbers of buffers for page flipping
#define NUM_BUFFERS NUM_FB_BUFFERS
//#define DEBUG_FB_POST

static int swapInterval = 1;

enum {
    PAGE_FLIP = 0x00000001,
};

#ifdef SPRD_DITHER_ENABLE

struct dither_info {
    FILE*    fp;
    uint32_t alg_handle;
};

uint32_t dither_open(uint32_t w, uint32_t h)
{
    struct dither_info *dither = NULL;

    ALOGE("dither: open: %dx%d", w, h);

    dither = (struct dither_info *)malloc(sizeof(struct dither_info));
    if (NULL != dither) {
        int ret = 0;
        struct img_dither_init_in_param init_in;
        struct img_dither_init_out_param init_out;
        FILE* fp = NULL;

        memset(dither, 0, sizeof(struct dither_info));

        fp = fopen("/sys/module/mali/parameters/gpu_cur_freq", "r");
        if(fp == NULL) {
            AERR("can not open /sys/module/mali/parameters/gpu_cur_freq %x", fp);
            free (dither);
            dither = NULL;
            return 0;
        }

        dither->fp = fp;
        init_in.alg_id = 0;
        init_in.height = h; //m->info.yres;
        init_in.width = w; //m->info.xres;

        ret = img_dither_init(&init_in, &init_out);
        if (0 != ret || 0 == init_out.param) {
            if(dither->fp) {
                fclose(dither->fp);
            }
            free (dither);
            dither = NULL;
            ALOGE("dither: init failed ,ret = 0x%x", ret);
            return 0;
        }

        dither->alg_handle = (uint32_t)init_out.param;

        AINF("dither open ID %i, handle = 0x%x\n", 1, dither->alg_handle);

    }
    else {
        ALOGE("dither: dither_open failed!");
    }

    return (uint32_t)dither;
}

void dither_close(uint32_t handle)
{
    if (NULL != handle)
    {
        struct dither_info *dither = (struct dither_info *)handle;

        img_dither_deinit(dither->alg_handle);
        dither->alg_handle = NULL;
        if(dither->fp) {
            fclose(dither->fp);
        }
        free((void *)handle);
    }
}
#endif

static int fb_set_swap_interval(struct framebuffer_device_t *dev, int interval)
{
    if (interval < dev->minSwapInterval)
        interval = dev->minSwapInterval;
    else if (interval > dev->maxSwapInterval)
        interval = dev->maxSwapInterval;

    swapInterval = interval;

    return 0;
}

static int fb_setUpdateRect(struct framebuffer_device_t* dev,
        int l, int t, int w, int h)
{
    if ((( w | h) <= 0) || ((l | t) < 0))
        return -EINVAL;

    private_module_t* m = reinterpret_cast<private_module_t*>(
        dev->common.module);
    m->info.reserved[0] = 0x6f766572; // "UPDT";
    m->info.reserved[1] = (uint16_t)l | ((uint32_t)t << 16);
    m->info.reserved[2] = (uint16_t)w | ((uint32_t)h << 16);

    return 0;
}

/* SPRD: add for apct functions @{ */
static void writeFpsToProc(float fps)
{
    char fps_buf[256] = {0};
    char *fps_proc = "/proc/benchMark/fps";
    int fpsInt = (int)(fps+0.5);
    
    sprintf(fps_buf, "fps:%d", fpsInt);
       
    FILE *f = fopen(fps_proc, "r+w");
    if (NULL != f) {
        fseek(f, 0, 0);
        fwrite(fps_buf, strlen(fps_buf), 1, f);
        fclose(f);
    }
}
  
static int64_t systemTime()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec*1000000000LL + t.tv_nsec;
}

bool gIsApctFpsShow = false;
bool gIsApctRead  = false;
bool getApctFpsSupport()
{
    if (gIsApctRead)
        return gIsApctFpsShow;
    gIsApctRead = true;

    char str[10] = {'\0'};
    char *FILE_NAME = "/data/data/com.sprd.APCT/apct/apct_support";

    FILE *f = fopen(FILE_NAME, "r");

    if (NULL != f) {
        fseek(f, 0, 0);
        fread(str, 5, 1, f);
        fclose(f);

        long apct_config = atol(str);

        gIsApctFpsShow = ((apct_config & 0x8002) == 0x8002);
    }
    return gIsApctFpsShow;
}
/* @} */

#ifdef SPRD_DITHER_ENABLE
static bool fb_is_dither_enable(struct dither_info *dither, private_handle_t const* hnd)
{
    char buf[16] = "312000";
    FILE *fp = dither->fp;

    if(fp == NULL)
        AERR("can not open /sys/module/mali/parameters/gpu_cur_freq %x", fp);
    else {
        fseek(fp, 0, SEEK_SET);
        fread(buf, 1, 8, dither->fp);
    }

    int gpu_cur_freq = atoi(buf);
    if(gpu_cur_freq <= 256000) {
        if(hnd->flags & private_handle_t::PRIV_FLAGS_SPRD_DITHER)
            return true;
    }
    return false;
}
#endif

static int frame_count_fbpost = 0;
static int fb_post(struct framebuffer_device_t* dev, buffer_handle_t buffer)
{
    if (private_handle_t::validate(buffer) < 0)
        return -EINVAL;

    /* SPRD: add for apct functions @{ */
    static int64_t now = 0, last = 0;
    static int flip_count = 0;

    if (getApctFpsSupport()) {
        flip_count++;
        now = systemTime();
        if ((now - last) >= 1000000000LL) {
            float fps = flip_count*1000000000.0f / (now - last);
            writeFpsToProc(fps);
            flip_count = 0;
            last = now;
        }
    }
    /* @} */

    /*
      in surfaceflinger init process, first setTransactionState(...) will evoke a screen update which is not necessary
      here we just skip this black frame
    */
    if(frame_count_fbpost < 1) {
        frame_count_fbpost++;
        return 0;
    }

    private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(buffer);
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);

#ifdef DEBUG_FB_POST
    AINF("%s in line=%d\n", __FUNCTION__, __LINE__);
#endif
    if (m->currentBuffer) {
        m->base.unlock(&m->base, m->currentBuffer);
        m->currentBuffer = 0;
    }

    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER) {
        m->base.lock(&m->base, buffer, private_module_t::PRIV_USAGE_LOCKED_FOR_POST,
                0, 0, m->info.xres, m->info.yres, NULL);

        const size_t offset = hnd->base - m->framebuffer->base;
        int interrupt;
        m->info.activate = FB_ACTIVATE_VBL;
        m->info.yoffset = offset / m->finfo.line_length;

#ifdef SPRD_DITHER_ENABLE
        struct dither_info *dither = (struct dither_info *)dev->reserved[6];

        if (NULL != dither) {
            if(fb_is_dither_enable(dither, hnd)) {
                struct img_dither_in_param in_param;
                struct img_dither_out_param out_param;
                uint32_t dither_handle = 0;

                dither_handle = dither->alg_handle;
                in_param.alg_id = 0;
                in_param.data_addr = (void*)(hnd->base);
                in_param.format = 0;
                in_param.height =  m->info.yres;
                in_param.width =  m->info.xres;
                img_dither_process(dither_handle, &in_param, &out_param);
                m->info.reserved[3] = 1;
            }
        } else {
            m->info.reserved[3] = 0;
        }
#else
        m->info.reserved[3] = 0;
#endif

#ifdef STANDARD_LINUX_SCREEN
#define FBIO_WAITFORVSYNC       _IOW('F', 0x20, __u32)
#define S3CFB_SET_VSYNC_INT _IOW('F', 206, unsigned int)
        if (ioctl(m->framebuffer->fd, FBIOPAN_DISPLAY, &m->info) == -1) {
            AERR("FBIOPAN_DISPLAY failed for fd: %d", m->framebuffer->fd);
            m->base.unlock(&m->base, buffer);
            return 0;
        }
        if (swapInterval == 1) {
            // enable VSYNC
            interrupt = 1;

            if (ioctl(m->framebuffer->fd, S3CFB_SET_VSYNC_INT, &interrupt) < 0) {
                AERR("S3CFB_SET_VSYNC_INT enable failed for fd: %d", m->framebuffer->fd);
                return 0;
            }
            // wait for VSYNC
            int crtc = 0;

            if (ioctl(m->framebuffer->fd, FBIO_WAITFORVSYNC, &crtc) < 0) {
                AERR("FBIO_WAITFORVSYNC failed for fd: %d", m->framebuffer->fd);
                return 0;
            }
            // disable VSYNC
            interrupt = 0;

            if (ioctl(m->framebuffer->fd, S3CFB_SET_VSYNC_INT, &interrupt) < 0) {
                AERR("S3CFB_SET_VSYNC_INT disable failed for fd: %d", m->framebuffer->fd);
                return 0;
            }
        }
#else
        /*Standard Android way*/
#ifdef FB_FORMAT_SWITCH
        if(dev->format==HAL_PIXEL_FORMAT_RGB_565) {
            m->info.bits_per_pixel = 16;
            m->info.red.offset     = 11;
            m->info.red.length     = 5;
            m->info.green.offset   = 5;
            m->info.green.length   = 6;
            m->info.blue.offset    = 0;
            m->info.blue.length    = 5;
            m->info.transp.offset  = 0;
            m->info.transp.length  = 0;
        } else {
            m->info.bits_per_pixel = 32;
            m->info.red.offset     = 0;
            m->info.red.length     = 8;
            m->info.green.offset   = 8;
            m->info.green.length   = 8;
            m->info.blue.offset    = 16;
            m->info.blue.length    = 8;
            m->info.transp.offset  = 24;
            m->info.transp.length  = 0;
        }
#endif
        if (ioctl(m->framebuffer->fd, FBIOPUT_VSCREENINFO, &m->info) == -1) {
            AERR("FBIOPUT_VSCREENINFO failed for fd: %d", m->framebuffer->fd);
            m->base.unlock(&m->base, buffer);
            return -errno;
        }
#endif
        m->currentBuffer = buffer;
    } else {
        void *fb_vaddr;
        void *buffer_vaddr;

        m->base.lock(&m->base, m->framebuffer, GRALLOC_USAGE_SW_WRITE_RARELY,
                     0, 0, m->info.xres, m->info.yres, &fb_vaddr);

        m->base.lock(&m->base, buffer, GRALLOC_USAGE_SW_READ_RARELY,
                     0, 0, m->info.xres, m->info.yres, &buffer_vaddr);

        memcpy(fb_vaddr, buffer_vaddr, m->finfo.line_length * m->info.yres);

        m->base.unlock(&m->base, buffer);
        m->base.unlock(&m->base, m->framebuffer);
    }

#ifdef DEBUG_FB_POST
    AINF("%s out line=%d\n", __FUNCTION__, __LINE__);
#endif
    return 0;
}

int init_frame_buffer_locked(struct private_module_t *module)
{
    if (module->framebuffer)
        return 0; // Nothing to do, already initialized

    char const *const device_template[] = {
        "/dev/graphics/fb%u",
        "/dev/fb%u",
        NULL
    };

    int fd = -1;
    int i = 0;
    char name[64];

    while ((fd == -1) && device_template[i]) {
        snprintf(name, 64, device_template[i], 0);
        fd = open(name, O_RDWR, 0);
        i++;
    }

    if (fd < 0)
        return -errno;

    struct fb_fix_screeninfo finfo;

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        close(fd);
        return -errno;
    }

    struct fb_var_screeninfo info;

    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1) {
        close(fd);
        return -errno;
    }

    info.reserved[0] = 0;
    info.reserved[1] = 0;
    info.reserved[2] = 0;
    info.reserved[3] = 0;
    info.xoffset = 0;
    info.yoffset = 0;
    info.activate = FB_ACTIVATE_NODISP;

    char value[PROPERTY_VALUE_MAX];
    property_get("ro.sf.lcd_width", value, "1");
    info.width = atoi(value);
    property_get("ro.sf.lcd_height", value, "1");
    info.height = atoi(value);

    if(info.bits_per_pixel == 16) {
        /*
         * Explicitly request 5/6/5
         */
        info.bits_per_pixel = 16;
        info.red.offset     = 11;
        info.red.length     = 5;
        info.green.offset   = 5;
        info.green.length   = 6;
        info.blue.offset    = 0;
        info.blue.length    = 5;
        info.transp.offset  = 0;
        info.transp.length  = 0;

        module->fbFormat = HAL_PIXEL_FORMAT_RGB_565;
    } else {
        /*
         * Explicitly request 8/8/8
         */
        info.bits_per_pixel = 32;
        info.red.offset     = 0;
        info.red.length     = 8;
        info.green.offset   = 8;
        info.green.length   = 8;
        info.blue.offset    = 16;
        info.blue.length    = 8;
        info.transp.offset  = 24;
        info.transp.length  = 0;

        module->fbFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    }

    /*
     * Request NUM_BUFFERS screens (at lest 2 for page flipping)
     */
    info.yres_virtual = info.yres * NUM_BUFFERS;

    uint32_t flags = PAGE_FLIP;

    if (ioctl(fd, FBIOPUT_VSCREENINFO, &info) == -1) {
        info.yres_virtual = info.yres;
        flags &= ~PAGE_FLIP;
        AWAR("FBIOPUT_VSCREENINFO failed, page flipping not supported fd: %d", fd);
    }

    if (info.yres_virtual < info.yres * 2) {
        // we need at least 2 for page-flipping
        info.yres_virtual = info.yres;
        flags &= ~PAGE_FLIP;
        AWAR("page flipping not supported (yres_virtual=%d, requested=%d)", info.yres_virtual, info.yres * 2);
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1) {
        close(fd);
        return -errno;
    }

    int refreshRate = 0;

    if (info.pixclock > 0) {
        refreshRate = 1000000000000000LLU /
                      (
                          uint64_t(info.upper_margin + info.lower_margin + info.yres + info.hsync_len)
                          * (info.left_margin  + info.right_margin + info.xres + info.vsync_len)
                          * info.pixclock
                     );
    } else {
        AWAR("fbdev pixclock is zero for fd: %d", fd);
    }

    if (refreshRate == 0)
        refreshRate = 60 * 1000; // 60 Hz

    if (int(info.width) <= 0 || int(info.height) <= 0) {
        // the driver doesn't return that information
        // default to 160 dpi
        info.width  = ((info.xres * 25.4f) / 160.0f + 0.5f);
        info.height = ((info.yres * 25.4f) / 160.0f + 0.5f);
    }

    float xdpi = (info.xres * 25.4f) / info.width;
    float ydpi = (info.yres * 25.4f) / info.height;
    float fps  = refreshRate / 1000.0f;

    AINF("using (fd=%d)\n"
         "id           = %s\n"
         "xres         = %d px\n"
         "yres         = %d px\n"
         "xres_virtual = %d px\n"
         "yres_virtual = %d px\n"
         "bpp          = %d\n"
         "r            = %2u:%u\n"
         "g            = %2u:%u\n"
         "b            = %2u:%u\n",
         fd,
         finfo.id,
         info.xres,
         info.yres,
         info.xres_virtual,
         info.yres_virtual,
         info.bits_per_pixel,
         info.red.offset, info.red.length,
         info.green.offset, info.green.length,
         info.blue.offset, info.blue.length);

    AINF("width        = %d mm (%f dpi)\n"
         "height       = %d mm (%f dpi)\n"
         "refresh rate = %.2f Hz\n",
         info.width,  xdpi,
         info.height, ydpi,
         fps);

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        close(fd);
        return -errno;
    }

    if (finfo.smem_len <= 0) {
        close(fd);
        return -errno;
    }

    module->flags = flags;
    module->info = info;
    module->finfo = finfo;
    module->xdpi = xdpi;
    module->ydpi = ydpi;
    module->fps = fps;

    /*
     * map the framebuffer
     */
    size_t fbSize = round_up_to_page_size(finfo.line_length * info.yres_virtual);
    void *vaddr = mmap(0, fbSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (vaddr == MAP_FAILED) {
        close(fd);
        AERR("Error mapping the framebuffer (%s)", strerror(errno));
        return -errno;
    }

    memset(vaddr, 0, fbSize);

    // Create a "fake" buffer object for the entire frame buffer memory, and store it in the module
    module->framebuffer = new private_handle_t(
            private_handle_t::PRIV_FLAGS_FRAMEBUFFER,
            0,
            fbSize,
            intptr_t(vaddr),
            0,
            dup(fd),
            0);

    close(fd);
    module->numBuffers = info.yres_virtual / info.yres;
    module->bufferMask = 0;

    return 0;
}

static int init_frame_buffer(struct private_module_t *module)
{
    pthread_mutex_lock(&module->lock);
    int err = init_frame_buffer_locked(module);
    pthread_mutex_unlock(&module->lock);
    return err;
}

static int fb_close(struct hw_device_t *device)
{
    framebuffer_device_t *dev = reinterpret_cast<framebuffer_device_t *>(device);

#ifdef SPRD_DITHER_ENABLE
    if (dev->reserved[6]) {
        int ret = 0;
        dither_close(dev->reserved[6]);
        dev->reserved[6] = 0;
        AINF("dither close ID %i\n", 1);
    }
#endif

    if (dev) {
#if 0
        free(dev);
#endif
    }

    return 0;
}

int compositionComplete(struct framebuffer_device_t *dev)
{
    /* By doing a finish here we force the GL driver to start rendering
       all the drawcalls up to this point, and to wait for the rendering to be complete.*/
    glFinish();
    /* The rendering of the backbuffer is now completed.
       When SurfaceFlinger later does a call to eglSwapBuffer(), the swap will be done
       synchronously in the same thread, and not asynchronoulsy in a background thread later.
       The SurfaceFlinger requires this behaviour since it releases the lock on all the
       SourceBuffers (Layers) after the compositionComplete() function returns.
       However this "bad" behaviour by SurfaceFlinger should not affect performance,
       since the Applications that render the SourceBuffers (Layers) still get the
       full renderpipeline using asynchronous rendering. So they perform at maximum speed,
       and because of their complexity compared to the Surface flinger jobs, the Surface flinger
       is normally faster even if it does everyhing synchronous and serial.
       */
    return 0;
}

int framebuffer_device_open(hw_module_t const *module, const char *name, hw_device_t **device)
{
    int status = -EINVAL;

    alloc_device_t *gralloc_device;
    status = gralloc_open(module, &gralloc_device);

    if (status < 0)
        return status;

    private_module_t *m = (private_module_t *)module;
    status = init_frame_buffer(m);

    if (status < 0) {
        gralloc_close(gralloc_device);
        return status;
    }

    /* initialize our state here */
    framebuffer_device_t *dev = (framebuffer_device_t *)malloc(sizeof(*dev));
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t *>(module);
    dev->common.close = fb_close;
    dev->setSwapInterval = fb_set_swap_interval;
    dev->post = fb_post;
    dev->setUpdateRect = 0;
    dev->compositionComplete = &compositionComplete;

    int stride = m->finfo.line_length / (m->info.bits_per_pixel >> 3);
    const_cast<uint32_t &>(dev->flags) = 0;
    const_cast<uint32_t &>(dev->width) = m->info.xres;
    const_cast<uint32_t &>(dev->height) = m->info.yres;
    const_cast<int &>(dev->stride) = stride;
    const_cast<int&>(dev->format) = m->fbFormat;
    const_cast<float &>(dev->xdpi) = m->xdpi;
    const_cast<float &>(dev->ydpi) = m->ydpi;
    const_cast<float &>(dev->fps) = m->fps;
    const_cast<int &>(dev->minSwapInterval) = 0;
    const_cast<int &>(dev->maxSwapInterval) = 1;
    *device = &dev->common;

#ifdef SPRD_DITHER_ENABLE
    uint32_t dither_handle = (uint32_t)dither_open(m->info.xres, m->info.yres);
    if (dither_handle > 0) {
        dev->reserved[6] = dither_handle;
    } else {
        dev->reserved[6] = 0;
        ALOGE("dither: dither open failed!");
    }
#endif

    status = 0;
    return status;
}
