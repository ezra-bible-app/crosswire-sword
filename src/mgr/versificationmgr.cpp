/******************************************************************************
 *
 *  versificationmgr.cpp -	implementation of class VersificationMgr used
 *				for managing versification systems
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

#include <versificationmgr.h>
#include <vector>
#include <map>
#include <treekey.h>
#include <canon.h>		// KJV internal versification system
#include <swlog.h>
#include <filemgr.h>
#include <utilxml.h>
#include <stringmgr.h>
#include <swlocale.h>
#include <list>
#include <algorithm>
#include <ctype.h>

#include <canon_null.h>		// null v11n system

#include <canon_leningrad.h>	// Leningrad Codex (WLC) v11n system
#include <canon_mt.h>		// Masoretic Text (MT) v11n system
#include <canon_kjva.h>		// KJV + Apocrypha v11n system
#include <canon_nrsv.h>		// NRSV v11n system
#include <canon_nrsva.h>	// NRSV + Apocrypha v11n system
#include <canon_synodal.h>	// Russian Synodal v11n system
#include <canon_synodalprot.h>	// Russian Synodal v11n system
#include <canon_vulg.h>		// Vulgate v11n system
#include <canon_german.h>	// German v11n system
#include <canon_luther.h>	// Luther v11n system
#include <canon_catholic.h>	// Catholic v11n system (10 chapter Esther)
#include <canon_catholic2.h>	// Catholic2 v11n system (16 chapter Esther)
#include <canon_lxx.h>		// General LXX v11n system (includes GNT, as used in Orthodox Bibles)
#include <canon_orthodox.h>	// Orthodox v11n system as used in Orthodox Bibles
#include <canon_segond.h>	// French v11n system as used by Segond Bibles and its derivatives
#include <canon_calvin.h>	// French v11n system 
#include <canon_darbyfr.h>	// French v11n system based on John Darby's French translation

using std::vector;
using std::map;
using std::distance;
using std::lower_bound;


SWORD_NAMESPACE_START


VersificationMgr *VersificationMgr::getSystemVersificationMgr() {
	if (!systemVersificationMgr) {
		systemVersificationMgr = new VersificationMgr();
		systemVersificationMgr->registerVersificationSystem("KJV", otbooks, ntbooks, vm);
		systemVersificationMgr->registerVersificationSystem("Leningrad", otbooks_leningrad, ntbooks_null, vm_leningrad);
		systemVersificationMgr->registerVersificationSystem("MT", otbooks_mt, ntbooks_null, vm_mt);
		systemVersificationMgr->registerVersificationSystem("KJVA", otbooks_kjva, ntbooks, vm_kjva);
		systemVersificationMgr->registerVersificationSystem("NRSV", otbooks, ntbooks, vm_nrsv, mappings_nrsv);
		systemVersificationMgr->registerVersificationSystem("NRSVA", otbooks_nrsva, ntbooks, vm_nrsva);
		systemVersificationMgr->registerVersificationSystem("Synodal", otbooks_synodal, ntbooks_synodal, vm_synodal, mappings_synodal);
		systemVersificationMgr->registerVersificationSystem("SynodalProt", otbooks_synodalProt, ntbooks_synodal, vm_synodalProt);
		systemVersificationMgr->registerVersificationSystem("Vulg", otbooks_vulg, ntbooks_vulg, vm_vulg, mappings_vulg);
		systemVersificationMgr->registerVersificationSystem("German", otbooks_german, ntbooks, vm_german);
		systemVersificationMgr->registerVersificationSystem("Luther", otbooks_luther, ntbooks_luther, vm_luther);
		systemVersificationMgr->registerVersificationSystem("Catholic", otbooks_catholic, ntbooks, vm_catholic);
		systemVersificationMgr->registerVersificationSystem("Catholic2", otbooks_catholic2, ntbooks, vm_catholic2);
		systemVersificationMgr->registerVersificationSystem("LXX", otbooks_lxx, ntbooks, vm_lxx);
		systemVersificationMgr->registerVersificationSystem("Orthodox", otbooks_orthodox, ntbooks, vm_orthodox);
		systemVersificationMgr->registerVersificationSystem("Calvin", otbooks, ntbooks, vm_calvin, mappings_calvin);
		systemVersificationMgr->registerVersificationSystem("DarbyFr", otbooks, ntbooks, vm_darbyfr, mappings_darbyfr);
		systemVersificationMgr->registerVersificationSystem("Segond", otbooks, ntbooks, vm_segond, mappings_segond);
	}
	return systemVersificationMgr;
}


class VersificationMgr::System::Private {
public:
	/** Array[chapmax] of maximum verses in chapters */
	vector<Book> books;
	map<SWBuf, int> osisLookup;
	/** General mapping rule is that first verse of every chapter corresponds first
		verse of another chapter in default intermediate canon(kjva), so mapping data
		contains expections. Intermediate canon could not contain corresponding data.
		
		Each element in @variable mappings contains of all rules that are related to
		particular book.

		@typedef mappingRule is a pointer on uchar[7]: 1 value - book id 1-based, ot+nt,
		2-4 map to, 5-7 map from (chap,verse from, verse to if greater then "verse from").
		Size of rule would be 8 if there is inter-book mapping and target book is abscent
		in reference system, in this case @variable mappingsExtraBooks data is used and
		id is rule book id minus book count.
	*/
	typedef VersificationMgr::System::MappingRule MappingRule;
	/** all rules, in the order they were added (translateVerse relies on
	 * this order).  addMappingRule interns the name fields into our own
	 * storage (what came in may have been transient), so stored rules are
	 * valid at all times */
	vector<MappingRule> mappingRules;
	/** for each book (index = 1-based book number - 1), the indices into
	 * mappingRules of the rules that involve it.  An inter-book rule is
	 * listed under both of its books.  Sized books.size() + mappingsExtraBooks.size() */
	vector< vector<int> > rulesByBook;
	/** reference-system books this system lacks but its rules refer to;
	 * numbered after our own books: books.size()+1, ...  A list, so growth
	 * never moves the names that stored rules point into (books are frozen
	 * before the first rule; these are the only other referent) */
	std::list<SWBuf> mappingsExtraBooks;
	const char *bookOSISName(int bookNumber) const {	// our book or extra book
		if (bookNumber >= 1 && bookNumber <= (int)books.size()) return books[bookNumber-1].getOSISName();
		int extra = bookNumber - (int)books.size() - 1;
		if (extra < 0 || extra >= (int)mappingsExtraBooks.size()) return "";
		std::list<SWBuf>::const_iterator it = mappingsExtraBooks.begin();
		while (extra--) ++it;
		return it->c_str();
	}

	SWBuf refSys;	// "Bible" unless the definition says otherwise
	/** the builtin abbreviation table that seeds this system's merged
	 * table (and that additions are deduped against): canon_abbrevs.h
	 * for Bible; an empty table for any other corpus, until one exists */
	const struct abbrev *seedAbbrevs() const {
		static const struct abbrev none[] = { { "", "" } };
		return (refSys == "Bible") ? builtin_abbrevs : none;
	}
	/** the system's own additions to the seed table: UPPERCASED abbreviation -> osisID */
	struct BookAbbrev {
		SWBuf abbrev, osis;
		BookAbbrev(const SWBuf &abbrev, const SWBuf &osis) : abbrev(abbrev), osis(osis) {}
	};
	vector<BookAbbrev> bookAbbrevs;
	/** builtin + additions + one locale's entries, merged and sorted */
	struct MergedAbbrevs {
		map<SWBuf, SWBuf> entries;
		vector<struct abbrev> table;	// points into entries
	};
	/** one merged table per locale name, built on demand.  A copy of the
	 * System starts with an empty cache (the tables point into their own
	 * maps, so they can't be copied cheaply) */
	struct MergedAbbrevsCache {
		map<SWBuf, MergedAbbrevs *> byLocale;
		MergedAbbrevsCache() {}
		MergedAbbrevsCache(const MergedAbbrevsCache &) {}
		MergedAbbrevsCache &operator=(const MergedAbbrevsCache &) { clear(); return *this; }
		~MergedAbbrevsCache() { clear(); }
		void clear() {
			for (map<SWBuf, MergedAbbrevs *>::iterator it = byLocale.begin(); it != byLocale.end(); ++it) delete it->second;
			byLocale.clear();
		}
	} mergedAbbrevs;

	void ensureRulesByBookSize() {
		size_t n = books.size() + mappingsExtraBooks.size();
		if (rulesByBook.size() < n) rulesByBook.resize(n);
	}

	// -- construction, driven by VersificationMgr's registration paths --

	int bookNumberByOSISName(const char *bookName) const {	// 1-based, or -1
		map<SWBuf, int>::const_iterator it = osisLookup.find(bookName);
		return (it != osisLookup.end()) ? it->second : -1;
	}

	/** the osisID of a book of this system or of a mapping extra book;
	 * returns the 1-based book number, or 0 if unknown */
	int resolveMappingBook(const char *book) const {
		if (!book || !*book) return 0;
		int n = bookNumberByOSISName(book);
		if (n > 0) return n;
		int i = 0;
		for (std::list<SWBuf>::const_iterator it = mappingsExtraBooks.begin(); it != mappingsExtraBooks.end(); ++it, ++i) {
			if (*it == book) return (int)books.size() + i + 1;
		}
		return 0;
	}

	void addMappingExtraBook(const char *osisName) {
		mappingsExtraBooks.push_back(SWBuf(osisName));
		ensureRulesByBookSize();
	}

	/** a refBook not in this system is added as an extra book
	 * automatically; returns false if ourBook is unknown */
	bool addMappingRule(MappingRule rule) {
		rule.ourBookNum = resolveMappingBook(rule.ourBook);
		if (rule.ourBookNum < 1 || rule.ourBookNum > (int)books.size()) return false;
		rule.refBookNum = 0;
		if (rule.refBook && *rule.refBook) {
			rule.refBookNum = resolveMappingBook(rule.refBook);
			if (!rule.refBookNum) {
				addMappingExtraBook(rule.refBook);
				rule.refBookNum = resolveMappingBook(rule.refBook);
			}
		}
		// intern the names into our own storage: books are frozen before the
		// first rule, and extra books live in a list, so these never move
		rule.ourBook = bookOSISName(rule.ourBookNum);
		rule.refBook = rule.refBookNum ? bookOSISName(rule.refBookNum) : 0;
		ensureRulesByBookSize();
		mappingRules.push_back(rule);
		int idx = (int)mappingRules.size() - 1;
		rulesByBook[rule.ourBookNum-1].push_back(idx);
		// an inter-book rule is also listed under its reference book
		if (rule.refBookNum && rule.refBookNum != rule.ourBookNum)
			rulesByBook[rule.refBookNum-1].push_back(idx);
		return true;
	}

	/** anything the refSys's builtin table already says is dropped -- an
	 * optimization; the merged result is the same.  Stored uppercased */
	void addBookAbbrev(const char *abbrev, const char *osisID) {
		if (!abbrev || !*abbrev || !osisID || !*osisID) return;
		SWBuf upper = abbrev;
		upper.toUpper();
		const struct abbrev *seed = seedAbbrevs();
		for (int j = 0; seed[j].osis[0]; ++j) {
			if (upper == seed[j].ab && !strcmp(osisID, seed[j].osis)) return;
		}
		for (unsigned int i = 0; i < bookAbbrevs.size(); ++i) {
			if (bookAbbrevs[i].abbrev == upper && bookAbbrevs[i].osis == osisID) return;
		}
		bookAbbrevs.push_back(BookAbbrev(upper, SWBuf(osisID)));
		mergedAbbrevs.clear();	// stale now
	}

	void setRefSys(const char *newRefSys) {
		refSys = (newRefSys && *newRefSys) ? newRefSys : "Bible";
		mergedAbbrevs.clear();	// seeded from a different table now
	}

	Private() : refSys("Bible") {
	}
	Private(const VersificationMgr::System::Private &other) {
		books = other.books;
		osisLookup = other.osisLookup;
		refSys = other.refSys;
	}
	VersificationMgr::System::Private &operator =(const VersificationMgr::System::Private &other) {
		books = other.books;
		osisLookup = other.osisLookup;
		refSys = other.refSys;
		return *this;
	}
};


