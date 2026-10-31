#!/bin/sh
#******************************************************************************
#
# registerVersificationSystemFromXML with <v11n base="..."> deltas and named
# collections: books appended, placed, patched; inherited mappings; the
# resolved system dumped back out as a full definition
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

../v11ndeltatest v11ndelta-fixture.xml v11ndelta-badbase.xml 2>/dev/null

# the resolved delta, dumped as a full definition: named collections
# survive, the appendix holds the base's own tail and the additions
echo "--- v11ndump"
../../utilities/v11ndump -r v11ndelta-fixture.xml TestDelta 2>/dev/null | grep "collection\|<book" | grep "collection\|Barn\|Herm\|EpLao\|4Macc\|3Macc\|2Macc\|\"Ps\"\|\"Rev\""
