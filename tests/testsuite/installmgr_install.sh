#!/bin/sh
#******************************************************************************
#
# installmgr_install.sh - InstallMgr::installModule tests: a single archive
#	package is preferred (packages/<Module>.zip), then the module's data
#	directory is copied; each source in the chain is tried for the package
#	(HTTPS PackagePreference first, then FTP) but a PackagePreference is
#	never asked to walk a directory (http directory listings are not
#	reliable across servers); the module .conf is installed with the
#	install source recorded; and removeModule cleans up.
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

. ./installmgr_fixture.sh installmgr_install

im -init -sc > /dev/null
im -r CrossWire -r "CrossWire Beta" > /dev/null

echo "== install from package over https"
im -ri CrossWire GBSReference -l
im_dest

echo
echo "== install module without a package: data directory over ftp (PackagePreference is never walked)"
im -ri CrossWire GBSSecond -l
im_dest

echo
echo "== uninstall"
im -u GBSSecond -u GBSReference -l
im_dest

echo
echo "== uninstall a module which lives in an augmented (e.g. shared) library: its conf and data go from there"
# build a second library by installing into it, then use it only as an augment of dest
im -ri CrossWire GBSSecond > /dev/null
mv $IMPRIV/dest $IMPRIV/other
mkdir -p $IMPRIV/dest/mods.d
im --augment $IMPRIV/other -l
im --augment $IMPRIV/other -u GBSSecond -l
echo "other files:"
(cd $IMPRIV/other && find . -type f | grep -v modules-conf.cache | sort | sed 's/^/\t/')
im_dest
rm -rf $IMPRIV/other

echo
echo "== https down: package over ftp"
im --fail https -ri CrossWire GBSReference -l
im -u GBSReference > /dev/null

echo
echo "== https down, no ftp package: data directory over ftp"
rm -rf $FTP_CW/raw/packages
im --fail https -ri CrossWire GBSReference -l
im -u GBSReference > /dev/null

echo
echo "== https package missing: ftp package missing too: ftp data directory"
rm -rf $HTTPS_CW/raw/packages
im -ri CrossWire GBSReference -l
im -u GBSReference > /dev/null

echo
echo "== a plain HTTPSSource chained after an FTPSource is walked when ftp fails mid-way through the data directory"
im_conf "FTPSource=CrossWire|ftp.crosswire.org|/pub/sword/raw|||20081216195754" \
	"HTTPSSource=CrossWire|crosswire.org|/ftpmirror/pub/sword/raw|||cwhttps"
im -r CrossWire > /dev/null
im --fail ftp://ftp.crosswire.org/pub/sword/raw/modules/gbsreference/gbsreference.dat=-9 -ri CrossWire GBSReference -l
im -u GBSReference > /dev/null
im -init -sc > /dev/null
im -r CrossWire -r "CrossWire Beta" > /dev/null

echo
echo "== everything down: install fails and nothing is left behind"
im --fail https --fail ftp -ri CrossWire GBSReference -l
im_dest

echo
echo "== user cancels: nothing else is tried"
im --fail https=-1 -ri CrossWire GBSReference -l

echo
echo "== install from ftp-only source"
im -ri "CrossWire Beta" GBSSecond -l
im_dest

echo
echo "== install a module the source does not have"
im -ri CrossWire NoSuchModule

echo
echo "== upgrade: a shorter .conf upstream must replace the installed one exactly"
im -ri CrossWire GBSReference > /dev/null
for d in $HTTPS_CW/raw $FTP_CW/raw; do
	cat > $d/mods.d/gbsreference.conf <<!
[GBSReference]
DataPath=./modules/gbsreference/gbsreference
ModDrv=RawGenBook
Version=1.1
Description=Short
!
	im_retar $d
done
im -r CrossWire > /dev/null
im -ri CrossWire GBSReference -l
echo "installed conf:"
cat $IMPRIV/dest/mods.d/gbsreference.conf | grep -v InstallSource
