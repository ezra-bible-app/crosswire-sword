/******************************************************************************
 *
 *  v11ndump.cpp -	dump a versification system as v11n XML or as a
 *			SWORD canon_*.h header
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

#include <versificationmgr.h>
#include <swbuf.h>
#include <swlog.h>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace sword;
using std::cout;

void usage(const char *app, const char *error = 0) {
	if (error) fprintf(stderr, "\n%s: %s\n", app, error);
	fprintf(stderr, "\nusage: %s [options] <v11nName>\n", app);
	fprintf(stderr, "  -f <xml|h>\t\t output format (default xml)\n");
	fprintf(stderr, "  -r <definition.xml>\t register this definition first (e.g., to convert or rename it)\n");
	fprintf(stderr, "  -n <name>\t\t dump under this name instead of <v11nName>; with -r, also register under it (e.g., seeding Bible from LXXNU)\n");
	fprintf(stderr, "\nRegistered systems:\n");
	StringList systems = VersificationMgr::getSystemVersificationMgr()->getVersificationSystems();
	for (StringList::iterator it = systems.begin(); it != systems.end(); ++it) {
		fprintf(stderr, "\t%s\n", it->c_str());
	}
	fprintf(stderr, "\n");
	exit(-1);
}


static SWBuf xmlEscape(const char *s) {
	SWBuf r;
	for (; s && *s; ++s) {
		switch (*s) {
		case '&': r += "&amp;"; break;
		case '<': r += "&lt;"; break;
		case '>': r += "&gt;"; break;
		case '"': r += "&quot;"; break;
		default: r += *s;
		}
	}
	return r;
}


void dumpXML(const VersificationMgr::System *s, const char *name) {
	const int *bmax = const_cast<VersificationMgr::System *>(s)->getBMAX();
	cout << "<v11n v11nID=\"" << xmlEscape(name) << "\"";
	if (strcmp(s->getRefSys(), "Bible")) cout << " refSys=\"" << xmlEscape(s->getRefSys()) << "\"";
	cout << ">\n";
	int bookNum = 0;
	for (int collection = 1; collection <= 2; ++collection) {
		int count = bmax[collection-1];
		if (!count) continue;
		cout << "\t<collection collectionID=\"" << collection << "\">\n";
		for (int i = 0; i < count; ++i, ++bookNum) {
			const VersificationMgr::Book *b = s->getBook(bookNum);
			cout << "\t\t<book osisID=\"" << xmlEscape(b->getOSISName())
			     << "\" preferredAbbrev=\"" << xmlEscape(b->getPreferredAbbreviation())
			     << "\" name=\"" << xmlEscape(b->getLongName())
			     << "\" bookNum=\"" << (i+1)
			     << "\" chapterMax=\"" << b->getChapterMax() << "\">\n";
			for (int c = 1; c <= b->getChapterMax(); ++c) {
				cout << "\t\t\t<chapter osisID=\"" << xmlEscape(b->getOSISName()) << "." << c
				     << "\" verseMax=\"" << b->getVerseMax(c) << "\"/>\n";
			}
			cout << "\t\t</book>\n";
		}
		cout << "\t</collection>\n";
	}

	if (s->getAddedBookAbbrevCount()) {
		cout << "\t<abbreviations>\n";
		for (int i = 0; i < s->getAddedBookAbbrevCount(); ++i) {
			const char *ab, *osis;
			s->getAddedBookAbbrev(i, &ab, &osis);
			cout << "\t\t<abbr osisID=\"" << xmlEscape(osis) << "\">" << xmlEscape(ab) << "</abbr>\n";
		}
		cout << "\t</abbreviations>\n";
	}

	int ruleCount = s->getMappingRuleCount();
	int extraCount = s->getMappingExtraBookCount();
	if (ruleCount || extraCount) {
		cout << "\t<mappings>\n";
		if (extraCount) {
			cout << "\t\t<extraBooks>\n";
			for (int i = 0; i < extraCount; ++i) {
				cout << "\t\t\t<book name=\"" << xmlEscape(s->getMappingExtraBook(i)) << "\"/>\n";
			}
			cout << "\t\t</extraBooks>\n";
		}
		for (int i = 0; i < ruleCount; ++i) {
			const VersificationMgr::System::MappingRule &r = *s->getMappingRule(i);
			// MappingRule's own attribute names; books named, not numbered
			cout << "\t\t<map ourBook=\"" << xmlEscape(r.ourBook)
			     << "\" ourChapter=\"" << r.ourChapter << "\" ourVerse=\"" << r.ourVerse << "\" ourVerseMax=\"" << r.ourVerseMax
			     << "\" refBook=\"" << (r.refBook ? xmlEscape(r.refBook) : SWBuf(""))
			     << "\" refChapter=\"" << r.refChapter << "\" refVerse=\"" << r.refVerse << "\" refVerseMax=\"" << r.refVerseMax
			     << "\" />\n";
		}
		cout << "\t</mappings>\n";
	}
	cout << "</v11n>\n";
}


void dumpHeader(const VersificationMgr::System *s, const char *name) {
	SWBuf lower = name; for (unsigned long i = 0; i < lower.length(); ++i) lower[i] = tolower(lower[i]);
	const int *bmax = const_cast<VersificationMgr::System *>(s)->getBMAX();

	cout << "/******************************************************************************\n";
	cout << " *\n *  canon_" << lower << ".h -\tVersification data for the " << name << " system\n";
	cout << " *\n *  generated by v11ndump\n *\n */\n\n";
	cout << "#ifndef CANON_" << SWBuf(name).toUpper() << "_H\n#define CANON_" << SWBuf(name).toUpper() << "_H\n\n";
	cout << "SWORD_NAMESPACE_START\n\n";

	int bookNum = 0;
	for (int t = 0; t < 2; ++t) {
		if (!bmax[t]) continue;
		cout << "// " << (t ? "New" : "Old") << " Testament\n";
		cout << "struct sbook " << (t ? "ntbooks_" : "otbooks_") << lower << "[] = {\n";
		for (int i = 0; i < bmax[t]; ++i, ++bookNum) {
			const VersificationMgr::Book *b = s->getBook(bookNum);
			cout << "  {\"" << b->getLongName() << "\", \"" << b->getOSISName() << "\", \"" << b->getPreferredAbbreviation() << "\", " << b->getChapterMax() << "},\n";
		}
		cout << "  {\"\", \"\", \"\", 0}\n};\n\n";
	}

	cout << "int vm_" << lower << "[] = {\n";
	for (int i = 0; i < s->getBookCount(); ++i) {
		const VersificationMgr::Book *b = s->getBook(i);
		cout << "  // " << b->getOSISName() << "\n  ";
		for (int c = 1; c <= b->getChapterMax(); ++c) {
			cout << b->getVerseMax(c) << (i == s->getBookCount()-1 && c == b->getChapterMax() ? "" : ",");
			if (!(c % 10) && c != b->getChapterMax()) cout << "\n  ";
			else if (c != b->getChapterMax()) cout << " ";
		}
		cout << "\n";
	}
	cout << "};\n\n";

	int ruleCount = s->getMappingRuleCount();
	if (ruleCount) {
		cout << "// our book chapter:verse[..verseMax] == the reference system's book chapter:verse[..verseMax]\n";
		cout << "// (verseMax 0 = single verse; reference book 0 = same book as ours)\n";
		cout << "VersificationMgr::System::MappingRule mappings_" << lower << "[] = {\n";
		cout << "//   ourBook    ch,   v,vMax    refBook    ch,   v,vMax\n";
		for (int i = 0; i < ruleCount; ++i) {
			const VersificationMgr::System::MappingRule &r = *s->getMappingRule(i);
			SWBuf our = SWBuf("\"") + r.ourBook + "\",";
			SWBuf ref = r.refBook ? SWBuf("\"") + r.refBook + "\"," : SWBuf("0,");
			printf("  { %-10s %4d,%4d,%4d,    %-10s %4d,%4d,%4d },\n",
				our.c_str(), r.ourChapter, r.ourVerse, r.ourVerseMax,
				ref.c_str(), r.refChapter, r.refVerse, r.refVerseMax);
		}
		cout << "  { 0,             0,   0,   0,    0,             0,   0,   0 }\n};\n\n";
	}

	cout << "SWORD_NAMESPACE_END\n\n#endif\n";
}


