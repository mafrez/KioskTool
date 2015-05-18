#! /usr/bin/env bash
#FIXME: handle translations for the .kiosk files in data/
#./extractxml kiosk_data.xml > kiosk_data.cpp
$EXTRACTRC `find . -name \*.ui -o -name \*.rc` >> rc.cpp
$XGETTEXT rc.cpp src/*.cpp src/*.h -o $podir/kiosktool.pot
rm rc.cpp
#rm kiosk_data.cpp