class VersificationMgr::Book::Private {
friend struct BookOffsetLess;
public:
	/** Array[chapmax] of maximum verses in chapters */
	vector<int> verseMax;
	vector<long> offsetPrecomputed;

	Private() {
		verseMax.clear();
	}
	Private(const VersificationMgr::Book::Private &other) {
		verseMax.clear();
		verseMax = other.verseMax;
		offsetPrecomputed = other.offsetPrecomputed;
	}
	VersificationMgr::Book::Private &operator =(const VersificationMgr::Book::Private &other) {
		verseMax.clear();
                int s = (int)other.verseMax.size();
                if (s) verseMax = other.verseMax;
		offsetPrecomputed = other.offsetPrecomputed;
		return *this;
	}
};


struct BookOffsetLess {
	bool operator() (const VersificationMgr::Book &o1, const VersificationMgr::Book &o2) const { return o1.p->offsetPrecomputed[0] < o2.p->offsetPrecomputed[0]; }
	bool operator() (const long &o1, const VersificationMgr::Book &o2) const { return o1 < o2.p->offsetPrecomputed[0]; }
	bool operator() (const VersificationMgr::Book &o1, const long &o2) const { return o1.p->offsetPrecomputed[0] < o2; }
	bool operator() (const long &o1, const long &o2) const { return o1 < o2; }
};


