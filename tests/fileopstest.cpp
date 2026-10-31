/******************************************************************************
 *
 *  fileopstest.cpp -	Test driver for FileMgr's pluggable FileOps: mounts a
 *			virtual path prefix onto a real directory through a
 *			FileOps subclass and then drives FileMgr, SWConfig
 *			and SWMgr through the virtual paths only.
 *
 *			This is the shape of a platform backend which can't
 *			reach its storage through ordinary paths (e.g. an
 *			Android shared folder behind a content provider):
 *			libsword sees "/sword-shared/..." and the FileOps
 *			turns that into whatever the platform needs.
 *
 * usage: fileopstest <realDir>
 *
 *   <realDir> is created/emptied and mounted as /sword-shared/
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

#include <iostream>
#include <string.h>
#include <filemgr.h>
#include <swconfig.h>
#include <swmgr.h>
#include <swmodule.h>

using namespace sword;
using std::cout;
using std::endl;

static const char *MOUNT = "/sword-shared";

/** Routes MOUNT/... to realRoot/...; everything else falls through to the
 * OS.  Counts calls so the test can prove the indirection was actually used.
 */
class MountFileOps : public FileOps {
	SWBuf realRoot;
	SWBuf mapped;
	// returns the real path for a virtual path, or 0 if path is not under the mount
	const char *map(const char *path) {
		int len = strlen(MOUNT);
		if (strncmp(path, MOUNT, len)) return 0;
		if (path[len] && path[len] != '/') return 0;
		mapped = realRoot;
		mapped += path + len;
		return mapped.c_str();
	}
public:
	int opens, accesses, stats, lists, mkdirs, removes;
	MountFileOps(const char *realRoot) : realRoot(realRoot), opens(0), accesses(0), stats(0), lists(0), mkdirs(0), removes(0) {}

	int open(const char *path, int mode, int perms) override {
		const char *real = map(path);
		if (!real) return FileOps::open(path, mode, perms);
		++opens;
		return FileOps::open(real, mode, perms);
	}
	bool hasAccess(const char *path, int mode) override {
		const char *real = map(path);
		if (!real) return FileOps::hasAccess(path, mode);
		++accesses;
		return FileOps::hasAccess(real, mode);
	}
	int stat(const char *path, DirEntry *st) override {
		const char *real = map(path);
		if (!real) return FileOps::stat(path, st);
		++stats;
		return FileOps::stat(real, st);
	}
	std::vector<struct DirEntry> listDir(const char *dirPath, bool includeSize, bool includeIsDirectory) override {
		const char *real = map(dirPath);
		if (!real) return FileOps::listDir(dirPath, includeSize, includeIsDirectory);
		++lists;
		// note: the base listDir stats entries through this->stat(), so we
		// must hand it a path it can map again, i.e., the virtual one.
		// Do our own listing of the real dir and stat via virtual paths.
		std::vector<struct DirEntry> entries = FileOps::listDir(real, false, false);
		SWBuf base = dirPath; base += "/";
		for (unsigned int i = 0; i < entries.size(); ++i) {
			if (includeIsDirectory || includeSize) {
				DirEntry st;
				if (!this->stat(base + entries[i].name, &st)) {
					entries[i].isDirectory = st.isDirectory;
					entries[i].modTime = st.modTime;
					if (!st.isDirectory && includeSize) entries[i].size = st.size;
				}
			}
		}
		return entries;
	}
	int mkdir(const char *path) override {
		const char *real = map(path);
		if (!real) return FileOps::mkdir(path);
		++mkdirs;
		return FileOps::mkdir(real);
	}
	int remove(const char *path) override {
		const char *real = map(path);
		if (!real) return FileOps::remove(path);
		++removes;
		return FileOps::remove(real);
	}
};


static void dumpDir(const char *vpath, int depth = 0) {
	std::vector<DirEntry> entries = FileMgr::getDirList(vpath, true, true);
	// deterministic order
	for (unsigned int i = 0; i < entries.size(); ++i)
		for (unsigned int j = i + 1; j < entries.size(); ++j)
			if (strcmp(entries[i].name.c_str(), entries[j].name.c_str()) > 0) { DirEntry t = entries[i]; entries[i] = entries[j]; entries[j] = t; }
	for (unsigned int i = 0; i < entries.size(); ++i) {
		for (int d = 0; d < depth; ++d) cout << "  ";
		cout << entries[i].name << (entries[i].isDirectory ? "/" : "");
		if (!entries[i].isDirectory) cout << " (" << entries[i].size << ")";
		cout << endl;
		if (entries[i].isDirectory) dumpDir(SWBuf(vpath) + "/" + entries[i].name, depth + 1);
	}
}


