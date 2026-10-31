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

/** the prefixes under which the native library sees SharedStorages;
 * must match SWMgr.SHARED_STORAGE_PATH / SWMgr.DATA_STORAGE_PATH */
#define SHARED_STORAGE_PATH "/sword-shared"
#define DATA_STORAGE_PATH   "/sword-data"

#include <vector>
#include <map>
#include <mutex>

/** Routes each mounted prefix to its own SWMgr.SharedStorage; any path
 * not under a mount goes to the plain OS FileOps.  Installed once; the
 * mounts change as the user picks folders. */
class SharedStorageFileOps : public sword::FileOps {
	struct Mount {
		sword::SWBuf prefix;
		jobject storage;	// global ref to the SWMgr.SharedStorage
	};
	JavaVM *vm;
	std::vector<Mount> mounts;
	mutable std::mutex mountsMutex;
	std::map<int, jobject> issued;	// fd -> storage which issued it (for close)
	// SWMgr$FileInfo's fields, taken from the first FileInfo object seen
	// (FindClass from a pool thread may resolve a different loader's class)
	jfieldID fName, fSize, fModTime, fIsDirectory;
	void resolveFileInfo(JNIEnv *env, jobject fileInfo);
	jmethodID mOpen, mStat, mList, mMkdir, mRemove, mClose;

	JNIEnv *getEnv(bool &attached);
	// fills rel with the store-relative path and returns a new global ref to
	// the mount's storage if path is under one (release with Held), else 0
	jobject rel(const char *path, sword::SWBuf &rel);
	struct Held {
		SharedStorageFileOps *ops; jobject storage;
		Held(SharedStorageFileOps *o, jobject s) : ops(o), storage(s) {}
		~Held();
	};
	void toEntry(JNIEnv *env, jobject fileInfo, sword::DirEntry *entry);

public:
	/** @param env a valid env for the calling thread */
	SharedStorageFileOps(JNIEnv *env);
	virtual ~SharedStorageFileOps();

	/** route paths under prefix (e.g. SHARED_STORAGE_PATH) to storage, a
	 * SWMgr.SharedStorage (local ref is fine; a global ref is taken), or to
	 * nothing with storage == 0; replaces any mount at that prefix */
	void setMount(JNIEnv *env, const char *prefix, jobject storage);
	bool hasMount(const char *prefix) const;

	virtual int open(const char *path, int mode, int perms);
	virtual bool hasAccess(const char *path, int mode);
	virtual int stat(const char *path, sword::DirEntry *entry);
	virtual std::vector<struct sword::DirEntry> listDir(const char *dirPath, bool includeSize, bool includeIsDirectory);
	virtual int mkdir(const char *path);
	virtual int remove(const char *path);
	virtual int close(int fd);
};

#endif