int main(int argc, char **argv) {
	SWBuf format = "xml";
	SWBuf registerPath;
	SWBuf outName;
	SWBuf systemName;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-f")) {
			if (i+1 < argc) format = argv[++i];
			else usage(*argv, "-f requires <xml|h>");
		}
		else if (!strcmp(argv[i], "-r")) {
			if (i+1 < argc) registerPath = argv[++i];
			else usage(*argv, "-r requires <definition.xml>");
		}
		else if (!strcmp(argv[i], "-n")) {
			if (i+1 < argc) outName = argv[++i];
			else usage(*argv, "-n requires <name>");
		}
		else if (!systemName.length()) systemName = argv[i];
		else usage(*argv, (((SWBuf)"Unknown argument: ") + argv[i]).c_str());
	}

	VersificationMgr *vmgr = VersificationMgr::getSystemVersificationMgr();

	if (registerPath.length()) {
		SWBuf registered;
		if (!vmgr->registerVersificationSystemFromXML(registerPath.c_str(), outName.length() ? outName.c_str() : 0, &registered)) {
			usage(*argv, (((SWBuf)"could not register ") + registerPath).c_str());
		}
		if (!systemName.length()) systemName = registered;
	}

	if (!systemName.length()) usage(*argv, "which versification system?");
	if (format != "xml" && format != "h") usage(*argv, "-f must be xml or h");

	const VersificationMgr::System *s = vmgr->getVersificationSystem(systemName.c_str());
	if (!s) usage(*argv, (((SWBuf)"versification not found: ") + systemName).c_str());

	if (!outName.length()) outName = systemName;

	if (format == "xml") dumpXML(s, outName.c_str());
	else dumpHeader(s, outName.c_str());

	return 0;
}
