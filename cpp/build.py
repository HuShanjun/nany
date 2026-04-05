#!/usr/bin/env python3

import os
import sys
print(f'操作系统: {sys.platform}')
# 如果是linux
if sys.platform == 'linux':
    # 执行cmake --preset linux-vcpkg-static,执行成功后执行cmake --build build/linux
    if os.system('cmake --preset linux-vcpkg-static') != 0:
        print('执行cmake --preset linux-vcpkg-static失败')
        exit(1)
    if os.system('cmake --build build/linux -j 16') != 0:
        print('执行cmake --build build/linux失败')
        exit(1)
# 如果是windows
elif sys.platform == 'win32':
    # 执行windows的build.bat
    if os.system('cmake --preset vcpkg-static') != 0:
        print('执行cmake --preset vcpkg-static失败')
        exit(1)
    if os.system('cmake --build build/msvc -j 16') != 0:
        print('执行cmake --build build/msvc失败')
        exit(1)
else:
    print(f'不支持的操作系统: {os.name}')