void VersificationMgr::Book::init() {
	p = new Private();
}


void VersificationMgr::System::init() {
	p = new Private();
	BMAX[0] = 0;
	BMAX[1] = 0;
	ntStartOffset = 0;
}


VersificationMgr::System::System(const System &other) {
	init();
	name = other.name;
	BMAX[0] = other.BMAX[0];
	BMAX[1] = other.BMAX[1];
	(*p) = *(other.p);
	ntStartOffset = other.ntStartOffset;
}


VersificationMgr::System &VersificationMgr::System::operator =(const System &other) {
	name = other.name;
	BMAX[0] = other.BMAX[0];
	BMAX[1] = other.BMAX[1];
	(*p) = *(other.p);
	ntStartOffset = other.ntStartOffset;
	return *this;
}


VersificationMgr::System::~System() {
	delete p;
}


const VersificationMgr::Book *VersificationMgr::System::getBook(int number) const {
	return (number < (signed int)p->books.size()) ? &(p->books[number]) : 0;
}


int VersificationMgr::System::getBookNumberByOSISName(const char *bookName) const {
	map<SWBuf, int>::const_iterator it = p->osisLookup.find(bookName);
	return (it != p->osisLookup.end()) ? it->second : -1;
}


void VersificationMgr::System::loadFromSBook(const sbook *ot, const sbook *nt, int *chMax) {
	int chap = 0;
	int book = 0;
	long offset = 0;	// module heading
	offset++;			// testament heading
	while (ot->chapmax) {
		p->books.push_back(Book(ot->name, ot->osis, ot->prefAbbrev, ot->chapmax));
		offset++;		// book heading
		Book &b = p->books[p->books.size()-1];
		p->osisLookup[b.getOSISName()] = (int)p->books.size();
		for (int i = 0; i < ot->chapmax; i++) {
			b.p->verseMax.push_back(chMax[chap]);
			offset++;		// chapter heading
			b.p->offsetPrecomputed.push_back(offset);
			offset += chMax[chap++];
		}
		ot++;
		book++;
	}
	BMAX[0] = book;

	book = 0;
	ntStartOffset = offset;
	offset++;			// testament heading
	while (nt->chapmax) {
		p->books.push_back(Book(nt->name, nt->osis, nt->prefAbbrev, nt->chapmax));
		offset++;		// book heading
		Book &b = p->books[p->books.size()-1];
		p->osisLookup[b.getOSISName()] = (int)p->books.size();
		for (int i = 0; i < nt->chapmax; i++) {
			b.p->verseMax.push_back(chMax[chap]);
			offset++;		// chapter heading
			b.p->offsetPrecomputed.push_back(offset);
			offset += chMax[chap++];
		}
		nt++;
		book++;
	}

	BMAX[1] = book;

	// TODO: build offset speed array
}


