#!/bin/sh
#-------------------------------------------------------------------------------
QT_PATH_FRAMEWORKS=/usr/local/Trolltech/Qt-4.3.3/lib
QT_PLUGINGS=/usr/local/Trolltech/Qt-4.3.3/plugins
YABAUSE_APP=../Yabause.app
YABAUSE_FRAMEWORKS=$YABAUSE_APP/Contents/Frameworks
YABAUSE=$YABAUSE_APP/Contents/MacOS/Yabause
SDL_FRAMEWORK=/Developer/SDKs/MacOSX10.4u.sdk/Library/Frameworks/SDL.framework

echo "Copying Qt 4 frameworks..."
mkdir -p $YABAUSE_FRAMEWORKS
cp $QT_PATH_FRAMEWORKS/QtCore.framework/Versions/4/QtCore $YABAUSE_FRAMEWORKS
cp $QT_PATH_FRAMEWORKS/QtGui.framework/Versions/4/QtGui $YABAUSE_FRAMEWORKS
cp $QT_PATH_FRAMEWORKS/QtOpenGL.framework/Versions/4/QtOpenGL $YABAUSE_FRAMEWORKS

echo "Copying SDL framework..."
rm -R $YABAUSE_FRAMEWORKS/SDL.framework
cp -R $SDL_FRAMEWORK $YABAUSE_FRAMEWORKS

echo "Relinking frameworks..."
install_name_tool -id @executable_path/../Frameworks/QtCore $YABAUSE_FRAMEWORKS/QtCore
install_name_tool -id @executable_path/../Frameworks/QtGui $YABAUSE_FRAMEWORKS/QtGui
install_name_tool -id @executable_path/../Frameworks/QtOpenGL $YABAUSE_FRAMEWORKS/QtOpenGL

install_name_tool -change $QT_PATH_FRAMEWORKS/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore $YABAUSE_FRAMEWORKS/QtGui
install_name_tool -change $QT_PATH_FRAMEWORKS/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore $YABAUSE_FRAMEWORKS/QtOpenGL
install_name_tool -change $QT_PATH_FRAMEWORKS/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui $YABAUSE_FRAMEWORKS/QtOpenGL

install_name_tool -change $QT_PATH_FRAMEWORKS/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore $YABAUSE
install_name_tool -change $QT_PATH_FRAMEWORKS/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui $YABAUSE
install_name_tool -change $QT_PATH_FRAMEWORKS/QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL $YABAUSE

echo "Creating temporary dmg..."
hdiutil create -megabytes 30 tmp.dmg -layout NONE -fs HFS+ -volname Yabause -ov
tmp=`hdid tmp.dmg`
disk=`echo $tmp | cut -f 1 -d\ `

echo "Copying Yabause..."
cp -R ../Yabause.app /Volumes/Yabause/

echo "Copying standard files..."
cp ../../../ChangeLog /Volumes/Yabause/
cp ../../../README /Volumes/Yabause/
cp ../../../README.QT /Volumes/Yabause/
cp ../../../AUTHORS /Volumes/Yabause/
cp ../../../COPYING /Volumes/Yabause/

echo "Creating release dmg..."
hdiutil eject $disk
hdiutil convert -format UDZO tmp.dmg -o Yabause.dmg
rm tmp.dmg
