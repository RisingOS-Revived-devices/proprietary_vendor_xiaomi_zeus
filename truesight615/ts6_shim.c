/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * libTrueSight (front camera beauty) ships GL program binaries built for
 * Adreno 0615.95, which newer drivers reject, leaving a green preview. Its
 * GL/EGL imports are renamed to ts6_* (see extract-files.py) and forwarded
 * here to a private, renamed copy of that driver in /vendor/lib64/camera,
 * while the rest of the system keeps the current driver.
 */

#include <dlfcn.h>
#include <stdatomic.h>
#include <stdint.h>

#include <android/log.h>

#include "ts6_names.h"

#define TAG "ts6"
#define DIR "/vendor/lib64/camera/"

__attribute__((visibility("hidden"))) void* ts6_ptrs[TS6_COUNT];

// Dependency order; the drivers dlopen() some of these by soname.
static const char* const kLibs[] = {
        DIR "libgs6.so",           DIR "libadreno_util6.so", DIR "libllvm-qg6.so",
        DIR "libllvm-glnex6.so",   DIR "libEGL_adren6.so",   DIR "eglSubDriverAndroi6.so",
        DIR "libGLESv2_adren6.so",
};
#define EGL_LIB 4
#define GLES_LIB 6

static atomic_bool loaded;

/*
 * Called on the first GL/EGL call, from the calling (render) thread, as the
 * system EGL loader would: the driver's compiler sets up per-thread state on
 * the thread that loads it and aborts on any other.
 */
__attribute__((visibility("hidden"))) void ts6_resolve(void) {
    if (atomic_load(&loaded)) return;

    void* handles[sizeof(kLibs) / sizeof(kLibs[0])];
    for (size_t i = 0; i < sizeof(kLibs) / sizeof(kLibs[0]); i++) {
        handles[i] = dlopen(kLibs[i], RTLD_NOW);
        if (!handles[i]) __android_log_print(ANDROID_LOG_ERROR, TAG, "%s", dlerror());
    }
    for (size_t i = 0; i < TS6_COUNT; i++) {
        void* p = handles[EGL_LIB] ? dlsym(handles[EGL_LIB], ts6_names[i]) : NULL;
        if (!p && handles[GLES_LIB]) p = dlsym(handles[GLES_LIB], ts6_names[i]);
        if (!p) __android_log_print(ANDROID_LOG_ERROR, TAG, "missing %s", ts6_names[i]);
        ts6_ptrs[i] = p;
    }
    atomic_store(&loaded, true);
}

/*
 * libEGL hands drivers GraphicBuffer::getNativeBuffer(). In a GraphicBuffer
 * the ANativeWindowBuffer base follows RefBase, so locate it by its header:
 * common.magic '_bfr' followed by common.version (its size).
 */
void* ts6_eglGetNativeClientBufferANDROID(const void* buffer) {
    const uint8_t* b = buffer;
    if (!b) return NULL;
    for (size_t off = 0; off <= 64; off += 8) {
        const int32_t* w = (const int32_t*)(b + off);
        if (w[0] == (('_' << 24) | ('b' << 16) | ('f' << 8) | 'r') && w[1] > 64 && w[1] < 1024)
            return (void*)(b + off);
    }
    __android_log_print(ANDROID_LOG_ERROR, TAG, "no ANativeWindowBuffer in %p", buffer);
    return NULL;
}
