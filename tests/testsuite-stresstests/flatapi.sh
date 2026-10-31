#!/bin/sh
#******************************************************************************
#
# The flat C API under churn, with valgrind watching: see ../flatapitest.cpp
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

rm -rf tmp/flatapi/
mkdir -p tmp/flatapi/mods.d
mkdir -p tmp/flatapi/modules/ztext tmp/flatapi/modules/rawtext tmp/flatapi/modules/rawld tmp/flatapi/modules/zld tmp/flatapi/modules/gbs

cat > tmp/flatapi/mods.d/ztext.conf <<!
[zText]
DataPath=./modules/ztext/
ModDrv=zText
Encoding=UTF-8
BlockType=BOOK
CompressType=ZIP
SourceType=OSIS
GlobalOptionFilter=OSISFootnotes
GlobalOptionFilter=OSISStrongs
GlobalOptionFilter=OSISLemma
GlobalOptionFilter=OSISMorph
Lang=en
Version=1.0
Description=flat API zText
!
cat > tmp/flatapi/mods.d/rawtext.conf <<!
[RawText]
DataPath=./modules/rawtext/
ModDrv=RawText
Encoding=UTF-8
SourceType=OSIS
GlobalOptionFilter=OSISFootnotes
GlobalOptionFilter=OSISStrongs
GlobalOptionFilter=OSISLemma
GlobalOptionFilter=OSISMorph
Lang=en
Version=1.0
Description=flat API RawText
!
cat > tmp/flatapi/mods.d/rawld.conf <<!
[RawLD]
DataPath=./modules/rawld/rawld
ModDrv=RawLD
Encoding=UTF-8
SourceType=ThML
Lang=en
Version=1.0
Description=flat API RawLD
!
cat > tmp/flatapi/mods.d/zld.conf <<!
[zLD]
DataPath=./modules/zld/zld
ModDrv=zLD
CompressType=ZIP
Encoding=UTF-8
SourceType=ThML
Lang=en
Version=1.0
Description=flat API zLD
!
cat > tmp/flatapi/mods.d/gbs.conf <<!
[GBS]
DataPath=./modules/gbs/gbs
ModDrv=RawGenBook
Encoding=UTF-8
SourceType=OSIS
Lang=en
Version=1.0
Description=flat API GBS
!

../../utilities/osis2mod tmp/flatapi/modules/ztext/ ../testsuite/osisReference.xml -z > /dev/null 2>&1
../../utilities/osis2mod tmp/flatapi/modules/rawtext/ ../testsuite/osisReference.xml > /dev/null 2>&1
../../utilities/imp2ld ../testsuite/ldr12n.imp -o tmp/flatapi/modules/rawld/rawld > /dev/null 2>&1
../../utilities/imp2ld ../testsuite/ldr12n.imp -z z -o tmp/flatapi/modules/zld/zld > /dev/null 2>&1
../../utilities/imp2gbs ../testsuite/gbsReference.imp -o tmp/flatapi/modules/gbs/gbs > /dev/null 2>&1

cd tmp/flatapi
# FLATAPI_COUNT: how often each thing is done (the answers don't depend on
# it; a few rounds already show a buffer freed twice or never)
valgrind -q --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=9 ../../../flatapitest . ${FLATAPI_COUNT:-5} 2> valgrind.err
echo "flatapitest exit code: $?"
# the first line of each valgrind report, without the stacks
sed 's/^==[0-9]*== //' valgrind.err | grep -vE "^(   at |   by | Address | Block |$)"
