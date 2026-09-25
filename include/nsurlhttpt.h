/******************************************************************************
 *
 * nsurlhttpt.h -	Apple NSURLSession HTTP(S) implementation of
 *			RemoteTransport
 *
 * $Id$
 *
 * Copyright 2026 CrossWire Bible Society (http://www.crosswire.org)
 *	CrossWire Bible Society
 *	P. O. Box 2528
 *	Tempe, AZ  85280-2528
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#ifndef NSURLHTTPT_H
#define NSURLHTTPT_H

#include <defs.h>
#include <remotetrans.h>

SWORD_NAMESPACE_START


// HTTP and HTTPS through the operating system (iOS, macOS), so an app
// carries no TLS of its own.  Built where NSURLAVAILABLE is defined.
class SWDLLEXPORT NSURLHTTPTransport : public RemoteTransport {

public:
	NSURLHTTPTransport(const char *host, StatusReporter *statusReporter = 0);
	~NSURLHTTPTransport();

	virtual std::vector<struct DirEntry> getDirList(const char *dirURL);
	virtual char getURL(const char *destPath, const char *sourceURL, SWBuf *destBuf = 0);
};


SWORD_NAMESPACE_END

#endif
