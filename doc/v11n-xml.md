# Versification definitions in XML

**TL;DR**

- A v11n is an XML file: books in order, chapters in order, `verseMax` each.
  Everything else (`abbreviations`, `mappings`, `base=`) is optional.
- Read by C++ (`VersificationMgr::registerVersificationSystemFromXML`), the
  Java port, and served by the VMR (`metadata/v11n/get`): one file, three readers.
- A module ships one as `v11n.xml` in its DataPath; SWMgr registers it under
  the module's name.  Build with `osis2mod -v file.xml` so data and system agree.
- First registration wins; nothing ever replaces a registered name.
- `<v11n base="X">` = a delta on a registered system: list only what differs.
- `v11ndump` writes any registered system back out in this format.
- **Don't define a new system for a small variation on a built-in one.**
  Use the closest built-in; osis2mod folds the odd extra verse into its
  neighbour. See *Before you write one*.

Design rationale for the mapping hub: [bible-intermediate-v11n.md](bible-intermediate-v11n.md).

## Example

```xml
<v11n v11nID="TestCustom">
	<collection collectionID="1">
		<book osisID="Gen" preferredAbbrev="Gen" name="Genesis">
			<chapter osisID="Gen.1" verseMax="31"/>
			<chapter osisID="Gen.2" verseMax="25"/>
		</book>
	</collection>
	<collection collectionID="2">
		<book osisID="Matt" preferredAbbrev="Mt" name="Matthew">
			<chapter osisID="Matt.1" verseMax="25"/>
		</book>
	</collection>
	<abbreviations>
		<abbr osisID="Matt">Matthieu</abbr>
	</abbreviations>
	<mappings>
		<map ourBook="Ps" ourChapter="10" ourVerse="1" ourVerseMax="0"
		     refBook=""    refChapter="9"  refVerse="22" refVerseMax="0"/>
	</mappings>
</v11n>
```

## Before you write one

**TL;DR** a versification is a shared address space, not a description of
your text. Pick the built-in system your text is closest to; define a new
one only when no built-in can hold the text at all.

A module on a built-in system is comparable verse for verse with every other
module on it, parallel displays line up, and the av11n mappings to the other
systems exist and have been checked. A module on its own system has none of
that: it is an island until someone writes and verifies its mappings, every
frontend lists it as one more entry, and its book list is frozen the moment
the module ships (changing it invalidates the module's keys). Twenty systems
for twenty editions of one Bible is the failure mode the built-in set exists
to prevent.

So, for a text that differs from a built-in by a verse here and there:

- **Use the built-in.** `osis2mod` keeps the text: a verse the system lacks
  is appended to the last verse that exists, logged as `INFO(V11N) ... Verse
  is not in the ... versification. Appending content to ...`. A chapter
  break that falls one verse off is the same case. Nothing is lost; the
  address is approximate.
- **A verse the system has but your text lacks** is simply empty. Link it to
  its neighbour if the edition merges them (`<verse osisID="Gen.1.1 Gen.1.2">`
  style ranges in OSIS do this).
- **Wrong mappings, not a wrong system.** If the problem is that your text
  maps badly to KJV or LXX, the fix is a mapping correction to the existing
  system, which is allowed to change; a new system is not the fix.

Define a new system when the built-ins genuinely cannot hold the text: a
different canon (books none of them have, or in an order that matters to
the edition), a work that is not a Bible at all (`refSys` other than
`Bible`), or a verse structure so different that the folded verses would be
most of a chapter. Prefer a **delta** (`base=`) on the nearest built-in
even then: it inherits the mappings and abbreviations, and it documents in
a dozen lines exactly how the edition deviates.

## Elements

| element / attribute | meaning |
|---|---|
| `<v11n v11nID>` | the system's name (what `Versification=` in a conf names). Required unless the caller overrides it. |
| `<v11n refSys>` | what the books are: `Bible` (default) or another work (`Homer`, `ChrHom`). Picks the built-in abbreviation seed (Bible's, or none). Only systems of one `refSys` map to each other. |
| `<v11n base>` | delta on a registered system; see *Deltas*. |
| `<collection collectionID>` | storage testament: `1` OT, `2` NT (default). Same-id collections concatenate in document order, so an appendix is a second collection in 2. |
| `<collection name>` | optional label; books report it via `Book::getCollectionName()` so a UI can group them. Invisible to keys and modules. |
| `<book osisID>` | required. `name` and `preferredAbbrev` are also accepted as abbreviations when parsing. |
| `<book after>` / `before` | delta only: place a new book next to an existing one. |
| `<chapter osisID verseMax>` | chapter number = last dotted segment of the osisID (so a delta can say `Ps.151` alone); omitted = next in sequence. |
| `<abbreviations><abbr osisID>text</abbr>` | extra parse spellings; stored uppercased. Precedence when parsing: locale `[Book Abbrevs]` > these > the `refSys` built-in table. |
| `<mappings>` | see below. |
| `bookNum`, `chapterMax` | written by dumps, **ignored on read** (order and chapter count come from the document). |

## Mappings

**TL;DR** a `<map>` says "our `Book ch:v[-vMax]` is the reference system's
`ch:v[-vMax]`". `our*` = this system, `ref*` = the reference (KJVA in C++,
LXXNU in Java). Empty `refBook` = same book. `verseMax` 0 = single verse.
Later rules win. Never reorder.

```xml
<map ourBook="Ps" ourChapter="10" ourVerse="1" ourVerseMax="0"
     refBook=""    refChapter="9"  refVerse="22" refVerseMax="0"/>
```

- A rule anchors a verse or range; verses after it in the same chapter shift
  by the rule's offset. One rule at verse 1 renumbers a whole chapter; split
  or merged verses need a rule per range. A chapter with no rule is unmapped.
- Books by osisID, never by position. A `refBook` this system lacks (`EpJer`
  for a system that folds it into `Bar`) becomes a *mapping extra book*.
- `<extraBooks><book name="EpJer"/></extraBooks>` inside `<mappings>` declares
  those up front. Optional on input, always written by dumps (their order is
  part of the system).
- Rule order is precedence: the forward lookup walks a book's rules from the
  last. Built-in systems interleave rules deliberately. Dumps preserve order.
- Older notes call the fields `to*`/`from*` and read them backwards; the
  layout is the same.

## Deltas

**TL;DR** `<v11n v11nID="X" base="Vulg">` starts from Vulg and applies only
what the body says. Base not registered = refused.

```xml
<v11n v11nID="TestDelta" base="Vulg">
	<collection collectionID="1">
		<book osisID="Ps"><chapter osisID="Ps.151" verseMax="7"/></book>
		<book osisID="3Macc" name="III Maccabees" preferredAbbrev="3Macc" after="2Macc">
			<chapter osisID="3Macc.1" verseMax="29"/>
		</book>
	</collection>
	<collection name="Appendix">
		<book osisID="EpLao"/>
		<book osisID="Barn" name="Epistle of Barnabas" preferredAbbrev="Barn">
			<chapter osisID="Barn.1" verseMax="8"/>
		</book>
	</collection>
</v11n>
```

1. Start with the base's books, OT then NT.
2. Merge each delta book in document order:
   - **base has it**: patch only the chapters listed (`Ps.151` extends the
     Psalter; the other 150 stay); a `name`, `preferredAbbrev` or enclosing
     collection name replaces the base's (`<book osisID="EpLao"/>` under
     `Appendix` relabels it in place). `after`/`before` ignored with a warning.
   - **base lacks it**: insert `after=`/`before=` the named book (same
     testament), else at the end of its collection's testament. Missing
     anchor = warn and append.