const char *VersificationMgr::System::getRefSys() const {
	return p->refSys.c_str();
}


const VersificationMgr::System::MappingRule *VersificationMgr::System::getMappingRule(int i) const {
	return (i >= 0 && i < (int)p->mappingRules.size()) ? &p->mappingRules[i] : 0;
}


const struct abbrev *VersificationMgr::System::getBookAbbrevs(SWLocale *locale, int *retSize) const {
	int localeCount = 0;
	const struct abbrev *localeAbbrevs = locale ? locale->getLocaleBookAbbrevs(&localeCount) : 0;
	const struct abbrev *seed = p->seedAbbrevs();
	if (p->bookAbbrevs.empty() && !localeCount) {
		// nothing to merge: the seed table serves as is
		int n = 0;
		while (seed[n].osis[0]) ++n;
		*retSize = n;
		return seed;
	}
	SWBuf key = locale ? locale->getName() : "";
	Private::MergedAbbrevs *&merged = p->mergedAbbrevs.byLocale[key];
	if (!merged) {
		merged = new Private::MergedAbbrevs();
		for (int j = 0; seed[j].osis[0]; ++j) merged->entries[seed[j].ab] = seed[j].osis;
		for (unsigned int i = 0; i < p->bookAbbrevs.size(); ++i) merged->entries[p->bookAbbrevs[i].abbrev] = p->bookAbbrevs[i].osis;
		for (int i = 0; i < localeCount; ++i) merged->entries[localeAbbrevs[i].ab] = localeAbbrevs[i].osis;
		merged->table.reserve(merged->entries.size() + 1);
		for (map<SWBuf, SWBuf>::const_iterator it = merged->entries.begin(); it != merged->entries.end(); ++it) {
			struct abbrev a = { it->first.c_str(), it->second.c_str() };
			merged->table.push_back(a);
		}
		struct abbrev end = { "", "" };
		merged->table.push_back(end);
	}
	*retSize = (int)merged->table.size() - 1;
	return &merged->table[0];
}


int VersificationMgr::System::getAddedBookAbbrevCount() const {
	return (int)p->bookAbbrevs.size();
}


void VersificationMgr::System::getAddedBookAbbrev(int i, const char **abbrev, const char **osisID) const {
	*abbrev = 0; *osisID = 0;
	if (i < 0 || i >= (int)p->bookAbbrevs.size()) return;
	*abbrev = p->bookAbbrevs[i].abbrev.c_str();
	*osisID = p->bookAbbrevs[i].osis.c_str();
}


int VersificationMgr::System::getMappingRuleCount() const {
	return (int)p->mappingRules.size();
}


int VersificationMgr::System::getMappingExtraBookCount() const { return (int)p->mappingsExtraBooks.size(); }


const char *VersificationMgr::System::getMappingExtraBook(int i) const {
	if (i < 0 || i >= (int)p->mappingsExtraBooks.size()) return 0;
	return p->bookOSISName((int)p->books.size() + i + 1);
}


