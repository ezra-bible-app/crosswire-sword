/******************************************************************************
 *
 *  sharedstoragefileops.hpp -	FileOps which routes paths under
 *				SWMgr.SHARED_STORAGE_PATH to a Java
 *				SWMgr.SharedStorage object and everything
 *				else to the plain OS FileOps.
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

#ifndef SHAREDSTORAGEFILEOPS_HPP
#define SHAREDSTORAGEFILEOPS_HPP

#include <jni.h>
#include <filemgr.h>
#include <swbuf.h>

/** the prefix under which the native library sees a SharedStorage;
 * must match SWMgr.SHARED_STORAGE_PATH */
#define SHARED_STORAGE_PATH "/sword-shared"

class SharedStorageFileOps : public sword::FileOps {
	JavaVM *vm;
	jobject storage;		// global ref to the SWMgr.SharedStorage
	jclass fileInfoClass;		// global ref to SWMgr$FileInfo
	jfieldID fName, fSize, fModTime, fIsDirectory;
	jmethodID mOpen, mStat, mList, mMkdir, mRemove;

	JNIEnv *getEnv(bool &attached);
	// fills rel with the store-relative path and returns true if path is under the mount
	bool rel(const char *path, sword::SWBuf &rel);
	void toEntry(JNIEnv *env, jobject fileInfo, sword::DirEntry *entry);

public:
	/** @param env a valid env for the calling thread
	 * @param storage a SWMgr.SharedStorage (local ref is fine; a global ref is taken)
	 */
	SharedStorageFileOps(JNIEnv *env, jobject storage);
	virtual ~SharedStorageFileOps();

	virtual int open(const char *path, int mode, int perms);
	virtual bool hasAccess(const char *path, int mode);
	virtual int stat(const char *path, sword::DirEntry *entry);
	virtual std::vector<struct sword::DirEntry> listDir(const char *dirPath, bool includeSize, bool includeIsDirectory);
	virtual int mkdir(const char *path);
	virtual int remove(const char *path);
};

#endif
