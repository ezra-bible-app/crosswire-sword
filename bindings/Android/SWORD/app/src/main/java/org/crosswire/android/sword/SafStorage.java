/******************************************************************************
 *
 *  SafStorage.java -	SWMgr.SharedStorage over a Storage Access Framework
 *			document tree, so several SWORD apps can share one
 *			module library (e.g., Documents/sword) on Android 11+
 *			where ordinary paths outside an app's own sandbox are
 *			no longer accessible.
 *
 *			The app obtains the tree once with
 *			Intent.ACTION_OPEN_DOCUMENT_TREE, persists the grant
 *			with ContentResolver.takePersistableUriPermission(),
 *			and hands the tree Uri to AndroidMgr.
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

package org.crosswire.android.sword;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SafStorage implements SWMgr.SharedStorage {

	private static final String TAG = "SafStorage";
	private static final String[] COLUMNS = {
		Document.COLUMN_DOCUMENT_ID,
		Document.COLUMN_DISPLAY_NAME,
		Document.COLUMN_MIME_TYPE,
		Document.COLUMN_SIZE,
		Document.COLUMN_LAST_MODIFIED,
	};

	private final ContentResolver resolver;
	private final Uri treeUri;
	private final String treeDocId;
	// relative path -> document id, learned from listings and creations
	private final Map<String, String> docIds = new HashMap<String, String>();

	public SafStorage(Context context, Uri treeUri) {
		this.resolver = context.getContentResolver();
		this.treeUri = treeUri;
		this.treeDocId = DocumentsContract.getTreeDocumentId(treeUri);
		docIds.put("", treeDocId);
	}

	public Uri getTreeUri() { return treeUri; }

	/** true if we still hold a grant for the tree and it is reachable */
	public boolean isAvailable() {
		return stat("") != null;
	}

	// ------------------------------------------------------------------
	// path resolution

	private static String parentOf(String path) {
		int i = path.lastIndexOf('/');
		return (i < 0) ? "" : path.substring(0, i);
	}

	private static String nameOf(String path) {
		int i = path.lastIndexOf('/');
		return (i < 0) ? path : path.substring(i + 1);
	}

	private Uri docUri(String docId) {
		return DocumentsContract.buildDocumentUriUsingTree(treeUri, docId);
	}

	/** document id for a relative path, or null if it doesn't exist.
	 * Resolves the parent (recursively, usually from cache), then lists
	 * it: one query caches every sibling, and a miss costs just that
	 * listing rather than a provider exception per path component.  A
	 * listing on every miss also means a file another app just added
	 * is seen right away.
	 */
	private String resolve(String path) {
		if (path.length() == 0) return treeDocId;
		String known;
		synchronized (this) { known = docIds.get(path); }
		if (known != null) return known;

		String parent = parentOf(path);
		String parentId = resolve(parent);
		if (parentId == null) return null;
		listChildren(parent, parentId);	// populates docIds
		synchronized (this) { return docIds.get(path); }
	}

	private SWMgr.FileInfo fromCursor(Cursor c) {
		SWMgr.FileInfo info = new SWMgr.FileInfo();
		info.name = c.getString(1);
		String mime = c.getString(2);
		info.isDirectory = Document.MIME_TYPE_DIR.equals(mime);
		info.size = c.isNull(3) ? 0 : c.getLong(3);
		info.modTime = c.isNull(4) ? 0 : c.getLong(4) / 1000;
		return info;
	}

	private SWMgr.FileInfo queryOne(Uri uri) {
		Cursor c = null;
		try {
			c = resolver.query(uri, COLUMNS, null, null, null);
			if (c != null && c.moveToFirst()) return fromCursor(c);
		}
		catch (Exception e) {
			// IllegalArgumentException / SecurityException: doesn't exist or no grant
		}
		finally {
			if (c != null) c.close();
		}
		return null;
	}

	private List<SWMgr.FileInfo> listChildren(String dirPath, String dirId) {
		List<SWMgr.FileInfo> entries = new ArrayList<SWMgr.FileInfo>();
		Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, dirId);
		String prefix = (dirPath.length() == 0) ? "" : dirPath + "/";
		Cursor c = null;
		try {
			c = resolver.query(childrenUri, COLUMNS, null, null, null);
			if (c != null) {
				while (c.moveToNext()) {
					SWMgr.FileInfo info = fromCursor(c);
					synchronized (this) { docIds.put(prefix + info.name, c.getString(0)); }
					entries.add(info);
				}
			}
		}
		catch (Exception e) {
			Log.w(TAG, "list failed for " + dirPath + ": " + e);
			return null;
		}
		finally {
			if (c != null) c.close();
		}
		return entries;
	}

	private synchronized void forget(String path) {
		docIds.remove(path);
		// anything below it is gone too
		String prefix = path + "/";
		for (String k : new ArrayList<String>(docIds.keySet()))
			if (k.startsWith(prefix)) docIds.remove(k);
	}

	// ------------------------------------------------------------------
	// SWMgr.SharedStorage

	@Override
	public int open(String path, String mode, boolean create) {
		String id = resolve(path);
		if (id == null) {
			if (!create) return -1;
			String parentId = resolve(parentOf(path));
			if (parentId == null) return -1;
			try {
				Uri created = DocumentsContract.createDocument(resolver, docUri(parentId), "application/octet-stream", nameOf(path));
				if (created == null) return -1;
				id = DocumentsContract.getDocumentId(created);
				synchronized (this) { docIds.put(path, id); }
			}
			catch (Exception e) {
				Log.w(TAG, "create failed for " + path + ": " + e);
				return -1;
			}
			// the provider may have adjusted the name; our caller asked for this one
			SWMgr.FileInfo info = queryOne(docUri(id));
			if (info != null && !nameOf(path).equals(info.name))
				Log.w(TAG, "provider renamed " + nameOf(path) + " to " + info.name);
		}
		try {
			ParcelFileDescriptor pfd = resolver.openFileDescriptor(docUri(id), mode);
			if (pfd == null) return -1;
			return pfd.detachFd();
		}
		catch (Exception e) {
			Log.w(TAG, "open failed for " + path + " (" + mode + "): " + e);
			return -1;
		}
	}

	@Override
	public SWMgr.FileInfo stat(String path) {
		String id = resolve(path);
		if (id == null) return null;
		SWMgr.FileInfo info = queryOne(docUri(id));
		if (info == null) forget(path);	// stale cache entry
		else if (path.length() == 0) info.name = "";
		return info;
	}

	@Override
	public SWMgr.FileInfo[] list(String path) {
		String id = resolve(path);
		if (id == null) return null;
		List<SWMgr.FileInfo> entries = listChildren(path, id);
		if (entries == null) return null;
		return entries.toArray(new SWMgr.FileInfo[entries.size()]);
	}

	@Override
	public boolean mkdir(String path) {
		if (resolve(path) != null) return false;
		String parentId = resolve(parentOf(path));
		if (parentId == null) return false;
		try {
			Uri created = DocumentsContract.createDocument(resolver, docUri(parentId), Document.MIME_TYPE_DIR, nameOf(path));
			if (created == null) return false;
			synchronized (this) { docIds.put(path, DocumentsContract.getDocumentId(created)); }
			return true;
		}
		catch (Exception e) {
			Log.w(TAG, "mkdir failed for " + path + ": " + e);
			return false;
		}
	}

	@Override
	public boolean remove(String path) {
		String id = resolve(path);
		if (id == null) return false;
		try {
			boolean ok = DocumentsContract.deleteDocument(resolver, docUri(id));
			if (ok) forget(path);
			return ok;
		}
		catch (Exception e) {
			Log.w(TAG, "remove failed for " + path + ": " + e);
			return false;
		}
	}
}
