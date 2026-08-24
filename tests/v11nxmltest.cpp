/******************************************************************************
 *
 *  v11nxmltest.cpp -	registerVersificationSystemFromXML
 *
 */

#include <versificationmgr.h>
#include <versekey.h>
#include <swmgr.h>
#include <swmodule.h>
#include <swlog.h>

#include <iostream>

using namespace sword;
using std::cout;

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "usage: " << *argv << " <v11n.xml>\n";
		return 1;
	}
	VersificationMgr *vmgr = VersificationMgr::getSystemVersificationMgr();

	cout << "register: " << (vmgr->registerVersificationSystemFromXML(argv[1]) ? "ok" : "FAILED") << "\n";
	const VersificationMgr::System *s = vmgr->getVersificationSystem("TestCustom");
	cout << "system found: " << (s ? "ok" : "FAILED") << "\n";
	if (!s) return 1;
	cout << "bookCount: " << s->getBookCount() << "\n";
	for (int i = 0; i < s->getBookCount(); i++) {
		const VersificationMgr::Book *b = s->getBook(i);
		cout << b->getOSISName() << " chapters: " << b->getChapterMax() << " lastVerseMax: " << b->getVerseMax(b->getChapterMax()) << "\n";
	}

	// first wins: a second registration under the same name is a no-op
	cout << "re-register: " << (vmgr->registerVersificationSystemFromXML(argv[1]) ? "ok" : "FAILED") << "\n";

	// custom books and abbreviations parse
	VerseKey vk;
	vk.setVersificationSystem("TestCustom");
	vk.setText("Vision of Daniel 1:7");
	cout << "abbr parse: " << vk.getOSISRef() << "\n";
	vk.setText("MATTHIEU 2:23");
	cout << "abbr parse: " << vk.getOSISRef() << "\n";
	vk.setText("FVD 1:14");
	cout << "osis parse: " << vk.getOSISRef() << "\n";
	// bounds honor the custom verseMax
	vk.setText("Gen 2:25");
	vk.increment();
	cout << "after Gen 2:25 comes: " << vk.getOSISRef() << "\n";

	// a module carrying v11n.xml in its data path gets its own system,
	// named after it, whatever its conf claimed
	if (argc > 3) {
		SWMgr mgr(argv[2]);
		SWModule *m = mgr.getModule(argv[3]);
		cout << "module found: " << (m ? "ok" : "FAILED") << "\n";
		if (m) {
			cout << "effective versification: " << m->getConfigEntry("Versification") << "\n";
			m->setKey("Vision of Daniel 1:9");
			VerseKey *mk = (VerseKey *)m->getKey();
			cout << "module key parse: " << mk->getOSISRef() << "\n";
		}
	}

	return 0;
}