bool VersificationMgr::registerVersificationSystemFromXML(const char *path, const char *overrideName, SWBuf *registeredName) {

	// pull the whole definition through the FileMgr, so pluggable FileOps
	// (e.g., Android shared storage) serve it like any module data
	FileDesc *fd = FileMgr::getSystemFileMgr()->open(path, FileMgr::RDONLY);
	if (!fd || fd->getFd() < 0) {
		if (fd) FileMgr::getSystemFileMgr()->close(fd);
		SWLog::getSystemLog()->logWarning("registerVersificationSystemFromXML: cannot open %s", path);
		return false;
	}
	SWBuf xml;
	char chunk[4096];
	long bytes;
	while ((bytes = fd->read(chunk, 4096)) > 0) xml.append(chunk, bytes);
	FileMgr::getSystemFileMgr()->close(fd);

	// stream the tags
	SWBuf v11nID, refSys;
	struct XBook { SWBuf name, osis, prefAbbrev; int chapMax; };
	vector<XBook> otBooks, ntBooks;
	vector<int> vm;
	vector< std::pair<SWBuf, SWBuf> > abbrevs;	// (abbrev text, osis)
	vector<SWBuf> extraBooks;
	// <map> with the MappingRule attribute names (ourBook, ourChapter, ...,
	// refBook, ...), books as osisIDs; resolved once the book list is complete
	struct XMap { SWBuf ourBook, refBook; int ourChapter, ourVerse, ourVerseMax, refChapter, refVerse, refVerseMax; };
	vector<XMap> maps;

	int collection = 0;		// 1 = ot, 2 = nt
	bool inAbbrevs = false, inMappings = false, inExtraBooks = false;
	SWBuf abbrOsis;			// pending <abbr osisID=...> text

	const char *sx = xml.c_str();
	while (*sx) {
		if (*sx != '<') { sx++; continue; }
		const char *e = strchr(sx, '>');
		if (!e) break;
		SWBuf tagText;
		tagText.append(sx+1, (unsigned long)(e-sx-1));
		sx = e+1;
		bool isEnd = tagText.startsWith("/");
		XMLTag tag(tagText.c_str());
		SWBuf tagName = isEnd ? SWBuf(tagText.c_str()+1) : SWBuf(tag.getName());
		tagName.trim();

		if (!isEnd && tagName == "v11n") { v11nID = tag.getAttribute("v11nID"); refSys = tag.getAttribute("refSys"); }
		else if (tagName == "collection") { collection = isEnd ? 0 : atoi(tag.getAttribute("collectionID")); }
		else if (tagName == "abbreviations") { inAbbrevs = !isEnd; }
		else if (tagName == "mappings") { inMappings = !isEnd; }
		else if (tagName == "extraBooks") { inExtraBooks = !isEnd; }
		else if (!isEnd && tagName == "book") {
			if (inMappings && inExtraBooks) { extraBooks.push_back(tag.getAttribute("name")); }
			else if (collection == 1 || collection == 2) {
				XBook b;
				b.name = tag.getAttribute("name");
				b.osis = tag.getAttribute("osisID");
				b.prefAbbrev = tag.getAttribute("preferredAbbrev");
				b.chapMax = 0;
				(collection == 1 ? otBooks : ntBooks).push_back(b);
			}
		}
		else if (!isEnd && tagName == "chapter" && (collection == 1 || collection == 2)) {
			vector<XBook> &books = (collection == 1 ? otBooks : ntBooks);
			if (books.size()) {
				books[books.size()-1].chapMax++;
				vm.push_back(atoi(tag.getAttribute("verseMax")));
			}
		}
		else if (tagName == "abbr" && inAbbrevs) {
			if (!isEnd) {
				abbrOsis = tag.getAttribute("osisID");
				// the abbreviation is the element's text content
				const char *t = sx;
				const char *te = strchr(t, '<');
				if (te) {
					SWBuf ab; ab.append(t, (unsigned long)(te-t)); ab.trim();
					if (ab.length() && abbrOsis.length()) abbrevs.push_back(std::make_pair(ab, abbrOsis));
				}
			}
		}
		else if (!isEnd && tagName == "map" && inMappings) {
			XMap m;
			m.ourBook = tag.getAttribute("ourBook");
			m.refBook = tag.getAttribute("refBook");
			m.ourChapter = atoi(tag.getAttribute("ourChapter"));
			m.ourVerse = atoi(tag.getAttribute("ourVerse"));
			m.ourVerseMax = atoi(tag.getAttribute("ourVerseMax"));
			m.refChapter = atoi(tag.getAttribute("refChapter"));
			m.refVerse = atoi(tag.getAttribute("refVerse"));
			m.refVerseMax = atoi(tag.getAttribute("refVerseMax"));
			maps.push_back(m);
		}
	}

	SWBuf name = (overrideName && *overrideName) ? SWBuf(overrideName) : v11nID;
	if (!name.length() || (otBooks.empty() && ntBooks.empty())) {
		SWLog::getSystemLog()->logWarning("registerVersificationSystemFromXML: %s is not a versification definition", path);
		return false;
	}

	// the first registration wins: nothing replaces a built-in or an
	// already-registered system, and keys holding a registered system stay
	// valid for the life of the process
	if (registeredName) *registeredName = name;
	if (getVersificationSystem(name.c_str())) {
		SWLog::getSystemLog()->logInformation("registerVersificationSystemFromXML: %s already registered; keeping the existing system", name.c_str());
		return true;
	}

	// build the sbook arrays (Books copy the strings; these are temporary)
	vector<sbook> ot, nt;
	sbook sb, terminator;
	terminator.name = ""; terminator.osis = ""; terminator.prefAbbrev = ""; terminator.chapmax = 0; terminator.versemax = 0;
	for (unsigned int i = 0; i < otBooks.size(); i++) {
		sb.name = otBooks[i].name.c_str(); sb.osis = otBooks[i].osis.c_str();
		sb.prefAbbrev = otBooks[i].prefAbbrev.c_str(); sb.chapmax = otBooks[i].chapMax; sb.versemax = 0;
		ot.push_back(sb);
	}
	ot.push_back(terminator);
	for (unsigned int i = 0; i < ntBooks.size(); i++) {
		sb.name = ntBooks[i].name.c_str(); sb.osis = ntBooks[i].osis.c_str();
		sb.prefAbbrev = ntBooks[i].prefAbbrev.c_str(); sb.chapmax = ntBooks[i].chapMax; sb.versemax = 0;
		nt.push_back(sb);
	}
	nt.push_back(terminator);

	registerVersificationSystem(name.c_str(), &ot[0], &nt[0], vm.empty() ? 0 : &vm[0]);

	System *s = const_cast<System *>(getVersificationSystem(name.c_str()));
	if (s) {
		s->p->setRefSys(refSys.c_str());	// before the abbreviations: it picks their seed table
		for (unsigned int i = 0; i < extraBooks.size(); i++) {
			s->p->addMappingExtraBook(extraBooks[i].c_str());
		}
		for (unsigned int i = 0; i < maps.size(); i++) {
			const XMap &m = maps[i];
			System::MappingRule rule;
			rule.ourBookNum = rule.refBookNum = 0;
			rule.ourBook = m.ourBook.c_str();
			rule.refBook = m.refBook.length() ? m.refBook.c_str() : 0;
			rule.ourChapter = m.ourChapter; rule.ourVerse = m.ourVerse; rule.ourVerseMax = m.ourVerseMax;
			rule.refChapter = m.refChapter; rule.refVerse = m.refVerse; rule.refVerseMax = m.refVerseMax;
			if (!s->p->addMappingRule(rule))
				SWLog::getSystemLog()->logWarning("registerVersificationSystemFromXML: %s: unknown mapping book %s", name.c_str(), m.ourBook.c_str());
		}
		for (unsigned int i = 0; i < otBooks.size(); i++) {
			s->p->addBookAbbrev(otBooks[i].name.c_str(), otBooks[i].osis.c_str());
			s->p->addBookAbbrev(otBooks[i].osis.c_str(), otBooks[i].osis.c_str());
			s->p->addBookAbbrev(otBooks[i].prefAbbrev.c_str(), otBooks[i].osis.c_str());
		}
		for (unsigned int i = 0; i < ntBooks.size(); i++) {
			s->p->addBookAbbrev(ntBooks[i].name.c_str(), ntBooks[i].osis.c_str());
			s->p->addBookAbbrev(ntBooks[i].osis.c_str(), ntBooks[i].osis.c_str());
			s->p->addBookAbbrev(ntBooks[i].prefAbbrev.c_str(), ntBooks[i].osis.c_str());
		}
		for (unsigned int i = 0; i < abbrevs.size(); i++) {
			s->p->addBookAbbrev(abbrevs[i].first.c_str(), abbrevs[i].second.c_str());
		}
	}
	SWLog::getSystemLog()->logInformation("registerVersificationSystemFromXML: registered %s from %s", name.c_str(), path);
	return true;
}


