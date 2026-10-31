#!/bin/sh
#******************************************************************************
#
# SWModule::clone(): one module per driver the suite can build, each cloned
# and checked by ../clonetest
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

rm -rf tmp/clone/
mkdir -p tmp/clone/mods.d
mkdir -p tmp/clone/modules/ztext tmp/clone/modules/rawtext tmp/clone/modules/rawld tmp/clone/modules/zld tmp/clone/modules/gbs

cat > tmp/clone/mods.d/ztext.conf <<!
[zText]
DataPath=./modules/ztext/
ModDrv=zText
Encoding=UTF-8
BlockType=BOOK
CompressType=ZIP
SourceType=OSIS
Lang=en
!
cat > tmp/clone/mods.d/rawtext.conf <<!
[RawText]
DataPath=./modules/rawtext/
ModDrv=RawText
Encoding=UTF-8
SourceType=OSIS
Lang=en
!
cat > tmp/clone/mods.d/rawld.conf <<!
[RawLD]
DataPath=./modules/rawld/rawld
ModDrv=RawLD
Encoding=UTF-8
SourceType=ThML
Lang=en
!
cat > tmp/clone/mods.d/zld.conf <<!
[zLD]
DataPath=./modules/zld/zld
ModDrv=zLD
CompressType=ZIP
Encoding=UTF-8
SourceType=ThML
Lang=en
!
cat > tmp/clone/mods.d/gbs.conf <<!
[GBS]
DataPath=./modules/gbs/gbs
ModDrv=RawGenBook
Encoding=UTF-8
SourceType=OSIS
Lang=en
!

../../utilities/osis2mod tmp/clone/modules/ztext/ osisReference.xml -z > /dev/null 2>&1
../../utilities/osis2mod tmp/clone/modules/rawtext/ osisReference.xml > /dev/null 2>&1
../../utilities/imp2ld ldr12n.imp -o tmp/clone/modules/rawld/rawld > /dev/null 2>&1
../../utilities/imp2ld ldr12n.imp -z z -o tmp/clone/modules/zld/zld > /dev/null 2>&1
../../utilities/imp2gbs gbsReference.imp -o tmp/clone/modules/gbs/gbs > /dev/null 2>&1

cd tmp/clone
../../../clonetest . "Acts 2:19"