3. `refSys`, mappings and abbreviations are inherited; the delta's rules and
   abbrevs are appended (so they take precedence).

`v11ndump -r delta.xml Name` prints the resolved system in full; use it to
see what a delta did.

## Registration

- **First wins.** Re-registering a name is a successful no-op. Systems live
  for the process (keys hold raw pointers). No file can hijack a built-in or
  another module's system.
- Fails (false + logged error) when: file unreadable, no name and no
  override, no books and no base, base not registered.
- `registerVersificationSystemFromXML(path, overrideName, &registeredName)`:
  override the file's `v11nID`; get back the effective name.

## Where definitions come from

| source | behaviour |
|---|---|
| module `DataPath/v11n.xml` | SWMgr registers it **under the module's name** for text/commentary drivers (`zText`, `RawText`, `zCom`, `RawCom`, their `4` variants, `RawFiles`, `HREFCom`) and sets the module's effective `Versification` to it. Two modules with different files never collide. |
| `osis2mod -v file.xml`, `imp2vs -v file.xml` | register, then build with that system. `-v` still takes a name or unique prefix; a `.xml` that exists is a definition. |
| application code | call `registerVersificationSystemFromXML`; reads through `FileMgr`, so pluggable file systems work. |
| Java port, sysconfig `AddVersifications` | `;`-separated files, registered before the built-ins, bases first. The VMR's family lives in its `contrib/v11ns`. |

## Tools and tests

`v11ndump [-f xml|h] [-r def.xml] [-n name] <system>`: any registered system
as XML (canonical serialisation) or a `canon_*.h` header; no args lists systems.
Tests: `tests/testsuite/v11nxml.sh` (register, parse, module hook, build with
`-v`) and `v11ndelta.sh` (every delta rule, inherited mappings, refused base,
dump round trip). The `tests/testsuite/v11n*.xml` fixtures are complete small examples.

## Gotchas

- The C++ loader **streams tags, it is not an XML parser**: every `<...>` in
  the file is read, comments and CDATA included. No element-looking text in
  comments; no entities.
- Unknown attributes and elements are ignored (the VMR's `Paratext` file
  carries a `<facets>` block for its own pickers, for instance).
- Chapter/verse 0 (introductions) are not in the file; the engine supplies them.
- A system is a contract: a module built against it depends on its book list
  and verse counts. Fix mappings freely; a changed book list is a new name.
- The Java loader mirrors the C++ step for step (same names, same warnings);
  its `VersificationDeltaTest` ports `v11ndeltatest.cpp`.
