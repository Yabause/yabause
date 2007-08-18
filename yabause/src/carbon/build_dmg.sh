hdiutil create -megabytes 20 tmp.dmg -layout NONE -fs HFS+ -volname Yabause -ov
tmp=`hdid tmp.dmg`
disk=`echo $tmp | cut -f 1 -d\ `
cp -R Yabause.app /Volumes/Yabause/
cp ../../ChangeLog /Volumes/Yabause/
cp ../../README /Volumes/Yabause/
cp ../../README.MAC /Volumes/Yabause/
cp ../../AUTHORS /Volumes/Yabause/
cp ../../COPYING /Volumes/Yabause/
hdiutil eject $disk
hdiutil convert -format UDZO tmp.dmg -o Yabause.dmg
rm tmp.dmg
