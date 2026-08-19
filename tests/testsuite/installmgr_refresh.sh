#!/bin/sh
#******************************************************************************
#
# installmgr_refresh.sh - InstallMgr::refreshRemoteSource tests: the head of
#	a source chain (HTTPS PackagePreference) is tried first, falling back
#	to the chained FTP source; mods.d.tar.gz is preferred, falling back to
#	copying the mods.d directory (a PackagePreference source is only ever
#	asked for the archive, never walked); and whichever way succeeds, the
#	local shadow of every source in the chain ends up with the same module
#	list.
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

. ./installmgr_fixture.sh installmgr_refresh

im -init -sc > /dev/null

echo "== refresh: https archive"
im -r CrossWire -rl CrossWire
im_shadow 20220529224400
im_shadow 20081216195754

echo
echo "== refresh: https down, ftp archive"
im --fail https -r CrossWire -rl CrossWire
im_shadow 20220529224400
im_shadow 20081216195754

echo
echo "== refresh: https down, ftp has no archive; ftp directory copy"
rm $FTP_CW/raw/mods.d.tar.gz
im --fail https -r CrossWire -rl CrossWire
im_shadow 20220529224400
im_shadow 20081216195754

echo
echo "== refresh: https serves a bogus archive (e.g., captive portal); the PackagePreference is never walked, ftp is used"
cp $HTTPS_CW/raw/mods.d.tar.gz $IMROOT/good.tar.gz
echo "<html>not a tar.gz</html>" > $HTTPS_CW/raw/mods.d.tar.gz
im -r CrossWire -rl CrossWire 2> /dev/null
cp $IMROOT/good.tar.gz $HTTPS_CW/raw/mods.d.tar.gz

echo
echo "== refresh: everything down; fails, and no stale module list survives"
im --fail https --fail ftp -r CrossWire -rl CrossWire
im_shadow 20220529224400
im_shadow 20081216195754

echo
echo "== refresh: user cancels during https (nothing else is tried)"
im --fail https=-1 -r CrossWire

echo
echo "== refresh: module removed upstream is gone after refresh (no stale shadow)"
im -r CrossWire -rl CrossWire > /dev/null
for d in $HTTPS_CW/raw $FTP_CW/raw; do rm -rf $d/mods.d/gbssecond.conf $d/modules/gbssecond; im_retar $d; done
im -r CrossWire -rl CrossWire
im_shadow 20220529224400
im_shadow 20081216195754

echo
echo "== refresh within one process flushes the cached module list of the head source"
for d in $HTTPS_CW/raw $FTP_CW/raw; do cp $IMREPO/mods.d/gbssecond.conf $d/mods.d/; im_retar $d; done
im -rl CrossWire -r CrossWire -rl CrossWire

echo
echo "== ftp-only source: archive, then directory copy when there is no archive"
im -r "CrossWire Beta" -rl "CrossWire Beta"
rm $FTP_CW/betaraw/mods.d.tar.gz
im -r "CrossWire Beta" -rl "CrossWire Beta"

echo
echo "== source with no reachable host"
im -r Elsewhere -rl Elsewhere
