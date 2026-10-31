#!/bin/sh
#******************************************************************************
#
# fileops_mount.sh - FileMgr pluggable FileOps test: a FileOps subclass
#	mounts the virtual prefix /sword-shared/ onto a real scratch directory
#	and FileMgr, SWConfig and SWMgr are driven only through virtual paths.
#	Proves every path-taking primitive (open, access, stat, list, mkdir,
#	remove) goes through the installed FileOps, which is what a platform
#	backend (e.g., Android scoped storage) relies on.
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

mkdir -p tmp
../fileopstest tmp/fileops_mount
