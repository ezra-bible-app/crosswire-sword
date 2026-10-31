/******************************************************************************
 *
 *  SWMgr.java -	
 *
 * $Id$
 *
 * Copyright 2009-2013 CrossWire Bible Society (http://www.crosswire.org)
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

package org.crosswire.android.sword;

public class SWMgr {
	
	static {
		System.loadLibrary("sword");
	}

	public static class ModInfo {
		public String name;
		public String description;
		public String category;
		public String language;
		public String version;
		public String delta;
		public String cipherKey;
		public String[] features;
	}

	public SWMgr() {
		reInit();
	}
	public SWMgr(boolean init) {
		if (init) reInit();
	}

	public native String version();
	public native void reInit();

	public native ModInfo[]   getModInfoList();
	public native SWModule    getModuleByName(String name);
	public native String      getPrefixPath();
	public native String      getConfigPath();
	/** where the frontend keeps small per-user study data (bookmarks/, ...):
	 *  the data folder from getDataStorage() if set, else beside the module
	 *  library; ends in '/' */
	public native String      getDataPath();

	// plain file access routed through the engine's FileMgr, so paths
	// under the shared (SAF) library work the same as private ones
	public native String      readFile(String path);		// null if unreadable
	public native boolean     writeFile(String path, String contents);	// creates parent dirs
	public native String[]    listDir(String path);		// subdirectories end in "/"
	public native boolean     removeFile(String path);
	public native boolean     mkdir(String path);
	public native void        setGlobalOption(String option, String value);
	public native String      getGlobalOption(String option);
	public native String      getGlobalOptionTip(String option);
	public native String      filterText(String filterName, String text);
	public native String[]    getGlobalOptions();
	public native String[]    getGlobalOptionValues(String option);
	public native void        setCipherKey(String modName, String key);
	public native void        setJavascript(boolean val);
	public native String[]    getAvailableLocales();
	public native void        setDefaultLocale(String name);
	public native String      translate(String text, String localeName);

	/**
	 * add a conf snippet to extraConfig.  This is useful for adding a config section
	 * sent from a module unlock key app.
	 * @param blob
	 * @return an array of section names which were contained in the blob
	 */
	public native String[]    addExtraConfig(String blob);
	public native void        setExtraConfigValue(String section, String key, String value);
	public native String[]    getExtraConfigSections();
	public native String[]    getExtraConfigKeys(String section);
	public native String      getExtraConfigValue(String section, String key);

	/**
	 * override in platform descendant, e.g., AndroidSWMgr
	 */
	public String             getStorageBasePath() {
		return ".";
	}

	/**
	 * The virtual path under which the native library sees a SharedStorage
	 * (see getSharedStorage()).  e.g., the module library of a SharedStorage
	 * is at SHARED_STORAGE_PATH + "/mods.d"
	 */
	public static final String SHARED_STORAGE_PATH = "/sword-shared";
	/** likewise for the study-data folder of getDataStorage() */
	public static final String DATA_STORAGE_PATH = "/sword-data";

	public static class FileInfo {
		public String name;
		public long size;
		public long modTime;		// seconds since the epoch; 0 if unknown
		public boolean isDirectory;
	}

	/**
	 * A file store the native library can only reach through a callback
	 * rather than an ordinary path, e.g., an Android folder shared between
	 * apps and granted through the Storage Access Framework.  The library
	 * addresses it as SHARED_STORAGE_PATH + "/" + path and FileMgr routes
	 * those paths here; everything else goes straight to the OS.
	 *
	 * paths are relative to the store root, '/'-separated, never start
	 * with '/'; "" is the root itself.
	 */
	public static interface SharedStorage {
		/**
		 * @param mode "r", "rw", "rwt" (truncate), or "wa" (append)
		 * @param create create the file if it doesn't exist
		 * @return a detached OS file descriptor (the caller owns it and
		 *	will read/write/seek/close it directly), or -1 on failure
		 */
		public int        open(String path, String mode, boolean create);
		/** @return info about path, or null if it doesn't exist */
		public FileInfo   stat(String path);
		/** @return contents of directory path (excluding . and ..), or null if not a directory */
		public FileInfo[] list(String path);
		/** create one directory; parent must exist */
		public boolean    mkdir(String path);
		/** remove a file or empty directory */
		public boolean    remove(String path);
		/** the library is done with a descriptor from open().  Return true
		 *  if this store closed it (and committed whatever that entails,
		 *  e.g. uploading a staged write); false to let the caller close
		 *  it as an ordinary descriptor */
		public boolean    close(int fd);
	}

	/**
	 * override in platform descendant to expose a shared module library
	 * which can't be reached through ordinary paths.
	 * @return null if the library should use plain paths only
	 */
	public SharedStorage      getSharedStorage() {
		return null;
	}
	/**
	 * override in platform descendant to expose a separately granted folder
	 * for small per-user study data (bookmarks/, ...), e.g. a cloud-synced
	 * folder.  @return null to keep study data beside the module library
	 */
	public SharedStorage      getDataStorage() {
		return null;
	}
	public static interface BibleSyncListener {
		public void navReceived(String osisRef);
		public void chatReceived(String user, String message);
		/** the interface address BibleSync is on */
		public void infoReceived(String iface);
		/**
		 * @param state 'A' a peer announced itself, 'S' it is a speaker we now listen to, 'D' a speaker went quiet
		 * @param who user @ address ('D': the uuid)
		 * @param what app version; device; uuid ('D': empty)
		 */
		public void peerReceived(char state, String who, String what);
		/** the library could not send, or stopped itself: what it said, and its detail */
		public void errorReceived(String message, String detail);
	}
	public native void        startBibleSync(String appName, String userName, String passphrase, BibleSyncListener listener);
	/**
	 * For the next startBibleSync; a negative value leaves that one as it is.
	 * @param repeatCount how many more times each verse is sent (nobody resends multicast); default 2
	 * @param repeatDelayMS at least this long between those; default 500
	 * @param peerLifetimeS a peer not heard from for this many seconds is given up; default 1800
	 */
	public native void        setBibleSyncOptions(int repeatCount, int repeatDelayMS, int peerLifetimeS);
	public native void        stopBibleSync();
	/**
	 * @return "off" (not running), "alive <address>",
	 * "disabled <address>" (the library shut itself down) or
	 * "moved <address> <address now>" (the network changed under it)
	 */
	public native String      getBibleSyncStatus();
	public native void        sendBibleSyncMessage(String osisRef);
}


