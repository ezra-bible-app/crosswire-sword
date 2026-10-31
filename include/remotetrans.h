/******************************************************************************
 *
 * remotetrans.h -	class RemoteTransport: the base for all communication
 * 			transport drivers
 *
 * $Id$
 *
 * Copyright 2004-2013 CrossWire Bible Society (http://www.crosswire.org)
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

#ifndef REMOTETRANS_H
#define REMOTETRANS_H

#include <vector>
#include <defs.h>
#include <swbuf.h>


SWORD_NAMESPACE_START

/** Class for reporting status
*/
class SWDLLEXPORT StatusReporter {
public:
	virtual ~StatusReporter() {};
	/** Messages before stages of a batch download */
	virtual void preStatus(long totalBytes, long completedBytes, const char *message);

	/**
	 * @deprecated Use override update instead, receive sttaus updates
	 */
	SWDEPRECATED virtual void statusUpdate(double dtTotal, double dlNow);
	/** frequently called throughout a download, to report status
	*	override to receive status information
	*/
	virtual void update(unsigned long totalBytes, unsigned long completedBytes);
};


/**
* A base class to be used for reimplementation of network services.
*/
class SWDLLEXPORT RemoteTransport {

protected:
	StatusReporter *statusReporter;
	bool passive;
	long timeoutMillis;
	bool term;
	bool unverifiedPeerAllowed;
	SWBuf caInfo;
	SWBuf host;
	SWBuf u;
	SWBuf p;

public:
	RemoteTransport(const char *host, StatusReporter *statusReporter = 0);
	virtual ~RemoteTransport();

	/***********
	 * override this method in your real impl
	 *
	 * if destBuf then write to buffer instead of file
	 * @return 0 success; -1 user requested termination; < -1 some kind of problem.
	 *   Clients typically only care about these three cases.  Which problem is
	 *   transport specific, e.g., the CURL transports report: -2 connection error
	 *   (e.g., connect timeout); -3 access denied; -4 resource not found; -5 http error;
	 *   -10-CURLcode for any other libcurl error
	 */
	virtual char getURL(const char *destPath, const char *sourceURL, SWBuf *destBuf = 0);

	/***********
	 * override this method in your real impl
	 *
	 * if sourceBuf then read from buffer instead of file
	 * @return 0 success; -1 user requested termination; < -1 some kind of problem (see getURL)
	 */
	virtual char putURL(const char *destURL, const char *sourcePath, SWBuf *sourceBuf = 0);


	/** network copy recursively a remote directory
	 * @return 0 success; -1 user requested termination; < -1 some kind of problem (see getURL)
	 */
	int copyDirectory(const char *urlPrefix, const char *dir, const char *dest, const char *suffix);

	virtual std::vector<struct DirEntry> getDirList(const char *dirURL);
	void setPassive(bool passive) { this->passive = passive; }
	void setTimeoutMillis(long timeoutMillis) { this->timeoutMillis = timeoutMillis; }
	long getTimeoutMillis() { return timeoutMillis; }
	bool isPassive() { return passive; }
	void setUser(const char *user) { u = user; }
	void setPasswd(const char *passwd) { p = passwd; }
	void setUnverifiedPeerAllowed(bool val) { this->unverifiedPeerAllowed = val; }
	bool isUnverifiedPeerAllowed() { return unverifiedPeerAllowed; }

	/** a PEM bundle of CA certificates to verify peers against (curl's
	 * CURLOPT_CAINFO); empty leaves the transport's built-in default */
	void setCAInfo(const char *caInfo) { this->caInfo = caInfo ? caInfo : ""; }
	const char *getCAInfo() const { return caInfo.c_str(); }
	void terminate() { term = true; }
};


SWORD_NAMESPACE_END

#endif
