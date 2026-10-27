#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#
# Rebuilds lib64/ from the Adreno 0615.95 stack in vendor/xiaomi/sm8450-common
# 5389f7a: git -C vendor/xiaomi/sm8450-common show 5389f7a:proprietary/vendor/lib64/<file>
# into SRC (egl/ files flattened), then: make_lib64.py SRC lib64
# Every soname/dlopen name is replaced by a same-length alias so the copy can
# live next to the system driver without either one resolving to the other.
import os
import subprocess
import sys

PATCHELF = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        '../../../../prebuilts/extract-tools/linux-x86/bin/patchelf-0_18')
DIR = '/vendor/lib64/camera/'


def elf_needed(path):
    out = subprocess.run([PATCHELF, '--print-needed', path], check=True,
                         capture_output=True, text=True).stdout
    return out.split()

src, dst = sys.argv[1], sys.argv[2]

NAMES = {
    'libEGL_adreno.so': 'libEGL_adren6.so',
    'libGLESv2_adreno.so': 'libGLESv2_adren6.so',
    'libGLESv1_CM_adreno.so': 'libGLESv1_CM_adren6.so',
    'eglSubDriverAndroid.so': 'eglSubDriverAndroi6.so',
    'libgsl.so': 'libgs6.so',
    'libllvm-glnext.so': 'libllvm-glnex6.so',
    'libllvm-qgl.so': 'libllvm-qg6.so',
    'libadreno_utils.so': 'libadreno_util6.so',
    'libadreno_app_profiles.so': 'libadreno_app_profile6.so',
    'libq3dtools_adreno.so': 'libq3dtools_adren6.so',
    'libCB.so': 'libC6.so',
}
FILES = ['libEGL_adreno.so', 'libGLESv2_adreno.so', 'eglSubDriverAndroid.so', 'libgsl.so',
         'libllvm-glnext.so', 'libllvm-qgl.so', 'libadreno_utils.so']

for old, new in NAMES.items():
    assert len(old) == len(new), old

os.makedirs(dst, exist_ok=True)
for f in FILES:
    data = open(os.path.join(src, f), 'rb').read()
    for old, new in NAMES.items():
        # match whole names only: preceded by NUL, '/', or a quote-free boundary
        data = data.replace(b'\0' + old.encode(), b'\0' + new.encode())
        data = data.replace(b'/' + old.encode(), b'/' + new.encode())
        data = data.replace(b'YZ' + old.encode(), b'YZ' + new.encode())
    left = [o for o in NAMES if b'\0' + o.encode() + b'\0' in data]
    assert not left, (f, left)
    out = os.path.join(dst, NAMES[f])
    open(out, 'wb').write(data)
    # Absolute NEEDED: /vendor/lib64/camera is not on the linker search path.
    args = []
    for lib in elf_needed(out):
        if lib in NAMES.values():
            args += ['--replace-needed', lib, DIR + lib]
    if args:
        subprocess.run([PATCHELF] + args + [out], check=True)
    print(f, '->', NAMES[f])