int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "usage: " << *argv << " <realDir>\n";
		return -1;
	}
	const char *realDir = argv[1];

	// start clean (through the plain OS ops)
	FileMgr::removeDir(realDir);
	FileMgr::createParent((SWBuf(realDir) + "/x").c_str());

	MountFileOps *ops = new MountFileOps(realDir);
	FileMgr::setSystemFileOps(ops);	// FileMgr owns it now

	SWBuf v = MOUNT;

	cout << "--- the virtual root doesn't exist on the OS, but does through FileMgr" << endl;
	cout << "OS sees " << MOUNT << ": " << (FileOps().hasAccess(MOUNT, 04) ? "yes" : "no") << endl;
	cout << "FileMgr sees " << MOUNT << ": " << (FileMgr::existsDir(MOUNT) ? "yes" : "no") << endl;

	cout << "--- createPathAndFile / write / stat" << endl;
	int fd = FileMgr::createPathAndFile(v + "/mods.d/hello.conf");
	cout << "created: " << (fd > 0 ? "ok" : "FAILED") << endl;
	const char *conf = "[Hello]\nDataPath=./modules/hello/\nModDrv=RawText\nDescription=Hello World\n";
	FileMgr::write(fd, conf, strlen(conf));
	FileMgr::closeFile(fd);
	cout << "size: " << FileMgr::getFileSize(v + "/mods.d/hello.conf") << endl;
	cout << "isDirectory(mods.d): " << (int)FileMgr::isDirectory(v + "/mods.d") << endl;
	cout << "modTime nonzero: " << (FileMgr::getFileModTime(v + "/mods.d/hello.conf") > 0) << endl;

	cout << "--- SWConfig round trip" << endl;
	{
		SWConfig cfg(v + "/mods.d/hello.conf");
		cout << "Description: " << cfg["Hello"]["Description"] << endl;
		cfg["Hello"]["Lang"] = "en";
		cfg.save();
	}
	{
		SWConfig cfg(v + "/mods.d/hello.conf");
		cout << "Lang after save: " << cfg["Hello"]["Lang"] << endl;
	}

	cout << "--- FileDesc open / getLine through FileMgr pool" << endl;
	{
		FileDesc *fdesc = FileMgr::getSystemFileMgr()->open(v + "/mods.d/hello.conf", FileMgr::RDONLY);
		SWBuf line;
		while (FileMgr::getLine(fdesc, line) && !line.trim().length());	// skip blank lines
		cout << "first line: " << line << endl;
		FileMgr::getSystemFileMgr()->close(fdesc);
	}

	cout << "--- copyDir / getDirList / removeDir" << endl;
	fd = FileMgr::createPathAndFile(v + "/modules/hello/ot");
	FileMgr::write(fd, "x", 1);
	FileMgr::closeFile(fd);
	FileMgr::copyDir(v + "/modules", v + "/backup/modules");
	dumpDir(MOUNT);
	FileMgr::removeDir(v + "/backup");
	cout << "backup after removeDir: " << (FileMgr::existsDir(v + "/backup") ? "still there" : "gone") << endl;

	cout << "--- SWMgr on the virtual path" << endl;
	{
		SWMgr mgr(MOUNT);
		cout << "prefixPath: " << mgr.prefixPath << endl;
		for (ModMap::iterator it = mgr.Modules.begin(); it != mgr.Modules.end(); ++it)
			cout << "module: " << it->second->getName() << " - " << it->second->getDescription() << endl;
	}

	cout << "--- real directory contents (through the OS, no mount)" << endl;
	{
		FileOps os;
		std::vector<DirEntry> entries = os.listDir(realDir, false, true);
		for (unsigned int i = 0; i < entries.size(); ++i)
			for (unsigned int j = i + 1; j < entries.size(); ++j)
				if (strcmp(entries[i].name.c_str(), entries[j].name.c_str()) > 0) { DirEntry t = entries[i]; entries[i] = entries[j]; entries[j] = t; }
		for (unsigned int i = 0; i < entries.size(); ++i)
			cout << entries[i].name << (entries[i].isDirectory ? "/" : "") << endl;
	}

	cout << "--- indirection was used: open " << (ops->opens > 0) << " access " << (ops->accesses > 0)
		<< " stat " << (ops->stats > 0) << " list " << (ops->lists > 0)
		<< " mkdir " << (ops->mkdirs > 0) << " remove " << (ops->removes > 0) << endl;

	// restore plain ops (deletes ours) and clean up
	FileMgr::setSystemFileOps(new FileOps());
	FileMgr::removeDir(realDir);
	return 0;
}
