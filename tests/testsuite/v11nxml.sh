#!/bin/sh
#******************************************************************************
#
# registerVersificationSystemFromXML and the SWMgr v11n.xml module hook
#
# Copyright 2026 CrossWire Bible Society (http://www.crosswire.org)
#	CrossWire Bible Society
#	P. O. Box 2528
#	Tempe, AZ  85280-2528
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation version 2.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#

rm -rf tmp/v11nxml/
mkdir -p tmp/v11nxml/mods.d tmp/v11nxml/modules/ztext

cat > tmp/v11nxml/mods.d/custom.conf <<!
[CustomV11n]
DataPath=./modules/ztext/
ModDrv=zText
Encoding=UTF-8
BlockType=BOOK
CompressType=ZIP
SourceType=OSIS
Versification=KJV
Lang=en
!

../../utilities/osis2mod tmp/v11nxml/modules/ztext/ osisReference.xml -z > /dev/null 2>&1
cp v11nxml-fixture.xml tmp/v11nxml/modules/ztext/v11n.xml

cd tmp/v11nxml
../../../v11nxmltest ../../v11nxml-fixture.xml . CustomV11n
