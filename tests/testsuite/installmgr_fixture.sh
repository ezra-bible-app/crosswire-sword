#!/bin/sh
#******************************************************************************
#
# installmgr_fixture.sh - shared fixture setup for the installmgr_* tests.
#
#	This is not a test itself (it has no .good file); it is sourced by the
#	installmgr_*.sh tests.  It builds a small "remote repository" tree
#	which ../installmgrtest serves through a mock transport in place of
#	the real ftp/http transports.  Layout (see tests/installmgrtest.cpp):
#
#	tmp/installmgr/fixture/<scheme>/<host>/<path>
#
#	The canonical repo lives in tmp/installmgr/repo and is copied to the
#	crosswire.org https mirror and the ftp.crosswire.org ftp host, so
#	both transports offer identical content unless a test alters one.
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

# arg 1: test name; everything is built under tmp/<testname>/
IMTEST=$1
IMROOT=tmp/$IMTEST
IMFIX=$IMROOT/fixture
IMREPO=$IMROOT/repo
IMPRIV=$IMROOT/private
IMBIN=../installmgrtest

HTTPS_CW=$IMFIX/https/crosswire.org/ftpmirror/pub/sword
FTP_CW=$IMFIX/ftp/ftp.crosswire.org/pub/sword

rm -rf $IMROOT
mkdir -p $IMREPO/mods.d $IMREPO/modules $IMPRIV

# ---------------------------------------------------------------------
# two small modules in the canonical repo
# ---------------------------------------------------------------------
cat > $IMREPO/mods.d/gbsreference.conf <<!
[GBSReference]
DataPath=./modules/gbsreference/gbsreference
ModDrv=RawGenBook
Encoding=UTF-8
SourceType=OSIS
Lang=en
Version=1.0
Description=GBS Reference Test Module
!

cat > $IMREPO/mods.d/gbssecond.conf <<!
[GBSSecond]
DataPath=./modules/gbssecond/gbssecond
ModDrv=RawGenBook
Encoding=UTF-8
SourceType=OSIS
Lang=en
Version=2.1
Description=Second Test Module
!

mkdir -p $IMREPO/modules/gbsreference $IMREPO/modules/gbssecond
../../utilities/imp2gbs gbsReference.imp -o $IMREPO/modules/gbsreference/gbsreference > /dev/null 2>&1
../../utilities/imp2gbs gbsReference.imp -o $IMREPO/modules/gbssecond/gbssecond > /dev/null 2>&1

# archive of module confs, as CrossWire offers on its repos
(cd $IMREPO && tar czf mods.d.tar.gz mods.d)

# single archive package of a module, as CrossWire offers on its repos
# (prebuilt: mods.d/gbsreference.conf + modules/gbsreference/*, made from
# the imp2gbs output above; only GBSReference has a package, GBSSecond
# does not, so both install paths get exercised)
mkdir -p $IMREPO/packages
cp installmgr_GBSReference.zip $IMREPO/packages/GBSReference.zip

# ---------------------------------------------------------------------
# master repo list, as served from crosswire.org
# ---------------------------------------------------------------------
mkdir -p $HTTPS_CW $FTP_CW
cat > $HTTPS_CW/masterRepoList.conf <<!
[Repos]
20220529224400=HTTPSPackagePreference=CrossWire|crosswire.org|/ftpmirror/pub/sword/raw
20081216195754=FTPSource=CrossWire|ftp.crosswire.org|/pub/sword/raw
20090224125400=FTPSource=CrossWire Beta|ftp.crosswire.org|/pub/sword/betaraw
20240314004500=FTPSource=Elsewhere|ftp.elsewhere.org|/pub/sword
!
cp $HTTPS_CW/masterRepoList.conf $FTP_CW/masterRepoList.conf

# ---------------------------------------------------------------------
# publish the canonical repo on both hosts
# ---------------------------------------------------------------------
cp -r $IMREPO $HTTPS_CW/raw
cp -r $IMREPO $FTP_CW/raw
cp -r $IMREPO $FTP_CW/betaraw

# ---------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------

# write an InstallMgr.conf with the given [Sources] lines
# usage: im_conf "FTPSource=CrossWire|ftp.crosswire.org|/pub/sword/raw|||20081216195754" ...
im_conf() {
	mkdir -p $IMPRIV
	echo "[General]" > $IMPRIV/InstallMgr.conf
	echo "PassiveFTP=true" >> $IMPRIV/InstallMgr.conf
	echo "TimeoutMillis=10000" >> $IMPRIV/InstallMgr.conf
	echo "UnverifiedPeerAllowed=true" >> $IMPRIV/InstallMgr.conf
	echo "[Sources]" >> $IMPRIV/InstallMgr.conf
	for line in "$@"; do
		echo "$line" >> $IMPRIV/InstallMgr.conf
	done
}

# run the test driver against our fixture and private path
im() {
	$IMBIN $IMFIX $IMPRIV "$@"
}

# list the confs currently in a local shadow of a source
# usage: im_shadow <uid>
im_shadow() {
	echo "shadow [$1] mods.d:"
	if [ -d $IMPRIV/$1/mods.d ]; then
		(cd $IMPRIV/$1/mods.d && ls | sed 's/^/\t/')
	else
		echo "	(none)"
	fi
}

# list the files installed in the destination library
im_dest() {
	echo "dest files:"
	(cd $IMPRIV/dest && find . -type f | grep -v modules-conf.cache | sort | sed 's/^/\t/')
}

# rebuild the mods.d.tar.gz of a published repo dir after altering it
# usage: im_retar <repo dir>
im_retar() {
	(cd $1 && rm -f mods.d.tar.gz && tar czf mods.d.tar.gz mods.d)
}
