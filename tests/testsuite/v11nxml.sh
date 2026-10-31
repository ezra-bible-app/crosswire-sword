#!/bin/sh
#******************************************************************************
#
# registerVersificationSystemFromXML, osis2mod -v <definition.xml>, and the
# SWMgr v11n.xml module hook: the module is BUILT with the custom system it
# ships, so its data and its versification agree
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
Versification=TestCustom
Lang=en
!

# the definition ships in the module's data path...
cp v11nxml-fixture.xml tmp/v11nxml/modules/ztext/v11n.xml
# ...and -v <definition.xml> builds the module with that same system
../../utilities/osis2mod tmp/v11nxml/modules/ztext/ v11nxml-osis.xml -z -v v11nxml-fixture.xml > /dev/null 2>&1

cd tmp/v11nxml
../../../v11nxmltest ../../v11nxml-fixture.xml . CustomV11n
