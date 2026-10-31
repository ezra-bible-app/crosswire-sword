/******************************************************************************
 *
 *  SWKey.java -	a SWORD key as seen from Java
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

/**
 * A key: a module's own (SWModule.getKey()) or a copy of it (clone()).
 * SWORD's objects are stateful: a walk which must not move a module goes on
 * a clone of its key.  The SWModule key methods (setKeyText, getKeyChildren,
 * ...) are these methods on the module's key.
 */
public class SWKey {

	/** native SWKey* */
	private long handle;
	/** a clone: dispose() frees it; a module's key belongs to the module */
	private boolean owned;

	SWKey(long handle, boolean owned) { this.handle = handle; this.owned = owned; }

	/** a copy of this key (SWKey::clone) for the caller to dispose(); null on failure */
	public SWKey                clone() {
		long h = nativeClone();
		return (h != 0) ? new SWKey(h, true) : null;
	}
	/** free a clone; the object must not be used afterwards.  A module's key is left alone. */
	public void                 dispose() {
		if (owned && handle != 0) { nativeDispose(handle); handle = 0; }
	}

	// Special values handled for a VerseKey: [+-][book|chapter], as SWModule.setKeyText
	// returns the key's error for this positioning (0 for none), which error() also still answers
	public native char          setText(String keyText);
	public native String        getText();
	public native char          error();
	public native void          begin();
	public native void          next();
	public native void          previous();
	public native boolean       hasChildren();

	// child nodes for a genbook key; for a VerseKey instead:
	//  [0..] [SWModule.VERSEKEY_TESTAMENT, VERSEKEY_BOOK, VERSEKEY_CHAPTER, VERSEKEY_VERSE, VERSEKEY_CHAPTERMAX, ... ]
	public native String[]      getChildren();
	public native String        getParent();

	private native long         nativeClone();
	private native void         nativeDispose(long handle);
}
