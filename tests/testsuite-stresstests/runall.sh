#!/bin/sh
#******************************************************************************
#
# Runs the whole stress suite: every test here runs a program under valgrind
# (--leak-check=full --error-exitcode), so valgrind must be installed.
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

if ! command -v valgrind > /dev/null 2>&1; then
	echo "valgrind is not installed; the stress tests need it"
	exit 1
fi

TESTSUITE=`for i in *.good; do basename $i .good; done`

for i in $TESTSUITE; do
	echo -n "$i: "
	./runtest.sh $i -q
	if [ $? -ne 0 ]; then
		echo FAILED
		echo ""
		echo "to see the difference: ./runtest.sh $i"
		exit 1
	else
		echo PASSED
	fi
done
