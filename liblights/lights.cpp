/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Copyright (C) 2016 The CyanogenMod Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "LIGHTS"
#include <cutils/log.h>
#include <hardware/lights.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <string>
#include <unordered_map>
#include <utility>

typedef enum {
    LIGHT_DEVICE_BACKLIGHT,
    LIGHT_DEVICE_BUTTONS,
    LIGHT_DEVICE_KEYBOARD,
    LIGHT_DEVICE_NOTIFICATIONS,
    LIGHT_DEVICE_ATTENTION,
} LIGHT_DEVICE_T_E;

struct priv_light_device_t {
    struct light_device_t device;
    LIGHT_DEVICE_T_E type;
    int fd;
    std::string name;
};

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static std::unordered_map<std::string, struct priv_light_device_t *> g_light_devices;

static void init_once(void)
{
    pthread_mutex_init(&g_lock, NULL);

#define INIT_LIGHT_DEVICE(TYPE, NAME, PATH)                                          \
    int NAME##_fd = open(PATH, O_RDWR);                                              \
    if (NAME##_fd == -1) {                                                           \
        ALOGE("Failed to open light device: path=%s errno=%d", PATH, errno);         \
    } else {                                                                         \
        struct priv_light_device_t *NAME##_priv_dev = (struct priv_light_device_t *) \
                malloc(sizeof(struct priv_light_device_t));                          \
        memset(NAME##_priv_dev, 0, sizeof(*NAME##_priv_dev));                        \
        NAME##_priv_dev->name = LIGHT_ID_##NAME;                                     \
        NAME##_priv_dev->fd = NAME##_fd;                                             \
        NAME##_priv_dev->type = TYPE;                                                \
        std::pair<std::string, struct priv_light_device_t *> pair(                   \
                LIGHT_ID_##NAME, NAME##_priv_dev);                                   \
        g_light_devices.insert(pair);                                                \
    }                                                                                \

#ifdef LIGHT_BACKLIGHT
    INIT_LIGHT_DEVICE(LIGHT_DEVICE_BACKLIGHT, BACKLIGHT, LIGHT_BACKLIGHT);
#endif
#ifdef LIGHT_BUTTONS
    INIT_LIGHT_DEVICE(LIGHT_DEVICE_BUTTONS, BUTTONS, LIGHT_BUTTONS);
#endif
#ifdef LIGHT_KEYBOARD
    INIT_LIGHT_DEVICE(LIGHT_DEVICE_KEYBOARD, KEYBOARD, LIGHT_KEYBOARD);
#endif
#ifdef LIGHT_NOTIFICATIONS
    INIT_LIGHT_DEVICE(LIGHT_DEVICE_NOTIFICATIONS, NOTIFICATIONS, LIGHT_NOTIFICATIONS);
#endif
#ifdef LIGHT_ATTENTION
    INIT_LIGHT_DEVICE(LIGHT_DEVICE_ATTENTION, ATTENTION, LIGHT_ATTENTION);
#endif

#undef INIT_LIGHT_DEVICE
}

static int rgb_to_brightness(struct light_state_t const *state)
{
    int color = state->color & 0x00ffffff;
    return ((77*((color>>16) & 0x00ff))
            + (150*((color>>8) & 0x00ff)) + (29*(color & 0x00ff))) >> 8;
}

static int is_lit(struct light_state_t const* state)
{
    return state->color & 0x00ffffff;
}

static int write_brightness(int fd, int brightness)
{
    char buffer[32] = { 0 };
    int bytes = snprintf(buffer, sizeof(buffer), "%d\n", brightness);
    int result = 0;
    if (write(fd, buffer, bytes) == -1)
        ALOGE("write_brightness: write error: %d", (result = -errno));
    return result;
}

static int set_lights(struct light_device_t *dev, struct light_state_t const *state)
{
    struct priv_light_device_t *priv_dev = (struct priv_light_device_t *)dev;
    int fd = priv_dev->fd;
    int brightness;
    int result = -EINVAL;
    switch (priv_dev->type) {
        case LIGHT_DEVICE_BACKLIGHT:
            brightness = rgb_to_brightness(state);
            break;
        case LIGHT_DEVICE_BUTTONS:
            brightness = is_lit(state) ? 255 : 0;
            break;
        case LIGHT_DEVICE_KEYBOARD:
            brightness = is_lit(state) ? 255 : 0;
            break;
        case LIGHT_DEVICE_NOTIFICATIONS:
            /* fall-through */
        case LIGHT_DEVICE_ATTENTION:
            /* fall-through */
        default:
            ALOGE("Unsupported light type: %d", priv_dev->type);
            goto err;
    }
    pthread_mutex_lock(&g_lock);
    result = write_brightness(fd, brightness);
    pthread_mutex_unlock(&g_lock);
err:
    return result;
}

static int close_lights(struct light_device_t *dev)
{
    struct priv_light_device_t *priv_dev = (struct priv_light_device_t *)dev;
    if (priv_dev) {
        if (priv_dev->fd != -1) {
            close(priv_dev->fd);
        }
        g_light_devices.erase(priv_dev->name);
        free(priv_dev);
    }
    return 0;
}

static int open_lights(const struct hw_module_t *module, char const *name,
                       struct hw_device_t **device)
{
    pthread_once(&g_init, init_once);

    auto search = g_light_devices.find(name);
    if (search == g_light_devices.end())
        return -EINVAL;

    struct light_device_t *dev = (struct light_device_t *)(*search).second;
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = (int (*)(struct hw_device_t *))close_lights;
    dev->set_light = set_lights;

    *device = (struct hw_device_t *)dev;

    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "lights Module",
    .author = "Google, Inc.",
    .methods = &lights_module_methods,
};
