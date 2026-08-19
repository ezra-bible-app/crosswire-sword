#!/bin/sh
#******************************************************************************
#
# installmgr_sources.sh - InstallMgr remote source configuration tests:
#	syncing with the master repo list (refreshRemoteSourceConfiguration),
#	chaining of sources which share a caption (HTTPS PackagePreference
#	tried before FTP), and https-first / ftp-fallback for the master list.
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

. ./installmgr_fixture.sh installmgr_sources

echo "== basic config, then sync with master list (https first)"
im -init -sc -s -conf

echo
echo "== sync again: must be idempotent (no duplicate chained sources)"
im -sc -sc -conf

echo
echo "== sync with https down: falls back to ftp for the master list"
im --fail https -sc -conf

echo
echo "== sync with everything down: fails, config untouched"
im --fail https --fail ftp -sc -conf

echo
echo "== master list updates a chained source's definition and removes another"
cat > $HTTPS_CW/masterRepoList.conf <<!
[Repos]
20220529224400=HTTPSPackagePreference=CrossWire|crosswire.org|/ftpmirror/pub/sword/raw
20081216195754=FTPSource=CrossWire|ftp.crosswire.org|/pub/sword/rawmoved
20090224125400=FTPSource=CrossWire Beta|ftp.crosswire.org|/pub/sword/betaraw
20240314004500=REMOVE
!
im -sc -s -conf

echo
echo "== master list removes the head of a chain; the chained source remains"
cat > $HTTPS_CW/masterRepoList.conf <<!
[Repos]
20220529224400=REMOVE
20081216195754=FTPSource=CrossWire|ftp.crosswire.org|/pub/sword/raw
20090224125400=FTPSource=CrossWire Beta|ftp.crosswire.org|/pub/sword/betaraw
!
im -sc -s -conf

echo
echo "== a user-defined source with the same caption chains after the master list ones"
im_conf "HTTPSPackagePreference=CrossWire|crosswire.org|/ftpmirror/pub/sword/raw|||20220529224400" \
	"FTPSource=CrossWire|ftp.crosswire.org|/pub/sword/raw|||20081216195754" \
	"HTTPSSource=CrossWire|mirror.example.org|/sword/raw|||mymirror" \
	"HTTPSource=Elsewhere|www.elsewhere.org|/sword|||elsewhere"
im -s -sn
