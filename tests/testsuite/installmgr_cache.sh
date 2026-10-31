#!/bin/sh
#******************************************************************************
#
# installmgr_cache.sh - mods.d/modules-conf.cache tests: SWMgr caches the
#	merged module configuration of a library in mods.d/modules-conf.cache
#	as a speedup.  The cache is only a speedup and must be thrown away
#	whenever anything in mods.d changes, whether by InstallMgr or by
#	anyone else (a .conf added, removed, or edited by hand or by another
#	program), so a stale module list is never shown.
#
#	(the sleeps let file timestamps move on; some filesystems only have
#	1-2 second resolution)
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

. ./installmgr_fixture.sh installmgr_cache

DESTMODSD=$IMPRIV/dest/mods.d

im -init -sc > /dev/null
im -r CrossWire > /dev/null

echo "== install a module; listing the library creates the cache"
im -ri CrossWire GBSReference > /dev/null
im -l
[ -f $DESTMODSD/modules-conf.cache ] && echo "cache exists" || echo "cache missing"

echo
echo "== installing another module through InstallMgr updates the list"
im -ri CrossWire GBSSecond > /dev/null
im -l

echo
echo "== uninstalling through InstallMgr updates the list"
im -u GBSSecond > /dev/null
im -l

sleep 2
echo
echo "== a .conf and module dropped in by someone else show up"
cp $IMREPO/mods.d/gbssecond.conf $DESTMODSD/
cp -r $IMREPO/modules/gbssecond $IMPRIV/dest/modules/
im -l

sleep 2
echo
echo "== a .conf edited in place by someone else (e.g., a cipher key added) is seen"
sed 's/^Version=1.0$/Version=1.7/' $DESTMODSD/gbsreference.conf > $DESTMODSD/gbsreference.conf.new
mv $DESTMODSD/gbsreference.conf.new $DESTMODSD/gbsreference.conf
im -l

sleep 2
echo
echo "== a .conf removed by someone else is gone"
rm $DESTMODSD/gbssecond.conf
im -l

sleep 2
echo
echo "== a .conf copied in with an old date, the directory's date held back, is still seen (the cache's file list)"
cp -p $IMREPO/mods.d/gbssecond.conf $DESTMODSD/
touch -r $DESTMODSD/modules-conf.cache $DESTMODSD
im -l
rm $DESTMODSD/gbssecond.conf
im -l > /dev/null

echo
echo "== a cache from before the file list was kept is rebuilt once"
grep -v '^Conf=' $DESTMODSD/modules-conf.cache | grep -v '^\[\.cache\]' > $DESTMODSD/modules-conf.cache.old
mv $DESTMODSD/modules-conf.cache.old $DESTMODSD/modules-conf.cache
touch -r $DESTMODSD/gbsreference.conf $DESTMODSD/modules-conf.cache $DESTMODSD
im -l
grep -c '^Conf=' $DESTMODSD/modules-conf.cache

echo
echo "== removing the cache by hand is always safe"
rm $DESTMODSD/modules-conf.cache
im -l
[ -f $DESTMODSD/modules-conf.cache ] && echo "cache exists" || echo "cache missing"

echo
echo "== a remote source shadow: modules removed upstream do not linger in a stale cache"
im -rl CrossWire > /dev/null
for d in $HTTPS_CW/raw $FTP_CW/raw; do rm -f $d/mods.d/gbssecond.conf; im_retar $d; done
im -r CrossWire -rl CrossWire