VersificationMgr::Book::Book(const Book &other) {
	longName = other.longName;
	osisName = other.osisName;
	prefAbbrev = other.prefAbbrev;
	chapMax = other.chapMax;
	init();
	(*p) = *(other.p);
}


VersificationMgr::Book& VersificationMgr::Book::operator =(const Book &other) {
	longName = other.longName;
	osisName = other.osisName;
	prefAbbrev = other.prefAbbrev;
	chapMax = other.chapMax;
	init();
	(*p) = *(other.p);
	return *this;
}


VersificationMgr::Book::~Book() {
	delete p;
}


int VersificationMgr::Book::getVerseMax(int chapter) const {
	chapter--;
	return (p && (chapter < (signed int)p->verseMax.size()) && (chapter > -1)) ? p->verseMax[chapter] : -1;
}


int VersificationMgr::System::getBookCount() const {
	return (int)(p ? p->books.size() : 0);
}


long VersificationMgr::System::getOffsetFromVerse(int book, int chapter, int verse) const {
	long  offset = -1;
	chapter--;

	const Book *b = getBook(book);

	if (!b)                                        return -1;	// assert we have a valid book
	if ((chapter > -1) && (chapter >= (signed int)b->p->offsetPrecomputed.size())) return -1;	// assert we have a valid chapter

	offset = b->p->offsetPrecomputed[(chapter > -1)?chapter:0];
	if (chapter < 0) offset--;

/* old code
 *
	offset = offsets[testament-1][0][book];
	offset = offsets[testament-1][1][(int)offset + chapter];
	if (!(offset|verse)) // if we have a testament but nothing else.
		offset = 1;

*/

	return (offset + verse);
}


