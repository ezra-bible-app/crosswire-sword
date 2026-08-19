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


SharedStorageFileOps::SharedStorageFileOps(JNIEnv *env, jobject storage) {
	env->GetJavaVM(&vm);
	this->storage = env->NewGlobalRef(storage);

	jclass clazz = env->GetObjectClass(storage);
	mOpen   = env->GetMethodID(clazz, "open",   "(Ljava/lang/String;Ljava/lang/String;Z)I");
	mStat   = env->GetMethodID(clazz, "stat",   "(Ljava/lang/String;)Lorg/crosswire/android/sword/SWMgr$FileInfo;");
	mList   = env->GetMethodID(clazz, "list",   "(Ljava/lang/String;)[Lorg/crosswire/android/sword/SWMgr$FileInfo;");
	mMkdir  = env->GetMethodID(clazz, "mkdir",  "(Ljava/lang/String;)Z");
	mRemove = env->GetMethodID(clazz, "remove", "(Ljava/lang/String;)Z");
	env->DeleteLocalRef(clazz);

	jclass fi = env->FindClass("org/crosswire/android/sword/SWMgr$FileInfo");
	fileInfoClass = (jclass)env->NewGlobalRef(fi);
	fName        = env->GetFieldID(fi, "name", "Ljava/lang/String;");
	fSize        = env->GetFieldID(fi, "size", "J");
	fModTime     = env->GetFieldID(fi, "modTime", "J");
	fIsDirectory = env->GetFieldID(fi, "isDirectory", "Z");
	env->DeleteLocalRef(fi);
}


SharedStorageFileOps::~SharedStorageFileOps() {
	bool attached = false;
	JNIEnv *env = getEnv(attached);
	if (env) {
		env->DeleteGlobalRef(storage);
		env->DeleteGlobalRef(fileInfoClass);
	}
	if (attached) vm->DetachCurrentThread();
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


bool SharedStorageFileOps::rel(const char *path, SWBuf &rel) {
	static const size_t len = strlen(SHARED_STORAGE_PATH);
	if (strncmp(path, SHARED_STORAGE_PATH, len)) return false;
	if (path[len] && path[len] != '/') return false;
	const char *r = path + len;
	while (*r == '/') ++r;
	rel = r;
	// normalize away any trailing slashes
	while (rel.length() && rel.endsWith("/")) rel.setSize(rel.length() - 1);
	return true;
}


void SharedStorageFileOps::toEntry(JNIEnv *env, jobject fileInfo, DirEntry *entry) {
	jstring name = (jstring)env->GetObjectField(fileInfo, fName);
	entry->name = jstr(env, name);
	if (name) env->DeleteLocalRef(name);
	entry->size = (unsigned long)env->GetLongField(fileInfo, fSize);
	entry->modTime = (long)env->GetLongField(fileInfo, fModTime);
	entry->isDirectory = env->GetBooleanField(fileInfo, fIsDirectory) == JNI_TRUE;
}


int SharedStorageFileOps::open(const char *path, int mode, int perms) {
	SWBuf r;
	if (!rel(path, r)) return FileOps::open(path, mode, perms);

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
	env->DeleteLocalRef(jp);
	env->DeleteLocalRef(jm);
	if (attached) vm->DetachCurrentThread();
	return fd;
}


bool SharedStorageFileOps::hasAccess(const char *path, int mode) {
	SWBuf r;
	if (!rel(path, r)) return FileOps::hasAccess(path, mode);
	// a granted store is readable and writable as a whole; access == exists
	DirEntry entry;
	return !this->stat(path, &entry);
}


int SharedStorageFileOps::stat(const char *path, DirEntry *entry) {
	SWBuf r;
	if (!rel(path, r)) return FileOps::stat(path, entry);

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
	if (!rel(dirPath, r)) return FileOps::listDir(dirPath, includeSize, includeIsDirectory);

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
	if (!rel(path, r)) return FileOps::mkdir(path);
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


int SharedStorageFileOps::remove(const char *path) {
	SWBuf r;
	if (!rel(path, r)) return FileOps::remove(path);
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
