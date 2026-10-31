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
import android.os.Bundle;
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
	private final java.io.File stagingDir;
	/** providers whose descriptors are real files we may hand to native code
	 *  directly; anything else (Drive, Nextcloud, ...) returns pipes whose
	 *  content only commits when the ParcelFileDescriptor is closed from
	 *  Java, so writes to those are staged in a local file and uploaded on
	 *  close() */
	private final boolean direct;

	/** a write in progress against a non-local provider */
	private static class Staged {
		java.io.File tmp;
		Uri doc;
		String mode;		// provider mode for the upload: "w" or "wa"
	}
	private final Map<Integer, Staged> staged = new HashMap<Integer, Staged>();
	// relative path -> document id, learned from listings and creations
	private final Map<String, String> docIds = new HashMap<String, String>();
	// how long a first listing of a cloud folder may keep loading before we take what there is
	private static final long LIST_WAIT_MILLIS = 15000, LIST_POLL_MILLIS = 250;

	public SafStorage(Context context, Uri treeUri) {
		this.stagingDir = new java.io.File(context.getCacheDir(), "saf-staging");
		this.direct = "com.android.externalstorage.documents".equals(treeUri.getAuthority());
		this.resolver = context.getContentResolver();
		this.treeUri = treeUri;
		this.treeDocId = DocumentsContract.getTreeDocumentId(treeUri);
		docIds.put("", treeDocId);
	}

	public Uri getTreeUri() { return treeUri; }

	/** the picked folder's own name as the provider reports it, or null */
	public String rootName() {
		SWMgr.FileInfo info = queryOne(docUri(treeDocId));
		return (info == null || info.name == null || info.name.length() == 0) ? null : info.name;
	}

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
			// a cloud provider (Google Drive) answers the first query on a folder
			// it has not fetched yet with a partial, often empty, cursor flagged
			// EXTRA_LOADING: taking that as the folder's contents made Bishop
			// think a bookmarks file was absent and create a second one.  Ask
			// again until the provider says it is done, within reason.
			long deadline = System.currentTimeMillis() + LIST_WAIT_MILLIS;
			while (true) {
				c = resolver.query(childrenUri, COLUMNS, null, null, null);
				if (c == null) break;
				Bundle extras = c.getExtras();
				String error = (extras == null) ? null : extras.getString(DocumentsContract.EXTRA_ERROR);
				if (error != null) {
					Log.w(TAG, "list failed for " + dirPath + ": provider says " + error);
					return null;
				}
				boolean loading = extras != null && extras.getBoolean(DocumentsContract.EXTRA_LOADING, false);
				if (!loading) break;
				if (System.currentTimeMillis() > deadline) {
					Log.w(TAG, "list of " + dirPath + " still loading after " + LIST_WAIT_MILLIS + " ms: not trusting it");
					return null;
				}
				c.close(); c = null;
				try { Thread.sleep(LIST_POLL_MILLIS); } catch (InterruptedException e) { break; }
			}
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
		int fd = openResolved(path, mode, create);
		if (fd >= 0) return fd;
		// the id we had may be stale: another device can replace a file
		// under the same name (a cloud provider gives it a new id); forget
		// what we knew and look the name up once more
		String known;
		synchronized (this) { known = docIds.get(path); }
		if (known == null) return fd;
		forget(path);
		// ask the provider to refresh what it knows about the file and its
		// folder (a cloud provider's local record can lag a change made on
		// the web or on another device) before the name is looked up again
		try {
			resolver.refresh(docUri(known), null, null);
			String parentId = resolve(parentOf(path));
			if (parentId != null) resolver.refresh(DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, parentId), null, null);
		}
		catch (Exception e) {
			Log.i(TAG, "refresh not honoured for " + path + ": " + e);
		}
		forget(path);
		Log.i(TAG, "retrying " + path + " by name after a failed open");
		return openResolved(path, mode, create);
	}

	private int openResolved(String path, String mode, boolean create) {
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
		boolean writing = !mode.equals("r");
		if (writing && !direct) return openStaged(path, id, mode);
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

	/** give native code a local temp file to write; close() uploads it */
	private int openStaged(String path, String id, String mode) {
		try {
			if (!stagingDir.isDirectory() && !stagingDir.mkdirs()) return -1;
			java.io.File tmp = java.io.File.createTempFile("saf", ".tmp", stagingDir);
			boolean truncate = mode.contains("t");
			boolean append = mode.equals("wa");
			if (!truncate) {
				// start from the current content (an existing file opened read/write or for append)
				try {
					java.io.InputStream in = resolver.openInputStream(docUri(id));
					if (in != null) {
						java.io.FileOutputStream out = new java.io.FileOutputStream(tmp);
						byte[] buf = new byte[65536]; int n;
						while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
						out.close(); in.close();
					}
				}
				catch (Exception e) { Log.w(TAG, "could not fetch current content of " + path + ": " + e); }
			}
			int flags = ParcelFileDescriptor.MODE_READ_WRITE | ParcelFileDescriptor.MODE_CREATE;
			if (append) flags |= ParcelFileDescriptor.MODE_APPEND;
			ParcelFileDescriptor pfd = ParcelFileDescriptor.open(tmp, flags);
			int fd = pfd.detachFd();
			Staged st = new Staged();
			st.tmp = tmp; st.doc = docUri(id); st.mode = "wt";	// the whole staged file replaces the document
			synchronized (staged) { staged.put(fd, st); }
			return fd;
		}
		catch (Exception e) {
			Log.w(TAG, "staged open failed for " + path + " (" + mode + "): " + e);
			return -1;
		}
	}

	@Override
	public boolean close(int fd) {
		Staged st;
		synchronized (staged) { st = staged.remove(fd); }
		if (st == null) return false;
		boolean ok = false;
		try {
			ParcelFileDescriptor.adoptFd(fd).close();
			java.io.OutputStream out;
			try { out = resolver.openOutputStream(st.doc, st.mode); }
			catch (Exception e) {
				// not every provider knows "wt"; "w" then, and hope it truncates
				Log.i(TAG, st.mode + " refused by provider, trying w: " + e);
				out = resolver.openOutputStream(st.doc, "w");
			}
			if (out != null) {
				java.io.FileInputStream in = new java.io.FileInputStream(st.tmp);
				byte[] buf = new byte[65536]; int n;
				while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
				in.close();
				out.flush();
				out.close();		// this is what commits the upload on pipe-based providers
				ok = true;
			}
		}
		catch (Exception e) {
			Log.w(TAG, "upload of staged write to " + st.doc + " failed: " + e);
		}
		st.tmp.delete();
		return true;	// we owned the descriptor either way
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

	/** set when the last list() could not be trusted (the provider failed or
	 *  never finished loading): native code sees only an empty listing, and
	 *  the app must not take that for an empty folder */
	public volatile boolean lastListFailed = false;

	@Override
	public SWMgr.FileInfo[] list(String path) {
		lastListFailed = false;
		String id = resolve(path);
		if (id == null) return null;
		List<SWMgr.FileInfo> entries = listChildren(path, id);
		if (entries == null) { lastListFailed = true; return null; }
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
