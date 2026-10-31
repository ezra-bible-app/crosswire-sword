/******************************************************************************
 *
 *  installmgrtest.cpp -	Test driver for InstallMgr which exercises the
 *				same operations as utilities/installmgr.cpp but
 *				replaces the network transports with a mock
 *				transport which serves files from a local
 *				fixture directory.  This lets the testsuite
 *				exercise InstallMgr's source chaining / package
 *				preference / transport fallback logic without
 *				network access and deterministically.
 *
 * usage: installmgrtest <fixtureRoot> <privatePath> [options] <commands>
 *
 *   <fixtureRoot> is a local directory tree laid out as:
 *
 *       <fixtureRoot>/<scheme>/<host>/<path>
 *
 *   e.g., a request for https://crosswire.org/ftpmirror/pub/sword/raw/mods.d.tar.gz
 *   is served from <fixtureRoot>/https/crosswire.org/ftpmirror/pub/sword/raw/mods.d.tar.gz
 *   and ftp://ftp.crosswire.org/pub/sword/raw/mods.d/ is listed from
 *   <fixtureRoot>/ftp/ftp.crosswire.org/pub/sword/raw/mods.d/
 *
 *   <privatePath> is the InstallMgr private path (holds InstallMgr.conf and
 *   the local shadow of each remote source).  Modules are installed into
 *   <privatePath>/dest
 *
 *   options:
 *     --augment <path>       also augment the destination SWMgr from library
 *                            <path> (like a shared or second library), so
 *                            modules there can be listed and uninstalled
 *     --fail <scheme>[=<code>]        make every request through <scheme> fail
 *     --fail <urlPrefix>[=<code>]     make every request for a URL starting with
 *                            <urlPrefix> (e.g., https://host/path) fail
 *                            with transport error <code> (default -2, connection error)
 *
 *   commands (run in the order they are passed):
 *     -init                write a basic InstallMgr.conf (like installmgr -init)
 *     -sc                  refreshRemoteSourceConfiguration()
 *     -s                   list sources (including chained sources)
 *     -sn                  list source names only
 *     -conf                dump the [Sources] section of InstallMgr.conf
 *     -r <src>             refreshRemoteSource(src)
 *     -rl <src>            list modules available from remote source
 *     -ri <src> <mod>      install module from remote source into dest
 *     -l                   list installed modules
 *     -u <mod>             uninstall module
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

#include <installmgr.h>
#include <remotetrans.h>
#include <swmgr.h>
#include <swmodule.h>
#include <swconfig.h>
#include <filemgr.h>
#include <swlog.h>
#include <swbuf.h>

#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace sword;
using std::cout;
using std::cerr;
using std::vector;
using std::map;


// -----------------------------------------------------------------------
// Mock transport
// -----------------------------------------------------------------------

struct FailRule {
	SWBuf prefix;	// e.g., "https" (matches scheme) or "https://host/path" (matches URL prefix)
	int   code;
};

static SWBuf fixtureRoot;
static vector<FailRule> failRules;

// timeout sentinel: we set timeoutMillis to -1 in our constructor so we
// can tell whether InstallMgr passed its configured timeout to the transport.
static const long TIMEOUT_UNSET = -1;


static void splitURL(const char *url, SWBuf &scheme, SWBuf &host, SWBuf &path) {
	SWBuf u = url;
	scheme = u.stripPrefix(':');	// "https"
	u << 2;				// skip "//"
	host = u.stripPrefix('/');	// "crosswire.org"
	path = SWBuf("/") + u;		// "/ftpmirror/pub/sword/raw/..."
}


static bool dirEntryLessThan(const DirEntry &a, const DirEntry &b) { return strcmp(a.name.c_str(), b.name.c_str()) < 0; }


class MockTransport : public RemoteTransport {
	SWBuf scheme;

	int failCode(const SWBuf &s, const char *url) {
		for (unsigned int i = 0; i < failRules.size(); ++i) {
			const SWBuf &prefix = failRules[i].prefix;
			if (prefix.indexOf("://") < 0) {
				if (prefix == s) return failRules[i].code;
			}
			else if (SWBuf(url).startsWith(prefix)) return failRules[i].code;
		}
		return 0;
	}

	SWBuf localPath(const SWBuf &s, const SWBuf &h, const SWBuf &p) {
		return fixtureRoot + "/" + s + "/" + h + p;
	}

public:
	MockTransport(const char *scheme, const char *host, StatusReporter *sr) : RemoteTransport(host, sr) {
		this->scheme = scheme;
		timeoutMillis = TIMEOUT_UNSET;
	}

	virtual char getURL(const char *destPath, const char *sourceURL, SWBuf *destBuf = 0) {
		SWBuf s, h, p;
		splitURL(sourceURL, s, h, p);
		int code = failCode(s, sourceURL);
		if (!code) {
			SWBuf local = localPath(s, h, p);
			if (p.endsWith("/")) {
				// directory listing request from base RemoteTransport::getDirList
				// we override getDirList below, so this shouldn't happen
				code = -9;
			}
			else if (!FileMgr::existsFile(local.c_str())) {
				// mimic the curl transports: ftp reports remote file not
				// found (-4); http reports http error (-5)
				code = (s == "ftp" || s == "sftp") ? -4 : -5;
			}
			else if (destBuf) {
				FileDesc *fd = FileMgr::getSystemFileMgr()->open(local.c_str(), FileMgr::RDONLY);
				*destBuf = FileMgr::loadFile(fd);
				FileMgr::getSystemFileMgr()->close(fd);
			}
			else {
				if (FileMgr::copyFile(local.c_str(), destPath)) code = -9;
			}
		}
		if (term) code = -1;
		cout << "MOCK " << scheme << " GET " << sourceURL << " -> " << code
			<< " (timeout=" << ((timeoutMillis == TIMEOUT_UNSET) ? "UNSET" : SWBuf().setFormatted("%ld", timeoutMillis).c_str()) << ")\n";
		return (char)code;
	}

	virtual vector<struct DirEntry> getDirList(const char *dirURL) {
		vector<struct DirEntry> dirList;
		SWBuf s, h, p;
		splitURL(dirURL, s, h, p);
		int code = failCode(s, dirURL);
		if (!code) {
			SWBuf local = localPath(s, h, p);
			if (FileMgr::existsDir(local.c_str())) {
				dirList = FileMgr::getDirList(local.c_str(), true, true);
				// directory order is filesystem dependent; keep our output deterministic
				std::sort(dirList.begin(), dirList.end(), dirEntryLessThan);
			}
			else code = (s == "ftp" || s == "sftp") ? -4 : -5;
		}
		cout << "MOCK " << scheme << " LIST " << dirURL << " -> ";
		if (code) cout << code;
		else cout << dirList.size() << " entries";
		cout << "\n";
		return dirList;
	}
};


// -----------------------------------------------------------------------
// InstallMgr subclass which uses our mock transports
// -----------------------------------------------------------------------

class TestInstallMgr : public InstallMgr {
public:
	TestInstallMgr(const char *privatePath) : InstallMgr(privatePath, 0) {}

	virtual RemoteTransport *createFTPTransport(const char *host, StatusReporter *sr) {
		return new MockTransport("ftp", host, sr);
	}
	virtual RemoteTransport *createHTTPTransport(const char *host, StatusReporter *sr) {
		return new MockTransport("http", host, sr);
	}
	virtual bool isUserDisclaimerConfirmed() const { return true; }
};


// -----------------------------------------------------------------------
// commands
// -----------------------------------------------------------------------

static SWBuf privatePath;
static SWBuf destPath;
static SWBuf confPath;
static std::vector<SWBuf> augmentPaths;
static TestInstallMgr *installMgr = 0;
static SWMgr *destMgr = 0;


static void init() {
	if (!installMgr) {
		FileMgr::createParent((destPath + "/mods.d/globals.conf").c_str());
		installMgr = new TestInstallMgr(privatePath.c_str());
		installMgr->setUserDisclaimerConfirmed(true);
	}
	if (!destMgr) {
		destMgr = new SWMgr(destPath.c_str(), true, 0, false, false);
		for (unsigned int i = 0; i < augmentPaths.size(); ++i)
			destMgr->augmentModules(augmentPaths[i].c_str(), false);
	}
}


static void reloadDest() {
	delete destMgr;
	destMgr = 0;
	init();
}


static void createBasicConfig() {
	FileMgr::createParent(confPath.c_str());
	FileMgr::removeFile(confPath.c_str());

	// mirrors utilities/installmgr.cpp createBasicConfig
	InstallSource is("FTP");
	is.caption = "CrossWire";
	is.source = "ftp.crosswire.org";
	is.directory = "/pub/sword/raw";
	is.uid = "20081216195754";

	SWConfig config(confPath.c_str());
	config["General"]["PassiveFTP"] = "true";
	config["General"]["TimeoutMillis"] = "10000";
	config["General"]["UnverifiedPeerAllowed"] = "true";
	config["Sources"]["FTPSource"] = is.getConfEnt();
	config.save();
	cout << "Initialized basic config\n";
	if (installMgr) installMgr->readInstallConf();
}


static void syncConfig() {
	init();
	int result = installMgr->refreshRemoteSourceConfiguration();
	cout << "refreshRemoteSourceConfiguration: " << (result ? "FAILED" : "OK") << " (" << result << ")\n";
}


static void listSources(bool namesOnly) {
	init();
	if (!namesOnly) cout << "Remote Sources:\n";
	for (InstallSourceMap::iterator it = installMgr->sources.begin(); it != installMgr->sources.end(); ++it) {
		if (namesOnly) {
			cout << it->second->caption << "\n";
			continue;
		}
		cout << "[" << it->second->caption << "]\n";
		int depth = 0;
		for (InstallSource *is = it->second; is; is = is->chainedSource, ++depth) {
			SWBuf indent = "\t";
			if (depth) indent.appendFormatted("(chained %d) ", depth);
			cout << indent << "Type     : " << is->type << (is->packagePreference ? " (PackagePreference)" : "") << "\n";
			cout << indent << "Source   : " << is->source << "\n";
			cout << indent << "Directory: " << is->directory << "\n";
			cout << indent << "UID      : " << is->uid << "\n";
		}
	}
}


static void dumpConf() {
	SWConfig config(confPath.c_str());
	cout << "[Sources]\n";
	SectionMap::iterator section = config.getSections().find("Sources");
	if (section != config.getSections().end()) {
		for (ConfigEntMap::iterator it = section->second.begin(); it != section->second.end(); ++it) {
			cout << it->first << "=" << it->second << "\n";
		}
	}
}


static InstallSource *findSource(const char *sourceName) {
	init();
	InstallSourceMap::iterator source = installMgr->sources.find(sourceName);
	if (source == installMgr->sources.end()) {
		cout << "Couldn't find remote source [" << sourceName << "]\n";
		return 0;
	}
	return source->second;
}


static void refreshRemoteSource(const char *sourceName) {
	InstallSource *is = findSource(sourceName);
	if (!is) return;
	int result = installMgr->refreshRemoteSource(is);
	cout << "refreshRemoteSource(" << sourceName << "): " << (result ? "FAILED" : "OK") << " (" << result << ")\n";
}


static void listModules(SWMgr *mgr, const char *title) {
	cout << title << "\n";
	int count = 0;
	for (ModMap::const_iterator it = mgr->getModules().begin(); it != mgr->getModules().end(); ++it) {
		SWModule *module = it->second;
		const char *version = module->getConfigEntry("Version");
		cout << "\t[" << module->getName() << "] (" << (version ? version : "") << ") - " << module->getDescription() << "\n";
		++count;
	}
	cout << "\t" << count << " modules\n";
}


static void remoteListModules(const char *sourceName) {
	InstallSource *is = findSource(sourceName);
	if (!is) return;
	SWBuf title = "Available Modules from [";
	title += sourceName;
	title += "]:";
	listModules(is->getMgr(), title.c_str());
}


static void remoteInstallModule(const char *sourceName, const char *modName) {
	InstallSource *is = findSource(sourceName);
	if (!is) return;
	SWModule *module = is->getMgr()->getModule(modName);
	if (!module) {
		cout << "Remote source [" << sourceName << "] does not make available module [" << modName << "]\n";
		return;
	}
	int result = installMgr->installModule(destMgr, 0, module->getName(), is);
	cout << "installModule(" << sourceName << ", " << modName << "): " << (result ? "FAILED" : "OK") << " (" << result << ")\n";
	reloadDest();
}


static void listInstalled() {
	reloadDest();
	listModules(destMgr, "Installed Modules:");
	// show install source recording
	for (ModMap::const_iterator it = destMgr->getModules().begin(); it != destMgr->getModules().end(); ++it) {
		const char *uid = it->second->getConfigEntry("InstallSourceID");
		const char *cap = it->second->getConfigEntry("InstallSourceCaption");
		cout << "\t[" << it->second->getName() << "] InstallSourceID=" << (uid ? uid : "") << " InstallSourceCaption=" << (cap ? cap : "") << "\n";
	}
}


static void uninstallModule(const char *modName) {
	init();
	SWModule *module = destMgr->getModule(modName);
	if (!module) {
		cout << "Couldn't find module [" << modName << "] to remove\n";
		return;
	}
	int result = installMgr->removeModule(destMgr, module->getName());
	cout << "removeModule(" << modName << "): " << (result ? "FAILED" : "OK") << " (" << result << ")\n";
	reloadDest();
}


static void usage(const char *progName, const char *error = 0) {
	if (error) cerr << progName << ": " << error << "\n";
	cerr << "usage: " << progName << " <fixtureRoot> <privatePath> [--fail <scheme>[://host][=code]]... <commands>\n";
	cerr << "  see header of installmgrtest.cpp for details\n";
	exit(-1);
}


int main(int argc, char **argv) {

	if (argc < 4) usage(*argv);

	fixtureRoot = argv[1];
	privatePath = argv[2];
	if (fixtureRoot.endsWith("/")) fixtureRoot.setSize(fixtureRoot.size()-1);
	if (privatePath.endsWith("/")) privatePath.setSize(privatePath.size()-1);
	confPath = privatePath + "/InstallMgr.conf";
	destPath = privatePath + "/dest";

	SWLog::getSystemLog()->setLogLevel(SWLog::LOG_ERROR);

	for (int i = 3; i < argc; i++) {
		if (!strcmp(argv[i], "-d")) {
			SWLog::getSystemLog()->setLogLevel(SWLog::LOG_DEBUG);
		}
		else if (!strcmp(argv[i], "--augment")) {
			if (i+1 >= argc) usage(*argv, "--augment requires <path>");
			augmentPaths.push_back(argv[++i]);
		}
		else if (!strcmp(argv[i], "--fail")) {
			if (i+1 >= argc) usage(*argv, "--fail requires <scheme>[://host][=code]");
			SWBuf rule = argv[++i];
			FailRule fr;
			fr.code = -2;
			const char *eq = strchr(rule.c_str(), '=');
			if (eq) {
				fr.code = atoi(eq+1);
				rule.setSize(eq - rule.c_str());
			}
			fr.prefix = rule;
			failRules.push_back(fr);
			cout << "MOCK FAIL RULE: " << fr.prefix << ((fr.prefix.indexOf("://") < 0) ? "://*" : "*") << " -> " << fr.code << "\n";
		}
		else if (!strcmp(argv[i], "-init")) {
			createBasicConfig();
		}
		else if (!strcmp(argv[i], "-sc")) {
			syncConfig();
		}
		else if (!strcmp(argv[i], "-s")) {
			listSources(false);
		}
		else if (!strcmp(argv[i], "-sn")) {
			listSources(true);
		}
		else if (!strcmp(argv[i], "-conf")) {
			dumpConf();
		}
		else if (!strcmp(argv[i], "-r")) {
			if (i+1 < argc) refreshRemoteSource(argv[++i]);
			else usage(*argv, "-r requires <remoteSrcName>");
		}
		else if (!strcmp(argv[i], "-rl")) {
			if (i+1 < argc) remoteListModules(argv[++i]);
			else usage(*argv, "-rl requires <remoteSrcName>");
		}
		else if (!strcmp(argv[i], "-ri")) {
			if (i+2 < argc) {
				const char *source = argv[++i];
				const char *modName = argv[++i];
				remoteInstallModule(source, modName);
			}
			else usage(*argv, "-ri requires <remoteSrcName> <modName>");
		}
		else if (!strcmp(argv[i], "-l")) {
			listInstalled();
		}
		else if (!strcmp(argv[i], "-u")) {
			if (i+1 < argc) uninstallModule(argv[++i]);
			else usage(*argv, "-u requires <modName>");
		}
		else usage(*argv, (((SWBuf)"unknown argument: ")+ argv[i]).c_str());
	}

	delete installMgr;
	delete destMgr;
	return 0;
}