char VersificationMgr::System::getVerseFromOffset(long offset, int *book, int *chapter, int *verse) const {

	if (offset < 1) {	// just handle the module heading corner case up front (and error case)
		(*book) = -1;
		(*chapter) = 0;
		(*verse) = 0;
		return offset;	// < 0 = error
	}

	// binary search for book
	vector<Book>::iterator b = lower_bound(p->books.begin(), p->books.end(), offset, BookOffsetLess());
	if (b == p->books.end()) b--;
	(*book)    = distance(p->books.begin(), b)+1;
	if (offset < (*(b->p->offsetPrecomputed.begin()))-((((!(*book)) || (*book)==BMAX[0]+1))?2:1)) { // -1 for chapter headings
		(*book)--;
		if (b != p->books.begin()) {
			b--;	
		}
	}
	vector<long>::iterator c = lower_bound(b->p->offsetPrecomputed.begin(), b->p->offsetPrecomputed.end(), offset);

	// if we're a book heading, we are lessthan chapter precomputes, but greater book.  This catches corner case.
	if (c == b->p->offsetPrecomputed.end()) {
		c--;
	}
	if ((offset < *c) && (c == b->p->offsetPrecomputed.begin())) {
		(*chapter) = (offset - *c)+1;	// should be 0 or -1 (for testament heading)
		(*verse) = 0;
	}
	else {
		if (offset < *c) c--;
		(*chapter) = distance(b->p->offsetPrecomputed.begin(), c)+1;
		(*verse)   = (offset - *c);
	}
	return ((*chapter > 0) && (*verse > b->getVerseMax(*chapter))) ? KEYERR_OUTOFBOUNDS : 0;
}


/***************************************************
 * VersificationMgr
 */

class VersificationMgr::Private {
public:
	Private() {
	}
	Private(const VersificationMgr::Private &other) {
		systems = other.systems;
	}
	VersificationMgr::Private &operator =(const VersificationMgr::Private &other) {
		systems = other.systems;
		return *this;
	}
	map<SWBuf, System> systems;
};
// ---------------- statics -----------------
VersificationMgr *VersificationMgr::systemVersificationMgr = 0;

class __staticsystemVersificationMgr {
public:
	__staticsystemVersificationMgr() { }
	~__staticsystemVersificationMgr() { delete VersificationMgr::systemVersificationMgr; }
} _staticsystemVersificationMgr;


void VersificationMgr::init() {
	p = new Private();
}


VersificationMgr::~VersificationMgr() {
	delete p;
}


void VersificationMgr::setSystemVersificationMgr(VersificationMgr *newVersificationMgr) {
	if (systemVersificationMgr)
		delete systemVersificationMgr;
	systemVersificationMgr = newVersificationMgr;
}


const VersificationMgr::System *VersificationMgr::getVersificationSystem(const char *name) const {
	map<SWBuf, System>::const_iterator it = p->systems.find(name);
	return (it != p->systems.end()) ? &(it->second) : 0;
}


void VersificationMgr::registerVersificationSystem(const char *name, const sbook *ot, const sbook *nt, int *chMax, const System::MappingRule *rules) {
	p->systems[name] = System(name);
	System &s = p->systems[name];
	s.loadFromSBook(ot, nt, chMax);
	for (int i = 0; rules && rules[i].ourBook; ++i) {
		if (!s.p->addMappingRule(rules[i]))
			SWLog::getSystemLog()->logWarning("registerVersificationSystem: %s: unknown mapping book %s", name, rules[i].ourBook);
	}
}


void VersificationMgr::registerVersificationSystem(const char *name, const TreeKey *tk) {
}


const StringList VersificationMgr::getVersificationSystems() const {
	StringList retVal;
	for (map<SWBuf, System>::const_iterator it = p->systems.begin(); it != p->systems.end(); it++) {
		retVal.push_back(it->first);
	}
	return retVal;
}

