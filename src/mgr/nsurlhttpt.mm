/*****************************************************************************
 *
 *  nsurlhttpt.mm -	NSURLHTTPTransport: HTTP(S) through Apple's
 *			NSURLSession, so the TLS is the operating system's
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

#import <Foundation/Foundation.h>

#include <vector>
#include <cctype>
#include <cstdlib>
#include <cstring>

#include <swlog.h>
#include <filemgr.h>
#include <nsurlhttpt.h>

using std::vector;


// answers the server-trust and login challenges as CURLHTTPTransport did:
// the system judges the certificate; one it rejects is still taken when
// unverified peers are allowed (SSL_VERIFYPEER off in the curl transport)
@interface SWNSURLHTTPDelegate : NSObject <NSURLSessionDelegate>
@property (nonatomic, assign) BOOL allowUnverified;
@property (nonatomic, copy) NSString *user;
@property (nonatomic, copy) NSString *password;
@end

@implementation SWNSURLHTTPDelegate

- (void)URLSession:(NSURLSession *)session didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
	completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition, NSURLCredential *))completionHandler {

	NSString *method = challenge.protectionSpace.authenticationMethod;
	if ([method isEqualToString:NSURLAuthenticationMethodServerTrust]) {
		SecTrustRef trust = challenge.protectionSpace.serverTrust;
		if (trust && !SecTrustEvaluateWithError(trust, NULL) && self.allowUnverified) {
			completionHandler(NSURLSessionAuthChallengeUseCredential, [NSURLCredential credentialForTrust:trust]);
			return;
		}
		completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
		return;
	}
	if (([method isEqualToString:NSURLAuthenticationMethodHTTPBasic] || [method isEqualToString:NSURLAuthenticationMethodHTTPDigest])
			&& self.user.length && challenge.previousFailureCount == 0) {
		completionHandler(NSURLSessionAuthChallengeUseCredential,
			[NSURLCredential credentialWithUser:self.user password:(self.password ? self.password : @"") persistence:NSURLCredentialPersistenceNone]);
		return;
	}
	completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
}

@end


SWORD_NAMESPACE_START

namespace {

	// the size column of an Apache/nginx directory index row (as in curlhttpt.cpp)
	const char *findSizeStart(const char *buffer) {
		const char *listing = buffer;
		const char *pEnd;

		pEnd = strstr(listing, "<td");
		if(pEnd == NULL) {
			return NULL;
		}
		listing = pEnd+2;
		pEnd = strstr(listing, "<td");
		if(pEnd == NULL)
			return NULL;
		listing = pEnd+2;
		pEnd = strchr(listing, '>');
		if(pEnd == NULL)
			return NULL;

		return pEnd+1;
	}
}


NSURLHTTPTransport::NSURLHTTPTransport(const char *host, StatusReporter *sr) : RemoteTransport(host, sr) {
}


NSURLHTTPTransport::~NSURLHTTPTransport() {
}


// return values as CURLHTTPTransport: 0 success; -1 terminated by the user;
// -2 timed out; -3 access denied; -5 an HTTP error status; < -9 anything else
char NSURLHTTPTransport::getURL(const char *destPath, const char *sourceURL, SWBuf *destBuf) {
	signed char retVal = 0;

	@autoreleasepool {
		NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:sourceURL]];
		if (!url) {
			SWLog::getSystemLog()->logWarning("NSURLHTTPTransport: not a URL: %s\n", sourceURL);
			return -10;
		}

SWLOGD("***** NSURLHTTPTransport destPath: %s \n", destPath);
SWLOGD("***** NSURLHTTPTransport sourceURL: %s \n", sourceURL);

		NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
		if (timeoutMillis > 0) request.timeoutInterval = timeoutMillis / 1000.0;

		SWNSURLHTTPDelegate *delegate = [[SWNSURLHTTPDelegate alloc] init];
		delegate.allowUnverified = isUnverifiedPeerAllowed();
		if (u.length()) delegate.user = [NSString stringWithUTF8String:u.c_str()];
		if (p.length()) delegate.password = [NSString stringWithUTF8String:p.c_str()];

		NSURLSession *session = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration ephemeralSessionConfiguration]
			delegate:delegate delegateQueue:nil];

		dispatch_semaphore_t done = dispatch_semaphore_create(0);
		__block NSData *body = nil;
		__block NSInteger status = 0;
		__block NSInteger errorCode = 0;

		NSURLSessionDataTask *task = [session dataTaskWithRequest:request
			completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
				if (error) errorCode = error.code;
				else {
					body = [data copy];
					if ([response isKindOfClass:[NSHTTPURLResponse class]]) status = ((NSHTTPURLResponse *)response).statusCode;
				}
				dispatch_semaphore_signal(done);
			}];
		[task resume];

		// report progress and honor terminate() while the download runs
		bool cancelled = false;
		while (dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC))) {
			if (term && !cancelled) {
				[task cancel];
				cancelled = true;
			}
			if (statusReporter) {
				long long total = task.countOfBytesExpectedToReceive;
				long long now = task.countOfBytesReceived;
				if (total < 0) total = 0;
				if (now < 0) now = 0;
				if (now > total) total = now;
				statusReporter->update((unsigned long)total, (unsigned long)now);
			}
		}
		[session finishTasksAndInvalidate];

		if (errorCode) {
			if (term) retVal = -1;
			else if (errorCode == NSURLErrorTimedOut) retVal = -2;
			else if (errorCode == NSURLErrorUserAuthenticationRequired || errorCode == NSURLErrorNoPermissionsToReadFile) retVal = -3;
			else retVal = -10;
			SWLog::getSystemLog()->logWarning("NSURLHTTPTransport: %s failed, NSURLError %ld\n", sourceURL, (long)errorCode);
		}
		else if (status >= 400) {
			retVal = -5;
			SWLog::getSystemLog()->logWarning("NSURLHTTPTransport: %s returned HTTP %ld\n", sourceURL, (long)status);
		}
		else if (destBuf) {
			unsigned long s = destBuf->size();
			destBuf->size(s + body.length);
			if (body.length) memcpy(destBuf->getRawData() + s, body.bytes, body.length);
		}
		else {
			int fd = FileMgr::createPathAndFile(destPath);
			if (fd < 0) retVal = -10;
			else {
				if (body.length && FileMgr::write(fd, body.bytes, body.length) != (long)body.length) retVal = -10;
				FileMgr::closeFile(fd);
			}
		}
		if (!retVal && statusReporter) statusReporter->update((unsigned long)body.length, (unsigned long)body.length);

#if !__has_feature(objc_arc)
		[delegate release];
		[body release];
		dispatch_release(done);
#endif
	}

	return retVal;
}


// an HTTP directory index, parsed as CURLHTTPTransport does
vector<struct DirEntry> NSURLHTTPTransport::getDirList(const char *dirURL) {

	vector<struct DirEntry> dirList;

	SWBuf dirBuf;
	const char *pBuf;
	char *pBufRes;
	SWBuf possibleName;
	double fSize;
	int possibleNameLength = 0;

	if (!getURL("", dirURL, &dirBuf)) {
		pBuf = strstr(dirBuf, "<a href=\"");//Find the next link to a possible file name.
		while (pBuf != NULL) {
			pBuf += 9;//move to the start of the actual name.
			pBufRes = (char *)strchr(pBuf, '\"');//Find the end of the possible file name
			if (!pBufRes)
				break;
			possibleNameLength = (int)(pBufRes - pBuf);
			possibleName.setFormatted("%.*s", possibleNameLength, pBuf);
			if (isalnum(possibleName[0])) {
SWLOGD("NSURLHTTPTransport::getDirList: Found a file: %s", possibleName.c_str());
				pBuf = pBufRes;
				pBufRes = (char *)findSizeStart(pBuf);
				fSize = 0;
				if(pBufRes != NULL) {
					pBuf = pBufRes;
					fSize = strtod(pBuf, &pBufRes);
					if (pBufRes[0] == 'K')
						fSize *= 1024;
					else if (pBufRes[0] == 'M')
						fSize *= 1048576;
					pBuf = pBufRes;
				}
				struct DirEntry i;
				i.name = possibleName;
				i.size = (long unsigned int)fSize;
				i.isDirectory = possibleName.endsWith("/");
				dirList.push_back(i);
			} else {
				pBuf += possibleNameLength;
			}
			pBuf++;
			pBuf = strstr(pBuf, "<a href=\"");//Find the next link to a possible file name.
		}
	}
	else
	{
		SWLog::getSystemLog()->logWarning("NSURLHTTPTransport::getDirList: failed to get dir %s\n", dirURL);
	}
	return dirList;
}


SWORD_NAMESPACE_END
