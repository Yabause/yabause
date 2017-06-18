adb push libs/armeabi-v7a/libsh2d.so /data/local/tmp/
adb push libs/armeabi-v7a/libgnustl_shared.so /data/local/tmp/
adb push libs/armeabi-v7a/sh2d_unittest /data/local/tmp/
adb shell chmod 775 /data/local/tmp/sh2d_unittest

adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/sh2d_unittest"

  