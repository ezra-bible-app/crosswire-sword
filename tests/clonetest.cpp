/******************************************************************************
 *
 *  clonetest.cpp -	SWModule::clone(): for every module in a library, a
 *			clone sits where the original was, renders the same,
 *			stays put when the original moves, and leaves the
 *			original intact once deleted.
 *
 * $Id$
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

#include <swmgr.h>
#include <swmodule.h>
#include <markupfiltmgr.h>
#include <iostream>

using namespace sword;
using std::cout;

int main(int argc, char **argv) {
	if (argc < 2) {
		cout << "usage: " << *argv << " <library path> [key]\n";
		return 1;
	}
	const char *key = (argc > 2) ? argv[2] : "Gen.1.1";
	SWMgr mgr(argv[1], true, new MarkupFilterMgr(FMT_PLAIN));
	int failures = 0;

	for (ModMap::iterator it = mgr.Modules.begin(); it != mgr.Modules.end(); ++it) {
		SWModule *m = it->second;
		SWBuf type = m->getType();
		bool keyed = (type == "Biblical Texts" || type == "Commentaries");
		if (keyed) m->setKey(key); else { m->setPosition(TOP); m->increment(2); }
		SWBuf origKey  = m->getKeyText();
		SWBuf origText = m->renderText();

		SWModule *c = m->clone();
		if (!c) {
			cout << m->getName() << " (" << m->getConfigEntry("ModDrv") << "): no clone\n";
			++failures;
			continue;
		}
		SWBuf cText = c->renderText();
		SWBuf cKey  = c->getKeyText();	// a lexicon reports the entry last looked up

		// move the original; the clone must not follow
		if (keyed) m->setKey("Gen.1.1"); else m->setPosition(TOP);
		m->renderText();
		SWBuf cTextAfter = c->renderText();
		SWBuf cKeyAfter  = c->getKeyText();
		delete c;

		// the original is untouched by the clone's life and death
		m->setKey(origKey.c_str());
		SWBuf origAgain = m->renderText();

		bool ok = (cKey == origKey) && (cText == origText) && (cKeyAfter == origKey) && (cTextAfter == origText) && (origAgain == origText);
		cout << m->getName() << " (" << m->getConfigEntry("ModDrv") << "): " << (ok ? "ok" : "FAILED") << ", " << cKey << ", " << cText.length() << " bytes\n";
		if (!ok) ++failures;
	}
	cout << (failures ? "FAILED" : "all clones ok") << "\n";
	return failures ? 1 : 0;
}
