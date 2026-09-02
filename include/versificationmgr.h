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
class SWLocale;


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
		friend class VersificationMgr;
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

		/** the named collection this book belongs to, if any (see getCollectionName) */
		SWBuf collectionName;

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

		/** The name of the collection this book was defined in, or ""
		 * for an ordinary Old or New Testament book.  A v11n XML
		 * definition may group books under a named collection, e.g.
		 * <collection name="Appendix"> for works a codex carries after
		 * the canon; such books are stored in a testament like any
		 * other (see registerVersificationSystemFromXML), so keys and
		 * modules never notice, but a frontend listing books may break
		 * them out under this heading */
		const char *getCollectionName() const { return collectionName.c_str(); }
	};

	class SWDLLEXPORT System {
	private:
		// registration (VersificationMgr's register methods) builds a
		// System through its Private half; a registered System is
		// read-only to everyone else
		friend class VersificationMgr;
		class Private;
		Private *p;
		SWBuf name;
		int BMAX[2];
		long ntStartOffset;
		void init();
		void loadFromSBook(const sbook *ot, const sbook *nt, int *chMax);
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
		long getOffsetFromVerse(int book, int chapter, int verse) const;
		char getVerseFromOffset(long offset, int *book, int *chapter, int *verse) const;
		const int *getBMAX() const { return BMAX; };
		long getNTStartOffset() const { return ntStartOffset; }
		void translateVerse(const System *dstSys, const char **book, int *chapter, int *verse, int *verse_end) const;

		/** What this system represents: "Bible" (the default).
		 * Selects the builtin abbreviation table that seeds
		 * getBookAbbrevs -- canon_abbrevs.h for Bible; nothing, today, for
		 * anything else.  A v11n XML definition sets it with the refSys
		 * attribute on its v11n element */
		const char *getRefSys() const;

		/** The book abbreviation table for parsing references in this
		 * system under a locale; precedence, low to high:
		 * the builtin table for the refSys, then
		 * the system's own additions, then
		 * the locale's [Book Abbrevs],
		 * each layer overriding the one before; sorted, and cached per
		 * locale.  Same table shape as canon_abbrevs.h: first match of the
		 * entire key wins, so "JO=JOHN" beats "JOB" */
		const struct abbrev *getBookAbbrevs(SWLocale *locale, int *retSize) const;

		/** The system's own additions to the refSys's builtin table --
		 * NOT the full abbreviation table (getBookAbbrevs is): entries
		 * the builtin table lacks (books it doesn't know, or an
		 * abbreviation meaning something else here).  A system loaded
		 * from XML gets each book's name, osisID and preferred
		 * abbreviation, plus its <abbr> elements, minus anything the
		 * builtin table already says.  Stored uppercased.
		 * i: 0 .. getAddedBookAbbrevCount()-1, insertion order; the
		 * returned pointers stay valid for the life of the System */
		int getAddedBookAbbrevCount() const;
		void getAddedBookAbbrev(int i, const char **abbrev, const char **osisID) const;

		/** One verse mapping rule: a verse (or verse range, verse..verseMax)
		 * of this system and the verse or range of the reference
		 * (intermediate) system it corresponds to, as canon_*.h data is
		 * written.  Books are osisIDs; a refBook this system lacks is a
		 * mapping extra book (see getMappingExtraBook); refBook NULL =
		 * same book as ours.  The bookNum fields are filled in by the
		 * engine at registration (leave 0 in hand-written data): 1-based
		 * book numbers, numbers past getBookCount() being mapping extra
		 * books; refBookNum 0 = same book as ours.
		 * See doc/bible-intermediate-v11n.md */
		struct MappingRule {
			const char *ourBook; int ourChapter, ourVerse, ourVerseMax;	// verseMax 0 = single verse
			const char *refBook; int refChapter, refVerse, refVerseMax;
			int ourBookNum, refBookNum;
		};
		int getMappingRuleCount() const;
		/** NULL if i is out of range; the rule and its book pointers stay
		 * valid for the life of the System */
		const MappingRule *getMappingRule(int i) const;
		int getMappingExtraBookCount() const;
		const char *getMappingExtraBook(int i) const;
	};

	VersificationMgr() { init(); }
	~VersificationMgr();
	static VersificationMgr *getSystemVersificationMgr();
	static void setSystemVersificationMgr(VersificationMgr *newVersificationMgr);
	const StringList getVersificationSystems() const;
	const System *getVersificationSystem(const char *name) const;
	/** rules, when supplied, are this system's av11n mappings; the array
	 * ends with a rule whose ourBook is NULL */
	void registerVersificationSystem(const char *name, const sbook *ot, const sbook *nt, int *chMax, const System::MappingRule *rules = NULL);

	/** Registers a versification system from an XML definition -- the
	 * format the VMR serves (v11n/collection/book/chapter, abbreviations,
	 * mappings).  overrideName, when supplied, names the system instead of
	 * the XML's v11nID (e.g., a module's own name).  A name already
	 * registered is never replaced: the first registration wins, so no
	 * definition can hijack a built-in or another module's system, and
	 * keys pointing at a registered system stay valid for the life of the
	 * process.
	 *
	 * Collections.  A collection's collectionID (1 or 2) is the testament
	 * its books are stored in and defaults to 2; a collection may carry a
	 * name, which its books report through Book::getCollectionName.
	 * Collections with the same collectionID concatenate in document
	 * order, so an appendix after the canon is written as a second,
	 * named collection:
	 *	<collection collectionID="2"> ...Matt..Rev... </collection>
	 *	<collection name="Appendix"> ...Barn, Herm... </collection>
	 * (and a KJVA-style apocrypha after the OT as
	 *	<collection collectionID="1" name="Apocrypha">).
	 *
	 * Deltas.  <v11n base="LXXNU"> derives the system from an already
	 * registered one (built-ins always are; a base that is another
	 * module's system depends on load order, and a base not registered
	 * yet is refused): the body then describes only the differences.  A book the base lacks is
	 * inserted -- after or before a base book named by an after= or
	 * before= attribute, else at the end of its testament.  A book the
	 * base has is patched: only the chapters listed (numbered by their
	 * osisID, Ps.151) change, a chapter past the base's last extends
	 * the book, and a name, preferredAbbrev or collection given replaces
	 * the base's.  refSys, abbreviations and mappings are inherited and
	 * added to.
	 *
	 * registeredName, when supplied, receives the effective system name.
	 *
	 * @return true if the system is registered (now or previously);
	 *	false if the definition could not be read or parsed, or its
	 *	base is not registered */
	bool registerVersificationSystemFromXML(const char *path, const char *overrideName = 0, SWBuf *registeredName = 0);
	void registerVersificationSystem(const char *name, const TreeKey *);
};

SWDLLEXPORT extern const struct abbrev builtin_abbrevs[];

SWORD_NAMESPACE_END
#endif