void VersificationMgr::System::translateVerse(const System *dstSys, const char **book, int *chapter, int *verse, int *verse_end) const {
//dbg_mapping SWLOGD("translate verse from %s to %s: %s.%i.%i-%i\n",getName(), dstSys->getName(), *book, *chapter, *verse, *verse_end);

	if (!strcmp(getName(),"KJVA") || !strcmp(getName(),"KJV")) {
		if (!strcmp(dstSys->getName(),"KJVA") || !strcmp(dstSys->getName(),"KJV"))
			return;
		// reversed mapping
		//dbg_mapping SWLOGD("Perform reversed mapping.\n");
		int b = dstSys->getBookNumberByOSISName(*book)-1;

		//dbg_mapping SWLOGD("\tgetBookNumberByOSISName %i %s.\n", b, *book);

		if (b < 0) {
			int extraNum = dstSys->p->resolveMappingBook(*book);	// extra books resolve past the book count
			if (extraNum > (int)dstSys->p->books.size()) b = extraNum - 1;
		}

		//dbg_mapping SWLOGD("\tb %i.\n", b);

		if (b >= (int)dstSys->p->rulesByBook.size() || b < 0) {
			//dbg_mapping SWLOGD("no modification");
			return;
		}
		
		const Private::MappingRule *a = NULL;
		
		// reversed mapping should use forward search for item
		for (unsigned int i=0; i<dstSys->p->rulesByBook[b].size(); ++i) {
			const Private::MappingRule &m = dstSys->p->mappingRules[dstSys->p->rulesByBook[b][i]];

			// we are looking up by the reference book: skip inter-book rules
			// listed here under their (our) destination book
			if ((m.refBookNum ? m.refBookNum : m.ourBookNum) != b+1) continue;

			if (m.refChapter == *chapter && m.refVerse <= *verse) {
				if (m.refVerse == *verse || (m.refVerseMax >= *verse && m.refVerse <= *verse)) {
					// inside of any mapping range
					*chapter = m.ourChapter;
					*verse = m.ourVerse;
					*verse_end = m.ourVerseMax;
					if (m.refBookNum) *book = dstSys->p->bookOSISName(m.ourBookNum);
					return;
				}
				// destination mapping can have duplicate items, use the last (by using <=)
				if (a == NULL || (a->refVerse>a->refVerseMax?a->refVerse:a->refVerseMax) <= (m.refVerse>m.refVerseMax?m.refVerse:m.refVerseMax))
					a = &m;
			}
		}
		if (a != NULL) {
			(*chapter) = a->ourChapter;
			// shift verse
			const int d = (a->ourVerseMax>a->ourVerse?a->ourVerseMax:a->ourVerse)-(a->refVerseMax>a->refVerse?a->refVerseMax:a->refVerse);
			if (*verse < *verse_end)
				*verse_end += d;
			else
				*verse_end = (*verse) + d;
			*verse += d;
			if (a->refBookNum) *book = dstSys->p->bookOSISName(a->ourBookNum);
			return;
		}
	}
	else if (strcmp(dstSys->getName(),"KJVA") && strcmp(dstSys->getName(),"KJV")) {
		const System *kjva = getSystemVersificationMgr()->getVersificationSystem("KJVA");
		const int src_verse = *verse;
		
		translateVerse(kjva, book, chapter, verse, verse_end);
		
		int interm_verse = *verse, interm_range = *verse_end, interm_chapter = *chapter;
		const char *interm_book = *book;
		
		kjva->translateVerse(dstSys, book, chapter, verse, verse_end);
		
		// contraction->expansion fix
		if (verse < verse_end && !(interm_verse < interm_range)) {
			kjva->translateVerse(this, &interm_book, &interm_chapter, &interm_verse, &interm_range);
			if (interm_verse < interm_range) {
				*verse += src_verse - interm_verse;
				if (*verse > *verse_end)
					*verse = *verse_end;
				else
					*verse_end = *verse;
			}
		}
	}
	else {
		//dbg_mapping SWLOGD("Perform forward mapping.\n");
		const int b = getBookNumberByOSISName(*book)-1;
		if (b >= (int)p->rulesByBook.size())
			return;
		// forward mapping should use reversed search for item
		for (int i = (int)p->rulesByBook[b].size()-1; i>=0; --i) {
			const Private::MappingRule &m = p->mappingRules[p->rulesByBook[b][i]];
			if (m.ourChapter < *chapter) {
				SWLog::getSystemLog()->logWarning("There is no mapping for this chapter.\n");
				return;
			}
			if (m.ourChapter == *chapter && m.ourVerse <= *verse) {
				if (m.ourVerse == *verse || (m.ourVerseMax >= *verse && m.ourVerse <= *verse)) {
					*chapter = m.refChapter;
					*verse = m.refVerse;
					*verse_end = m.refVerseMax;
				}
				else {
					*chapter = m.refChapter;
					// shift verse
					const int d = (m.refVerseMax>m.refVerse?m.refVerseMax:m.refVerse)-(m.ourVerseMax>m.ourVerse?m.ourVerseMax:m.ourVerse);
					if (*verse < *verse_end)
						*verse_end += d;
					else
						*verse_end = (*verse) + d;
					*verse += d;
				}
				if (m.refBookNum) *book = p->bookOSISName(m.refBookNum);
				return;
			}
		}
		//dbg_mapping SWLOGD("No mapping.\n");
	}
}

SWORD_NAMESPACE_END

