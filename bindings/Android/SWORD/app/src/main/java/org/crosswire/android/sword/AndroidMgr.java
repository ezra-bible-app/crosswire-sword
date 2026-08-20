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

	private Application app;
	private SafStorage sharedStorage;

	public AndroidMgr(Application app) {
		super(false);
		this.app = app;
		loadSharedStorage();
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

	/** the conventional shared library location offered to the user */
	public static final String DEFAULT_SHARED_FOLDER = "Documents/sword";

	/** the picker Intent for choosing a shared library folder; start it
	 * for a result and pass the returned data Uri to setSharedStorageUri.
	 * The conventional location (Documents/sword) is created if absent --
	 * an app may create its own folders under public storage without any
	 * permission -- and the picker opens there, so accepting the
	 * convention is a single tap. */
	public static Intent      createOpenSharedStorageIntent() {
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
		intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
			| Intent.FLAG_GRANT_WRITE_URI_PERMISSION
			| Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
			| Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
		try {
			File conventional = new File(Environment.getExternalStorageDirectory(), DEFAULT_SHARED_FOLDER + "/mods.d");
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
		sharedStorage = null;
		Context context = app.getApplicationContext();
		String uriString = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(PREF_SHARED_STORAGE_URI, null);
		if (uriString == null) return;
		Uri treeUri = Uri.parse(uriString);

		// still granted?
		boolean granted = false;
		for (UriPermission p : context.getContentResolver().getPersistedUriPermissions()) {
			if (p.getUri().equals(treeUri) && p.isReadPermission() && p.isWritePermission()) { granted = true; break; }
		}
		if (!granted) {
			Log.w(TAG, "grant for shared storage " + treeUri + " is gone; using private storage");
			return;
		}
		SafStorage storage = new SafStorage(context, treeUri);
		if (!storage.isAvailable()) {
			Log.w(TAG, "shared storage " + treeUri + " is not reachable; using private storage");
			return;
		}
		sharedStorage = storage;
	}
}
