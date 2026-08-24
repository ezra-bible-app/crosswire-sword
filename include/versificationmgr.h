/******************************************************************************
 *
 * versificationmgr.h -	class VersificationMgr: used for managing
 *			versification systems
 *
 * $Id$
 *
 * Copyright 2008-2013 CrossWire Bible Society (http://www.crosswire.org)
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

#include <list>
#include <defs.h>
#include <swcacher.h>
#include <swbuf.h>


#ifndef VERSIFICATIONMGR_H
#define VERSIFICATIONMGR_H


SWORD_NAMESPACE_START

typedef std::list <SWBuf>StringList;

struct sbook;
class TreeKey;


struct abbrev {
	const char *ab;
	const char *osis;
};

struct sbook {
	/**Name of book
	*/
	const char *name;

	/**OSIS name
	*/
	const char *osis;

	/**Preferred Abbreviation
	*/
	const char *prefAbbrev;

	/**Maximum chapters in book
	*/
	unsigned char chapmax;
	/** Array[chapmax] of maximum verses in chapters
	*/
	int *versemax;
};


class SWDLLEXPORT VersificationMgr : public SWCacher {

private:
	friend class __staticsystemVersificationMgr;

	class Private;
	Private *p;

	void init();

protected:
	static VersificationMgr *systemVersificationMgr;

public:
	class System;
	class SWDLLEXPORT Book {
	private:
		friend class System;
		friend struct BookOffsetLess;
		class Private;
		Private *p;

		/** book name */
		SWBuf longName;

		/** OSIS Abbreviation */
		SWBuf osisName;

		/** Preferred Abbreviation */
		SWBuf prefAbbrev;

		/** Maximum chapters in book */
		unsigned int chapMax;

		void init();

	public:
		Book() { init(); }
		Book(const Book &other);
		Book &operator =(const Book &other);
		Book(const char *longName, const char *osisName, const char *prefAbbrev, int chapMax) {
			this->longName = longName;
			this->osisName = osisName;
			this->prefAbbrev = prefAbbrev;
			this->chapMax = chapMax;
			init();
		}
		~Book();
		const char *getLongName() const { return longName.c_str(); }
		const char *getOSISName() const { return osisName.c_str(); }
		const char *getPreferredAbbreviation() const { return prefAbbrev.c_str(); }
		int getChapterMax() const { return chapMax; }
		int getVerseMax(int chapter) const;
	};

	class SWDLLEXPORT System {
	private:
		class Private;
		Private *p;
		SWBuf name;
		int BMAX[2];
		long ntStartOffset;
		void init();
	public:
		System() { this->name = ""; init(); }
		System(const System &other);
		System &operator =(const System &other);
		System(const char *name) { this->name = name; init(); }
		~System();
		const char *getName() const { return name.c_str(); }
		const Book *getBookByName(const char *bookName) const;
		int getBookNumberByOSISName(const char *bookName) const;
		const Book *getBook(int number) const;
		int getBookCount() const;
		void loadFromSBook(const sbook *ot, const sbook *nt, int *chMax, const unsigned char *mappings=NULL);
		long getOffsetFromVerse(int book, int chapter, int verse) const;
		char getVerseFromOffset(long offset, int *book, int *chapter, int *verse) const;
		const int *getBMAX() const { return BMAX; };
		long getNTStartOffset() const { return ntStartOffset; }
		void translateVerse(const System *dstSys, const char **book, int *chapter, int *verse, int *verse_end) const;

		/** a system loaded from XML may carry its own book abbreviations,
		 * which then serve instead of the locale's table (see
		 * VerseKey::getBookFromAbbrev) */
		bool hasOwnAbbrevs() const;
		void addOwnAbbrev(const char *abbrev, const char *osis);
		/** upperAbbrev is already uppercased; returns book number or -1 */
		int getBookNumberByAbbrev(const char *upperAbbrev) const;
	};

	VersificationMgr() { init(); }
	~VersificationMgr();
	static VersificationMgr *getSystemVersificationMgr();
	static void setSystemVersificationMgr(VersificationMgr *newVersificationMgr);
	const StringList getVersificationSystems() const;
	const System *getVersificationSystem(const char *name) const;
	void registerVersificationSystem(const char *name, const sbook *ot, const sbook *nt, int *chMax, const unsigned char *mappings=NULL);

	/** Registers a versification system from an XML definition -- the
	 * format the VMR serves (v11n/collection/book/chapter, abbreviations,
	 * mappings).  overrideName, when supplied, names the system instead of
	 * the XML's v11nID (e.g., a module's own name).  A name already
	 * registered is never replaced: the first registration wins, so no
	 * definition can hijack a built-in or another module's system, and
	 * keys pointing at a registered system stay valid for the life of the
	 * process.
	 *
	 * @return true if the system is registered (now or previously);
	 *	false if the definition could not be read or parsed */
	bool registerVersificationSystemFromXML(const char *path, const char *overrideName = 0);
	void registerVersificationSystem(const char *name, const TreeKey *);
};

SWDLLEXPORT extern const struct abbrev builtin_abbrevs[];

SWORD_NAMESPACE_END
#endif
