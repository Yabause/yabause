adb push libs/arm64-v8a/libsh2d.so /data/local/tmp/
adb push libs/arm64-v8a/libgnustl_shared.so /data/local/tmp/
adb push libs/arm64-v8a/sh2d_unittest /data/local/tmp/
adb push "C:\ext\android-ndk-r13b\prebuilt\android-arm64\gdbserver" /data/local/tmp/
adb shell chmod 775 /data/local/tmp/sh2d_unittest
adb shell chmod 775 /data/local/tmp/gdbserver/gdbserver

REM adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/sh2d_unittest"

adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/gdbserver/gdbserver :5039 /data/local/tmp/sh2d_unittest"
REM C:\ext\android-ndk-r13b\prebuilt\windows-x86_64\bin\gdb c:/ext/GitHub/yabasanshiro64/yabause/src/sh2_dynarec_devmiyax/libs/arm64-v8a/sh2d_unittest
REM target remote 192.168.150.30:5039
