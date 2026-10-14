/******************************************************************************
 *
 *  flatapitest.cpp -	the flat C API (bindings/flatapi.cpp) under churn:
 *			every entry point short of InstallMgr and BibleSync
 *			called over and over, handles made and freed, for
 *			valgrind to watch.  What the calls answered is printed
 *			once, so the output is the same at any count.
 *
 *			usage: flatapitest <libraryPath> [count]
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

#include <flatapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define SW(x) org_crosswire_sword_##x

static int N = 100;		// how often each thing is done
static int fails = 0;

#define CHECK(c, msg) do { if (!(c)) { fails++; printf("FAIL: %s\n", msg); } } while (0)

// print a string array, once per label
static void show(const char *label, const char **a) {
	printf("%s:", label);
	if (!a) printf(" (null)");
	else for (int i = 0; a[i]; i++) printf(" [%s]", a[i]);
	printf("\n");
}
static int count(const char **a) { int n = 0; if (a) while (a[n]) n++; return n; }
static bool has(const char **a, const char *s) { for (int i = 0; a && a[i]; i++) if (!strcmp(a[i], s)) return true; return false; }

static char *upperAscii(const char *buf, unsigned long maxLen) {
	for (char *p = (char *)buf; *p; p++) *p = toupper(*p);
	return (char *)buf;
}
static int progressCalls = 0;
static void progress(int percent) { progressCalls++; }

// ---------------------------------------------------------------- SWMgr
static void mgrRound(const char *path) {
	SWHANDLE mgr = SW(SWMgr_newWithPath)(path);
	CHECK(mgr, "manager");
	CHECK(SW(SWMgr_version)(mgr) && *SW(SWMgr_version)(mgr), "version");
	CHECK(SW(SWMgr_getPrefixPath)(mgr) && SW(SWMgr_getConfigPath)(mgr), "paths");
	const struct org_crosswire_sword_ModInfo *mods = SW(SWMgr_getModInfoList)(mgr);
	mods = SW(SWMgr_getModInfoList)(mgr);		// the list is replaced, the old one freed
	int n = 0; for (; mods[n].name; n++) ;
	CHECK(n == 5, "five modules");
	const char **opts = SW(SWMgr_getGlobalOptions)(mgr);
	CHECK(has(opts, "Strong's Numbers"), "Strong's option listed");
	for (int i = 0; opts[i]; i++) {
		const char **vals = SW(SWMgr_getGlobalOptionValues)(mgr, opts[i]);
		CHECK(count(vals) >= 1, "option has values");
		CHECK(SW(SWMgr_getGlobalOptionTip)(mgr, opts[i]) != 0, "option tip");
		for (int v = 0; vals[v]; v++) {
			SW(SWMgr_setGlobalOption)(mgr, opts[i], vals[v]);
			CHECK(!strcmp(SW(SWMgr_getGlobalOption)(mgr, opts[i]), vals[v]), "option set and read back");
		}
		SW(SWMgr_setGlobalOption)(mgr, opts[i], vals[0]);
	}
	SW(SWMgr_getAvailableLocales)(mgr);
	SW(SWMgr_setDefaultLocale)(mgr, "en");
	CHECK(!strcmp(SW(SWMgr_translate)(mgr, "Genesis", "en"), "Genesis"), "translate");
	SW(SWMgr_setGlobalOption)(mgr, "Greek Accents", "Off");
	const char *stripped = SW(SWMgr_filterText)(mgr, "Greek Accents", "\xe1\xbc\x80\xce\xb3\xce\xac\xcf\x80\xce\xb7");	// ἀγάπη
	CHECK(stripped && !strcmp(stripped, "\xce\xb1\xce\xb3\xce\xb1\xcf\x80\xce\xb7"), "accents stripped");	// αγαπη
	CHECK(SW(SWMgr_filterText)(mgr, "No Such Filter", "text") != 0, "unknown filter still answers");
	SW(SWMgr_setCipherKey)(mgr, "zText", "");
	SW(SWMgr_setCipherKey)(mgr, "NoSuchModule", "x");
	SW(SWMgr_setJavascript)(mgr, 1);
	SW(SWMgr_setJavascript)(mgr, 0);
	CHECK(SW(SWMgr_getModuleByName)(mgr, "NoSuchModule") == 0, "unknown module is 0");
	SWHANDLE a = SW(SWMgr_getModuleByName)(mgr, "zText"), b = SW(SWMgr_getModuleByName)(mgr, "zText");
	CHECK(a && a == b, "one handle per module");
	SW(SWMgr_delete)(mgr);
}

// ---------------------------------------------------------------- SWModule
static void moduleRound(SWHANDLE mgr, const char *name, const char *key, const char *searchWord, bool verbose) {
	SWHANDLE mod = SW(SWMgr_getModuleByName)(mgr, name);
	CHECK(mod, name);
	if (!mod) return;
	if (verbose) printf("-- %s: %s, %s, driver %s\n", SW(SWModule_getName)(mod), SW(SWModule_getDescription)(mod), SW(SWModule_getCategory)(mod), SW(SWModule_getConfigEntry)(mod, "ModDrv"));
	CHECK(SW(SWModule_getConfigEntry)(mod, "NoSuchEntry") == 0, "missing config entry is 0");

	SW(SWModule_setKeyText)(mod, key);
	if (verbose) printf("key %s, entry size %ld\n", SW(SWModule_getKeyText)(mod), SW(SWModule_getEntrySize)(mod));
	const char *render = SW(SWModule_renderText)(mod);
	const char *strip = SW(SWModule_stripText)(mod);
	const char *header = SW(SWModule_getRenderHeader)(mod);
	const char *raw = SW(SWModule_getRawEntry)(mod);
	CHECK(render && strip && header && raw, "entry texts");
	if (verbose) printf("strip: %s\nraw: %s\n", strip, raw);
	// buffers are replaced on each call, the old ones freed
	for (int i = 0; i < 3; i++) { SW(SWModule_renderText)(mod); SW(SWModule_stripText)(mod); SW(SWModule_getRenderHeader)(mod); SW(SWModule_getRawEntry)(mod); SW(SWModule_getConfigEntry)(mod, "ModDrv"); }

	SW(SWModule_begin)(mod);
	char *first = strdup(SW(SWModule_getKeyText)(mod));	// the pointer is the key's own: copy before moving
	SW(SWModule_next)(mod); SW(SWModule_next)(mod); SW(SWModule_previous)(mod);
	if (verbose) printf("begin %s, next next previous %s\n", first, SW(SWModule_getKeyText)(mod));
	SW(SWModule_begin)(mod); SW(SWModule_previous)(mod);
	CHECK(SW(SWModule_popError)(mod) != 0 && SW(SWModule_popError)(mod) == 0, "previous before the top errors once");

	const char **parsed = SW(SWModule_parseKeyList)(mod, "Gen.1.1-3; Gen.1.5");
	if (verbose) show("parseKeyList", parsed);
	parsed = SW(SWModule_parseKeyList)(mod, "Jn.3.16");	// replaced, the old list freed
	SW(SWModule_setKeyText)(mod, key);
	const char **attrs = SW(SWModule_getEntryAttribute)(mod, "-", 0, 0, 0);
	if (verbose) show("entry attribute types", attrs);
	attrs = SW(SWModule_getEntryAttribute)(mod, 0, 0, "*", 0);
	CHECK(attrs, "entry attributes array");
	attrs = SW(SWModule_getEntryAttribute)(mod, "Footnote", "1", "body", 0);
	if (verbose) show("footnote 1 body raw", attrs);
	attrs = SW(SWModule_getEntryAttribute)(mod, "Footnote", "1", "body", 1);
	if (verbose) show("footnote 1 body rendered", attrs);
	attrs = SW(SWModule_getEntryAttribute)(mod, "Footnote", "-", 0, 0);
	if (verbose) show("footnote names", attrs);

	progressCalls = 0;
	const struct org_crosswire_sword_SearchHit *hits = SW(SWModule_search)(mod, searchWord, org_crosswire_sword_SWModule_SEARCHTYPE_MULTIWORD, 0, "", progress);
	int n = 0; for (; hits && hits[n].key; n++) ;
	if (verbose) { printf("search '%s': %d hits, progress reported %s", searchWord, n, progressCalls ? "yes" : "no"); for (int i = 0; i < n && i < 3; i++) printf(", %s", hits[i].key); printf("\n"); }
	hits = SW(SWModule_search)(mod, searchWord, org_crosswire_sword_SWModule_SEARCHTYPE_MULTIWORD, 0, "", 0);	// hits replaced, no reporter
	SW(SWModule_terminateSearch)(mod);
	CHECK(SW(SWModule_hasSearchFramework)(mod) == 0, "no lucene index");
	SW(SWModule_deleteSearchFramework)(mod);
	// the module's key as a key handle; its clone walks without moving the module
	SW(SWModule_setKeyText)(mod, key);
	// the module's key text through the key handle, and the module's own
	// getKeyText (a dictionary answers the entry's stored key there); each
	// pointer is the key's own, so copy one before asking for the other
	char *viaKey = strdup(SW(SWKey_getText)(SW(SWModule_getKey)(mod)));
	if (verbose) printf("key handle text %s, module key text %s\n", viaKey, SW(SWModule_getKeyText)(mod));
	SWHANDLE k = SW(SWKey_clone)(SW(SWModule_getKey)(mod));
	CHECK(k, "key clone");
	SW(SWKey_begin)(k); SW(SWKey_next)(k);
	CHECK(!strcmp(SW(SWKey_getText)(SW(SWModule_getKey)(mod)), viaKey), "module key stays while its clone moves");
	if (verbose) printf("clone after begin+next: %s\n", SW(SWKey_getText)(k));
	free(viaKey);
	SW(SWKey_getChildren)(k); SW(SWKey_getParent)(k); SW(SWKey_hasChildren)(k);
	SW(SWKey_delete)(k);
	SW(SWKey_delete)(SW(SWModule_getKey)(mod));	// a module's key: no-op
	CHECK(SW(SWModule_getKeyChildren)(mod) != 0 || !strcmp(SW(SWModule_getCategory)(mod), "Lexicons / Dictionaries"), "children on a verse or tree key");
	free(first);
}

static void writableRound(SWHANDLE mgr) {
	// setRawEntry on the raw drivers: write, read back, restore
	const char *names[] = { "RawText", "RawLD", "GBS", 0 };
	const char *keys[]  = { "Gen.1.1", "0002", "/Chapter 2", 0 };
	for (int m = 0; names[m]; m++) {
		SWHANDLE mod = SW(SWMgr_getModuleByName)(mgr, names[m]);
		SW(SWModule_setKeyText)(mod, keys[m]);
		char *was = strdup(SW(SWModule_getRawEntry)(mod));
		SW(SWModule_setRawEntry)(mod, "written by flatapitest");
		CHECK(!strcmp(SW(SWModule_getRawEntry)(mod), "written by flatapitest"), "raw entry written");
		SW(SWModule_setRawEntry)(mod, was);
		CHECK(!strcmp(SW(SWModule_getRawEntry)(mod), was), "raw entry restored");
		free(was);
	}
}

// ---------------------------------------------------------------- SWKey walks
static void keyRound(SWHANDLE mgr, bool verbose) {
	SWHANDLE mod = SW(SWMgr_getModuleByName)(mgr, "zText");
	SW(SWModule_setKeyText)(mod, "Gen.1.5");
	SWHANDLE k = SW(SWKey_clone)(SW(SWModule_getKey)(mod));
	int books = 0;
	for (SW(SWKey_begin)(k); SW(SWKey_popError)(k) == 0; SW(SWKey_setText)(k, "+book")) { SW(SWKey_getChildren)(k); books++; }
	CHECK(books == 66, "66 books in the walk");
	CHECK(!strcmp(SW(SWModule_getKeyText)(mod), "Genesis 1:5"), "module stays during the walk");
	CHECK(SW(SWKey_setText)(k, "Nonsense 9:9") != 0, "bad text errors");
	CHECK(SW(SWKey_popError)(k) != 0 && SW(SWKey_popError)(k) == 0, "error pops once");
	SW(SWKey_setText)(k, "=jn.1.0");
	if (verbose) printf("intro key: %s\n", SW(SWKey_getText)(k));
	SW(SWKey_setText)(k, "Gen.2.3"); SW(SWKey_setText)(k, "-chapter"); SW(SWKey_setText)(k, "+chapter"); SW(SWKey_previous)(k); SW(SWKey_next)(k);
	if (verbose) printf("steps: %s\n", SW(SWKey_getText)(k));
	const char **c = SW(SWKey_getChildren)(k);
	if (verbose) show("verse key children", c);
	SW(SWKey_delete)(k);

	// the borrowed handle follows the module across key replacement
	for (int i = 0; i < 20; i++) {
		SW(SWModule_setKeyText)(mod, (i % 2) ? "Jn.3.16" : "Gen.1.1");
		SWHANDLE mk = SW(SWModule_getKey)(mod);
		c = SW(SWKey_getChildren)(mk);
		CHECK(c && !strcmp(c[7], (i % 2) ? "John.3.16" : "Gen.1.1"), "borrowed children follow the module");
		SW(SWKey_delete)(mk);
		CHECK(SW(SWModule_getKey)(mod) != 0, "module key survives delete of the borrowed handle");
	}

	// the tree key: children, parent, hasChildren, on the module and on a clone
	mod = SW(SWMgr_getModuleByName)(mgr, "GBS");
	SW(SWModule_setKeyText)(mod, "/Chapter 6");
	CHECK(SW(SWModule_hasKeyChildren)(mod) == 1, "chapter 6 has children");
	c = SW(SWModule_getKeyChildren)(mod);
	if (verbose) show("tree children of /Chapter 6", c);
	k = SW(SWKey_clone)(SW(SWModule_getKey)(mod));
	SW(SWKey_setText)(k, "/Chapter 6/Section 2");
	CHECK(SW(SWKey_hasChildren)(k) == 0, "section has no children");
	const char *p = SW(SWKey_getParent)(k);
	if (verbose) printf("parent of /Chapter 6/Section 2: %s, clone now at %s, module still at %s\n", p, SW(SWKey_getText)(k), SW(SWModule_getKeyText)(mod));
	p = SW(SWKey_getParent)(k);	// root: empty, buffer reused
	CHECK(p && !*p, "root has no parent");
	c = SW(SWKey_getChildren)(k);
	if (verbose) show("tree children at the root", c);
	SW(SWKey_delete)(k);
	SW(SWModule_setKeyText)(mod, "/Chapter 6/Section 3");
	p = SW(SWModule_getKeyParent)(mod);
	CHECK(p && !strcmp(p, "/Chapter 6"), "module parent");

	// a string key (dictionary): neither verse nor tree; nothing to free, nothing read
	mod = SW(SWMgr_getModuleByName)(mgr, "RawLD");
	SW(SWModule_setKeyText)(mod, "0002");
	k = SW(SWKey_clone)(SW(SWModule_getKey)(mod));
	CHECK(k && SW(SWKey_getChildren)(k) == 0 && SW(SWKey_hasChildren)(k) == 0, "string key has no children");
	p = SW(SWKey_getParent)(k);
	CHECK(p && !*p, "string key parent empty");
	SW(SWKey_next)(k); SW(SWKey_previous)(k);
	SW(SWKey_delete)(k);
	CHECK(SW(SWModule_getKeyChildren)(mod) == 0, "module string key children");
}

// ---------------------------------------------------------------- SWConfig
static void configRound(const char *path, bool verbose) {
	char conf[1024]; snprintf(conf, sizeof conf, "%s/flatapitest.conf", path);
	remove(conf);
	CHECK(count(SW(SWConfig_getSections)(conf)) == 0, "no sections before the file exists");
	CHECK(SW(SWConfig_getKeyValue)(conf, "One", "a") == 0, "no value before the file exists");
	SW(SWConfig_setKeyValue)(conf, "One", "a", "1");
	SW(SWConfig_setKeyValue)(conf, "One", "b", "2");
	SW(SWConfig_setKeyValue)(conf, "Two", "c", "3");
	SW(SWConfig_setKeyValue)(conf, "One", "a", "one");	// replaced, not added
	// getKeyValue answers in one buffer it reuses: one value per statement
	if (verbose) { show("sections", SW(SWConfig_getSections)(conf)); show("keys of One", SW(SWConfig_getSectionKeys)(conf, "One")); printf("One/a = %s", SW(SWConfig_getKeyValue)(conf, "One", "a")); printf(", Two/c = %s", SW(SWConfig_getKeyValue)(conf, "Two", "c")); printf(", missing = %s\n", SW(SWConfig_getKeyValue)(conf, "Two", "zz") ? "set" : "(null)"); }
	CHECK(!strcmp(SW(SWConfig_getKeyValue)(conf, "One", "a"), "one"), "value replaced");
	const char **added = SW(SWConfig_augmentConfig)(conf, "[Three]\nd=4\n[One]\nb=two\n");
	if (verbose) { show("augment answered", added); show("sections after augment", SW(SWConfig_getSections)(conf)); printf("One/b = %s", SW(SWConfig_getKeyValue)(conf, "One", "b")); printf(", Three/d = %s\n", SW(SWConfig_getKeyValue)(conf, "Three", "d")); }
	const char **sections = SW(SWConfig_getSections)(conf);
	CHECK(has(sections, "One") && has(sections, "Two") && has(sections, "Three"), "three sections after augment");
	remove(conf);
}

// ---------------------------------------------------------------- bad handles
static void nullRound() {
	CHECK(SW(SWMgr_getModuleByName)(0, "x") == 0 && SW(SWMgr_getModInfoList)(0) == 0 && SW(SWMgr_getGlobalOptions)(0) == 0, "null manager");
	CHECK(SW(SWMgr_version)(0) != 0, "the version needs no manager");
	SW(SWMgr_delete)(0);
	CHECK(SW(SWModule_renderText)(0) == 0 && SW(SWModule_getKeyText)(0) == 0 && SW(SWModule_setKeyText)(0, "x") == -1 && SW(SWModule_popError)(0) == -1 && SW(SWModule_getKey)(0) == 0 && SW(SWModule_search)(0, "x", -2, 0, "", 0) == 0, "null module");
	SW(SWModule_begin)(0); SW(SWModule_next)(0); SW(SWModule_previous)(0); SW(SWModule_setRawEntry)(0, "x"); SW(SWModule_terminateSearch)(0);
	CHECK(SW(SWKey_clone)(0) == 0 && SW(SWKey_getChildren)(0) == 0 && SW(SWKey_getText)(0) == 0 && SW(SWKey_setText)(0, "x") == -1 && SW(SWKey_popError)(0) == -1 && SW(SWKey_getParent)(0) == 0 && SW(SWKey_hasChildren)(0) == 0, "null key");
	SW(SWKey_delete)(0); SW(SWKey_begin)(0); SW(SWKey_next)(0); SW(SWKey_previous)(0);
}

int main(int argc, char **argv) {
	if (argc < 2) { fprintf(stderr, "usage: %s <libraryPath> [count]\n", argv[0]); return 1; }
	const char *path = argv[1];
	if (argc > 2) N = atoi(argv[2]);

	SW(SWLog_setLogLevel)(org_crosswire_sword_SWLog_LOG_ERROR);
	CHECK(SW(SWLog_getLogLevel)() == org_crosswire_sword_SWLog_LOG_ERROR, "log level");
	SW(SWLog_logError)("flatapitest: an error line"); SW(SWLog_logWarning)("w"); SW(SWLog_logInformation)("i"); SW(SWLog_logTimedInformation)("t"); SW(SWLog_logDebug)("d");
	SW(StringMgr_setToUpper)(upperAscii);

	printf("-- managers\n");
	for (int i = 0; i < N / 10 + 1; i++) mgrRound(path);
	nullRound();

	SWHANDLE mgr = SW(SWMgr_newWithPath)(path);
	const struct org_crosswire_sword_ModInfo *mods = SW(SWMgr_getModInfoList)(mgr);
	for (int m = 0; mods[m].name; m++) printf("%s (%s, %s, v%s)\n", mods[m].name, mods[m].category, mods[m].language, mods[m].version);

	printf("-- modules\n");
	const char *names[] = { "zText", "RawText", "RawLD", "zLD", "GBS" };
	const char *keys[]  = { "Gen.1.3", "Gen.1.3", "0002", "0002", "/Chapter 6/Section 2" };
	const char *words[] = { "God", "God", "0", "0", "Section" };
	for (int i = 0; i < N; i++)
		for (int m = 0; m < 5; m++) moduleRound(mgr, names[m], keys[m], words[m], i == 0);
	for (int i = 0; i < N; i++) writableRound(mgr);

	printf("-- keys\n");
	for (int i = 0; i < N; i++) keyRound(mgr, i == 0);

	printf("-- config\n");
	for (int i = 0; i < N; i++) configRound(path, i == 0);

	// buffers left allocated on purpose: the handles free them
	SW(SWModule_renderText)(SW(SWMgr_getModuleByName)(mgr, "zText"));
	SW(SWModule_getKeyChildren)(SW(SWMgr_getModuleByName)(mgr, "zText"));
	SW(SWModule_search)(SW(SWMgr_getModuleByName)(mgr, "zText"), "God", org_crosswire_sword_SWModule_SEARCHTYPE_MULTIWORD, 0, "", 0);
	SW(SWMgr_getGlobalOptionValues)(mgr, "Strong's Numbers");
	SW(SWMgr_delete)(mgr);

	printf("%s (%d failures)\n", fails ? "FAILED" : "all checks passed", fails);
	return fails ? 1 : 0;
}
