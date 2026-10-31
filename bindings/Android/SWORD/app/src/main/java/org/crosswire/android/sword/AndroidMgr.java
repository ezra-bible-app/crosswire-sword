/******************************************************************************
 *
 *  AndroidMgr.java -
 *
 * $Id: SWMgr.java 2833 2013-06-29 06:40:28Z chrislit $
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

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.UriPermission;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.util.Log;

import java.io.File;

public class AndroidMgr extends SWMgr {

	private static final String TAG = "AndroidMgr";
	/** SharedPreferences file and key under which the shared library tree Uri is kept */
	public static final String PREFS = "org.crosswire.android.sword";
	public static final String PREF_SHARED_STORAGE_URI = "sharedStorageUri";
	/** ... and the study-data (bookmarks etc.) tree Uri */
	public static final String PREF_DATA_STORAGE_URI = "dataStorageUri";
	/** the last data folder chosen, kept after "keep with library" so the picker can reopen there */
	public static final String PREF_LAST_DATA_STORAGE_URI = "lastDataStorageUri";

	private Application app;
	private SafStorage sharedStorage;
	private SafStorage dataStorage;

	public AndroidMgr(Application app) {
		super(false);
		this.app = app;
		loadSharedStorage();
		loadDataStorage();
		reInit();
	}

	@Override
	public String             getStorageBasePath() {
		Context context = app.getApplicationContext();
		return context.getFilesDir().getAbsolutePath();
	}

	/**
	 * The shared module library, if the user has picked one (see
	 * setSharedStorageUri); null means this app's private storage only.
	 */
	@Override
	public SharedStorage      getSharedStorage() {
		return sharedStorage;
	}

	public Uri                getSharedStorageUri() {
		return (sharedStorage != null) ? sharedStorage.getTreeUri() : null;
	}

	/**
	 * Use (or stop using, with null) a document tree as the shared module
	 * library.  Typically the result of Intent.ACTION_OPEN_DOCUMENT_TREE:
	 * the grant is persisted and remembered across restarts, and the
	 * library is re-initialized on it.
	 */
	public void               setSharedStorageUri(Uri treeUri) {
		Context context = app.getApplicationContext();
		SharedPreferences prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
		if (treeUri != null) {
			try {
				context.getContentResolver().takePersistableUriPermission(treeUri,
					Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
			}
			catch (SecurityException e) {
				Log.w(TAG, "could not persist grant for " + treeUri + ": " + e);
			}
			prefs.edit().putString(PREF_SHARED_STORAGE_URI, treeUri.toString()).apply();
		}
		else {
			prefs.edit().remove(PREF_SHARED_STORAGE_URI).apply();
		}
		loadSharedStorage();
		reInit();
	}

	/**
	 * The study-data folder (bookmarks/, ...), if the user has picked one
	 * separately from the library -- typically a cloud-synced folder; null
	 * means study data lives beside the module library.
	 */
	@Override
	public SharedStorage      getDataStorage() {
		return dataStorage;
	}

	public Uri                getDataStorageUri() {
		return (dataStorage != null) ? dataStorage.getTreeUri() : null;
	}

	/** use (or stop using, with null) a document tree as the study-data folder */
	public void               setDataStorageUri(Uri treeUri) {
		Context context = app.getApplicationContext();
		SharedPreferences prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
		if (treeUri != null) {
			try {
				context.getContentResolver().takePersistableUriPermission(treeUri,
					Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
			}
			catch (SecurityException e) {
				Log.w(TAG, "could not persist grant for " + treeUri + ": " + e);
			}
			prefs.edit().putString(PREF_DATA_STORAGE_URI, treeUri.toString()).putString(PREF_LAST_DATA_STORAGE_URI, treeUri.toString()).apply();
		}
		else {
			prefs.edit().remove(PREF_DATA_STORAGE_URI).apply();
		}
		loadDataStorage();
		reInit();
	}

	/** the picker Intent for choosing a study-data folder; no conventional
	 * location is pre-created -- the point of a separate folder is that
	 * the user puts it somewhere of their own, e.g. on a cloud provider */
	public Intent             createOpenDataStorageIntent() {
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
		intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
			| Intent.FLAG_GRANT_WRITE_URI_PERMISSION
			| Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
			| Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
		// reopen where the user last chose (the remembered uri, even if its grant has lapsed)
		String last = app.getApplicationContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(PREF_DATA_STORAGE_URI, null);
		if (last == null) last = app.getApplicationContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(PREF_LAST_DATA_STORAGE_URI, null);
		if (last != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			try {
				Uri tree = Uri.parse(last);
				intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, DocumentsContract.buildDocumentUriUsingTree(tree, DocumentsContract.getTreeDocumentId(tree)));
			}
			catch (Exception e) { }
		}
		return intent;
	}

	private void              loadDataStorage() {
		dataStorage = loadGrantedStorage(PREF_DATA_STORAGE_URI, "data storage");
	}

	/** the conventional shared library location offered to the user */
	public static final String DEFAULT_SHARED_FOLDER = "Documents/sword";

	/** the picker Intent for choosing a shared library folder; start it
	 * for a result and pass the returned data Uri to setSharedStorageUri.
	 * The conventional location (Documents/sword) is created if absent --
	 * an app may create its own folders under public storage without any
	 * permission -- and the picker opens there, so accepting the
	 * convention is a single tap.  Only the folder itself is created:
	 * library scaffolding (mods.d) is laid down through the grant after
	 * selection, and showing a mods.d in the picker would tempt the user
	 * to descend into it and grant the wrong tree. */
	public static Intent      createOpenSharedStorageIntent() {
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
		intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
			| Intent.FLAG_GRANT_WRITE_URI_PERMISSION
			| Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
			| Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
		try {
			File conventional = new File(Environment.getExternalStorageDirectory(), DEFAULT_SHARED_FOLDER);
			if (!conventional.isDirectory() && !conventional.mkdirs())
				Log.i(TAG, "could not pre-create " + conventional + "; picker will open at its default location");
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				Uri initial = DocumentsContract.buildDocumentUri("com.android.externalstorage.documents", "primary:" + DEFAULT_SHARED_FOLDER);
				intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, initial);
			}
		}
		catch (Exception e) {
			Log.w(TAG, "could not prepare conventional shared folder: " + e);
		}
		return intent;
	}

	private void              loadSharedStorage() {
		sharedStorage = loadGrantedStorage(PREF_SHARED_STORAGE_URI, "shared storage");
	}

	/** the SafStorage for a remembered tree grant, or null if none / gone / unreachable */
	private SafStorage        loadGrantedStorage(String prefKey, String what) {
		Context context = app.getApplicationContext();
		String uriString = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(prefKey, null);
		if (uriString == null) return null;
		Uri treeUri = Uri.parse(uriString);

		// still granted?
		boolean granted = false;
		for (UriPermission p : context.getContentResolver().getPersistedUriPermissions()) {
			if (p.getUri().equals(treeUri) && p.isReadPermission() && p.isWritePermission()) { granted = true; break; }
		}
		if (!granted) {
			Log.w(TAG, "grant for " + what + " " + treeUri + " is gone; ignoring it");
			return null;
		}
		SafStorage storage = new SafStorage(context, treeUri);
		if (!storage.isAvailable()) {
			Log.w(TAG, what + " " + treeUri + " is not reachable; ignoring it");
			return null;
		}
		return storage;
	}
}
