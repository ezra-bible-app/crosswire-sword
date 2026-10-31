/******************************************************************************
 *
 *  sharedstoragefileops.cpp -	see sharedstoragefileops.hpp
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

#include <fcntl.h>
#include <string.h>
#include <swlog.h>
#include "sharedstoragefileops.hpp"
#include <mutex>

using namespace sword;

namespace {
	// copies a Java string out as UTF-8 and releases it
	SWBuf jstr(JNIEnv *env, jstring js) {
		SWBuf retVal;
		if (!js) return retVal;
		const char *c = env->GetStringUTFChars(js, nullptr);
		if (c) { retVal = c; env->ReleaseStringUTFChars(js, c); }
		return retVal;
	}
}


SharedStorageFileOps::SharedStorageFileOps(JNIEnv *env) {
	env->GetJavaVM(&vm);
	fName = fSize = fModTime = fIsDirectory = nullptr;

	jclass clazz = env->FindClass("org/crosswire/android/sword/SWMgr$SharedStorage");
	mOpen   = env->GetMethodID(clazz, "open",   "(Ljava/lang/String;Ljava/lang/String;Z)I");
	mStat   = env->GetMethodID(clazz, "stat",   "(Ljava/lang/String;)Lorg/crosswire/android/sword/SWMgr$FileInfo;");
	mList   = env->GetMethodID(clazz, "list",   "(Ljava/lang/String;)[Lorg/crosswire/android/sword/SWMgr$FileInfo;");
	mMkdir  = env->GetMethodID(clazz, "mkdir",  "(Ljava/lang/String;)Z");
	mRemove = env->GetMethodID(clazz, "remove", "(Ljava/lang/String;)Z");
	mClose  = env->GetMethodID(clazz, "close",  "(I)Z");
	env->DeleteLocalRef(clazz);

}


void SharedStorageFileOps::resolveFileInfo(JNIEnv *env, jobject fileInfo) {
	static std::mutex m;
	std::lock_guard<std::mutex> lock(m);
	if (fName) return;
	jclass fi = env->GetObjectClass(fileInfo);
	jfieldID n = env->GetFieldID(fi, "name", "Ljava/lang/String;");
	fSize        = env->GetFieldID(fi, "size", "J");
	fModTime     = env->GetFieldID(fi, "modTime", "J");
	fIsDirectory = env->GetFieldID(fi, "isDirectory", "Z");
	fName = n;		// last: readers test fName
	env->DeleteLocalRef(fi);
}


SharedStorageFileOps::~SharedStorageFileOps() {
	bool attached = false;
	JNIEnv *env = getEnv(attached);
	if (env) {
		for (size_t i = 0; i < mounts.size(); ++i) env->DeleteGlobalRef(mounts[i].storage);
	}
	if (attached) vm->DetachCurrentThread();
}


void SharedStorageFileOps::setMount(JNIEnv *env, const char *prefix, jobject storage) {
	SWBuf p = prefix;
	while (p.length() > 1 && p.endsWith("/")) p.setSize(p.length() - 1);
	jobject old = nullptr;
	{
		std::lock_guard<std::mutex> lock(mountsMutex);
		for (size_t i = 0; i < mounts.size(); ++i) {
			if (mounts[i].prefix == p) {
				old = mounts[i].storage;
				if (storage) mounts[i].storage = env->NewGlobalRef(storage);
				else mounts.erase(mounts.begin() + i);
				goto done;
			}
		}
		if (storage) {
			Mount m; m.prefix = p; m.storage = env->NewGlobalRef(storage);
			mounts.push_back(m);
		}
	}
done:
	// a call already inside the old storage keeps its own local ref to it
	if (old) env->DeleteGlobalRef(old);
}


bool SharedStorageFileOps::hasMount(const char *prefix) const {
	std::lock_guard<std::mutex> lock(mountsMutex);
	for (size_t i = 0; i < mounts.size(); ++i) if (mounts[i].prefix == prefix) return true;
	return false;
}


JNIEnv *SharedStorageFileOps::getEnv(bool &attached) {
	JNIEnv *env = nullptr;
	attached = false;
	jint status = vm->GetEnv((void **)&env, JNI_VERSION_1_6);
	if (status == JNI_EDETACHED) {
		if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
		attached = true;
	}
	else if (status != JNI_OK) return nullptr;
	return env;
}


jobject SharedStorageFileOps::rel(const char *path, SWBuf &rel) {
	std::lock_guard<std::mutex> lock(mountsMutex);
	for (size_t i = 0; i < mounts.size(); ++i) {
		const SWBuf &prefix = mounts[i].prefix;
		size_t len = prefix.length();
		if (strncmp(path, prefix.c_str(), len)) continue;
		if (path[len] && path[len] != '/') continue;
		const char *r = path + len;
		while (*r == '/') ++r;
		rel = r;
		// normalize away any trailing slashes
		while (rel.length() && rel.endsWith("/")) rel.setSize(rel.length() - 1);
		// our own ref: a setMount meanwhile may drop the mount's
		bool attached; JNIEnv *env = getEnv(attached);
		jobject mine = env ? env->NewGlobalRef(mounts[i].storage) : nullptr;
		if (attached) vm->DetachCurrentThread();
		return mine;
	}
	return nullptr;
}


// releases the ref rel() handed out when the operation is done
SharedStorageFileOps::Held::~Held() {
	if (!storage) return;
	bool attached; JNIEnv *env = ops->getEnv(attached);
	if (env) env->DeleteGlobalRef(storage);
	if (attached) ops->vm->DetachCurrentThread();
}


void SharedStorageFileOps::toEntry(JNIEnv *env, jobject fileInfo, DirEntry *entry) {
	resolveFileInfo(env, fileInfo);
	jstring name = (jstring)env->GetObjectField(fileInfo, fName);
	entry->name = jstr(env, name);
	if (name) env->DeleteLocalRef(name);
	entry->size = (unsigned long)env->GetLongField(fileInfo, fSize);
	entry->modTime = (long)env->GetLongField(fileInfo, fModTime);
	entry->isDirectory = env->GetBooleanField(fileInfo, fIsDirectory) == JNI_TRUE;
}


int SharedStorageFileOps::open(const char *path, int mode, int perms) {
	SWBuf r;
	Held held(this, rel(path, r));
	jobject storage = held.storage;
	if (!storage) return FileOps::open(path, mode, perms);

	const char *m = "r";
	if (mode & O_TRUNC) m = "rwt";
	else if (mode & O_APPEND) m = "wa";
	else if ((mode & O_ACCMODE) != O_RDONLY) m = "rw";
	bool create = (mode & O_CREAT) == O_CREAT;

	bool attached;
	JNIEnv *env = getEnv(attached);
	if (!env) return -1;
	jstring jp = env->NewStringUTF(r.c_str());
	jstring jm = env->NewStringUTF(m);
	int fd = env->CallIntMethod(storage, mOpen, jp, jm, create ? JNI_TRUE : JNI_FALSE);
	if (env->ExceptionCheck()) { env->ExceptionClear(); fd = -1; }
	if (fd >= 0) { std::lock_guard<std::mutex> lock(mountsMutex); issued[fd] = env->NewGlobalRef(storage); }	// ours until close()
	env->DeleteLocalRef(jp);
	env->DeleteLocalRef(jm);
	if (attached) vm->DetachCurrentThread();
	return fd;
}


bool SharedStorageFileOps::hasAccess(const char *path, int mode) {
	SWBuf r;
	Held held(this, rel(path, r));
	jobject storage = held.storage;
	if (!storage) return FileOps::hasAccess(path, mode);
	// a granted store is readable and writable as a whole; access == exists
	DirEntry entry;
	return !this->stat(path, &entry);
}


int SharedStorageFileOps::stat(const char *path, DirEntry *entry) {
	SWBuf r;
	Held held(this, rel(path, r));
	jobject storage = held.storage;
	if (!storage) return FileOps::stat(path, entry);

	bool attached;
	JNIEnv *env = getEnv(attached);
	if (!env) return -1;
	int retVal = -1;
	jstring jp = env->NewStringUTF(r.c_str());
	jobject info = env->CallObjectMethod(storage, mStat, jp);
	if (env->ExceptionCheck()) { env->ExceptionClear(); info = nullptr; }
	if (info) {
		toEntry(env, info, entry);
		env->DeleteLocalRef(info);
		retVal = 0;
	}
	env->DeleteLocalRef(jp);
	if (attached) vm->DetachCurrentThread();
	return retVal;
}


std::vector<struct DirEntry> SharedStorageFileOps::listDir(const char *dirPath, bool includeSize, bool includeIsDirectory) {
	SWBuf r;
	Held held(this, rel(dirPath, r));
	jobject storage = held.storage;
	if (!storage) return FileOps::listDir(dirPath, includeSize, includeIsDirectory);

	std::vector<struct DirEntry> dirList;
	bool attached;
	JNIEnv *env = getEnv(attached);
	if (!env) return dirList;
	jstring jp = env->NewStringUTF(r.c_str());
	jobjectArray infos = (jobjectArray)env->CallObjectMethod(storage, mList, jp);
	if (env->ExceptionCheck()) { env->ExceptionClear(); infos = nullptr; }
	if (infos) {
		jsize count = env->GetArrayLength(infos);
		for (jsize i = 0; i < count; ++i) {
			jobject info = env->GetObjectArrayElement(infos, i);
			if (!info) continue;
			struct DirEntry entry;
			toEntry(env, info, &entry);
			env->DeleteLocalRef(info);
			// the store tells us everything in one query; the flags are only hints
			dirList.push_back(entry);
		}
		env->DeleteLocalRef(infos);
	}
	env->DeleteLocalRef(jp);
	if (attached) vm->DetachCurrentThread();
	return dirList;
}


int SharedStorageFileOps::mkdir(const char *path) {
	SWBuf r;
	Held held(this, rel(path, r));
	jobject storage = held.storage;
	if (!storage) return FileOps::mkdir(path);
	if (!r.length()) return 0;	// the root always exists

	bool attached;
	JNIEnv *env = getEnv(attached);
	if (!env) return -1;
	jstring jp = env->NewStringUTF(r.c_str());
	jboolean ok = env->CallBooleanMethod(storage, mMkdir, jp);
	if (env->ExceptionCheck()) { env->ExceptionClear(); ok = JNI_FALSE; }
	env->DeleteLocalRef(jp);
	if (attached) vm->DetachCurrentThread();
	return ok ? 0 : -1;
}


int SharedStorageFileOps::close(int fd) {
	jobject storage = nullptr;
	{
		std::lock_guard<std::mutex> lock(mountsMutex);
		std::map<int, jobject>::iterator it = issued.find(fd);
		if (it != issued.end()) { storage = it->second; issued.erase(it); }
	}
	if (!storage) return FileOps::close(fd);
	bool attached;
	JNIEnv *env = getEnv(attached);
	if (!env) return FileOps::close(fd);
	jboolean handled = env->CallBooleanMethod(storage, mClose, (jint)fd);
	if (env->ExceptionCheck()) { env->ExceptionClear(); handled = JNI_FALSE; }
	env->DeleteGlobalRef(storage);
	if (attached) vm->DetachCurrentThread();
	return handled ? 0 : FileOps::close(fd);
}


int SharedStorageFileOps::remove(const char *path) {
	SWBuf r;
	Held held(this, rel(path, r));
	jobject storage = held.storage;
	if (!storage) return FileOps::remove(path);
	if (!r.length()) return -1;	// never remove the root

	bool attached;
	JNIEnv *env = getEnv(attached);
	if (!env) return -1;
	jstring jp = env->NewStringUTF(r.c_str());
	jboolean ok = env->CallBooleanMethod(storage, mRemove, jp);
	if (env->ExceptionCheck()) { env->ExceptionClear(); ok = JNI_FALSE; }
	env->DeleteLocalRef(jp);
	if (attached) vm->DetachCurrentThread();
	return ok ? 0 : -1;
}
