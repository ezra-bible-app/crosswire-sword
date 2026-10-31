/******************************************************************************
 *
 *  v11ndeltatest.cpp -	registerVersificationSystemFromXML with a base
 *  			versification (<v11n base="...">) and named collections
 *
 */

#include <versificationmgr.h>
#include <versekey.h>
#include <swlog.h>

#include <iostream>

using namespace sword;
using std::cout;

void showBook(const VersificationMgr::System *s, int i) {
	const VersificationMgr::Book *b = s->getBook(i);
	cout << "  " << (i+1) << " " << b->getOSISName()
	     << " (" << b->getLongName() << ", " << b->getPreferredAbbreviation() << ")"
	     << " chapters: " << b->getChapterMax()
	     << " lastVerseMax: " << b->getVerseMax(b->getChapterMax());
	if (*b->getCollectionName()) cout << " collection: " << b->getCollectionName();
	cout << "\n";
}

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "usage: " << *argv << " <delta.xml> [<badbase.xml>]\n";
		return 1;
	}
	VersificationMgr *vmgr = VersificationMgr::getSystemVersificationMgr();
	const VersificationMgr::System *base = vmgr->getVersificationSystem("Vulg");

	cout << "register: " << (vmgr->registerVersificationSystemFromXML(argv[1]) ? "ok" : "FAILED") << "\n";
	const VersificationMgr::System *s = vmgr->getVersificationSystem("TestDelta");
	cout << "system found: " << (s ? "ok" : "FAILED") << "\n";
	if (!s) return 1;

	// the base is untouched
	cout << "base bookCount: " << base->getBookCount() << " Ps chapters: " << base->getBook(base->getBookNumberByOSISName("Ps")-1)->getChapterMax() << "\n";
	cout << "bookCount: " << s->getBookCount() << " (base + 4)\n";
	cout << "BMAX: " << s->getBMAX()[0] << " " << s->getBMAX()[1] << "\n";
	cout << "refSys: " << s->getRefSys() << "\n";

	// placements: 3Macc after 2Macc; 4Macc at the end of the OT under
	// Apocrypha; Barn and Herm at the end of the NT under Appendix
	cout << "end of OT:\n";
	for (int i = s->getBMAX()[0]-4; i < s->getBMAX()[0]+1; i++) showBook(s, i);
	cout << "end of NT:\n";
	for (int i = s->getBookCount()-8; i < s->getBookCount(); i++) showBook(s, i);

	// patches: Ps grows a chapter; Matt 1 gets a verse; EpLao only changes collection
	cout << "patched:\n";
	showBook(s, s->getBookNumberByOSISName("Ps")-1);
	cout << "  Ps.150 verseMax: " << s->getBook(s->getBookNumberByOSISName("Ps")-1)->getVerseMax(150) << "\n";
	showBook(s, s->getBookNumberByOSISName("Matt")-1);
	cout << "  Matt.1 verseMax: " << s->getBook(s->getBookNumberByOSISName("Matt")-1)->getVerseMax(1) << "\n";
	cout << "  Matt.2 verseMax: " << s->getBook(s->getBookNumberByOSISName("Matt")-1)->getVerseMax(2) << "\n";

	// inherited mappings and abbreviations
	cout << "mapping rules: base " << base->getMappingRuleCount() << " delta " << s->getMappingRuleCount() << "\n";
	cout << "extra books: base " << base->getMappingExtraBookCount() << " delta " << s->getMappingExtraBookCount() << "\n";

	VerseKey vk;
	vk.setVersificationSystem("TestDelta");
	vk.setText("Shepherd 2:3");
	cout << "abbr parse: " << vk.getOSISRef() << "\n";
	vk.setText("Epistle of Barnabas 1:8");
	cout << "name parse: " << vk.getOSISRef() << "\n";
	vk.setText("Ps 151:7");
	cout << "osis parse: " << vk.getOSISRef() << "\n";
	// traversal crosses into the appended books
	vk.setText("Rev 22:21"); vk.increment();
	cout << "after Rev 22:21 comes: " << vk.getOSISRef() << "\n";
	vk.setText("Herm 3:5"); vk.increment();
	cout << "after Herm 3:5: " << vk.getOSISRef() << " error " << (int)vk.popError() << "\n";
	vk.setText("2Macc 15:40"); vk.increment();
	cout << "after 2Macc 15:40 comes: " << vk.getOSISRef() << "\n";
	// base mappings still apply to a base book: Vulgate Ps 146:1 is KJV Ps 147:1
	vk.setText("Ps 146:1");
	VerseKey kjv; kjv.setVersificationSystem("KJV");
	kjv.positionFrom(vk);
	cout << "Ps 146:1 in KJV: " << kjv.getOSISRef() << "\n";
	vk.setText("Ps 147:1"); kjv.positionFrom(vk);
	cout << "Ps 147:1 in KJV: " << kjv.getOSISRef() << "\n";

	// first wins here too
	cout << "re-register: " << (vmgr->registerVersificationSystemFromXML(argv[1]) ? "ok" : "FAILED") << "\n";

	// an unregistered base is refused
	if (argc > 2) {
		cout << "bad base register: " << (vmgr->registerVersificationSystemFromXML(argv[2]) ? "ok" : "refused") << "\n";
		cout << "bad base system found: " << (vmgr->getVersificationSystem("TestBadBase") ? "yes" : "no") << "\n";
	}

	return 0;
}
