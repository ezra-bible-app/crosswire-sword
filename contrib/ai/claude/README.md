# Notes for AI-assisted work on the SWORD engine

Working notes kept by Claude across sessions, reviewed by Troy before
commit like everything else in this repository.

## Ground rules (Troy's, learned the hard way)

- **Never commit or push without Troy's review.**  Leave work staged or
  unstaged so `git diff` shows exactly what would land.  He is finicky
  about code in this repository, and that is a feature.  (Sole
  exception: this notes folder may be updated freely.)
- **Run the full testsuite before proposing any engine change**:
  `cd tests/testsuite && ./runall.sh` must end `ALL PASSED`.
  Header-CSS-affecting changes require regenerating `osis_basic.good`.
- **A released versification system is immutable.**  The v11n book and
  verse-max tables drive the byte-offset index files of every module
  built against them; changing them shifts offsets and silently corrupts
  every deployed module.  Only the av11n *mappings* may be corrected
  after release -- they affect translateVerse, not module indexing.
- The client owns stateful-object threading, not the library.  No
  engine-wide locks.  Stateful use across threads = clone() or
  per-object serialization by the caller.  See SWModule::clone(),
  the Android bindings' manager carousel, and the single process-wide
  FileOps with mutable mounts.

## Architecture notes (non-obvious)

- **av11n mapping blob format** (parsed in
  VersificationMgr::System::loadFromSBook): first extra-book names as
  NUL-separated strings ended by an empty string (a lone 0 byte even
  when there are none); then 7-byte rules
  `{book, toChap, toVerseMin, toVerseMax, fromChap, fromVerseMin,
  fromVerseMax}`, with an 8th byte (other-book id) consumed when
  rule book > system book count; ended by a 0 book byte.  The System
  stores POINTERS into the caller's blob -- blobs must outlive it.
- **Versification registry is first-wins and immortal**: registered
  Systems are never replaced or deleted (VerseKey holds raw System*),
  so re-registration is a no-op and re-inits are safe.  XML systems:
  registerVersificationSystemFromXML (VMR schema); SWMgr auto-registers
  a module's DataPath v11n.xml under the module's own name.
- **SWBuf under C++20**: it has an implicit `operator const char *()`.
  Before the member operator<=> was added, std::pair/std::tuple
  synthesized comparisons satisfied themselves through that conversion
  and compared buffer POINTERS.  Direct `a < b` always used the member.
  Beware this pattern anywhere else implicit conversions meet
  synthesized operators.
- **" I" book names**: the verse parser treats a trailing roman numeral
  as a chapter/book number, so locale book names ending in " I"
  (Hungarian "Mozes I", Latin "... I") cannot round-trip.  Inherent to
  the parser by Troy's ruling (July 2026); fix belongs in locale data.
- The in-tree build is autotools (`make` at top level).  After editing
  tests/Makefile.am run `./config.status tests/Makefile`.  Hand-linking
  a test needs `lib/.libs/libsword.a -licuuc -licui18n -licudata
  -lclucene-core -lclucene-shared -lz -lcurl -lpthread`.
- configure.ac computes a version ordinal via `git rev-list --count`
  (svn-rev-like: orders and measures; only comparable between full
  clones).  Cross-repo port markers (crosswire-java) carry both that
  ordinal and the sha.

## State of play (2026-08-25)

- Release prep for the first engine release since 1.9.0 (Nov 2020).
  Done toward it: SWModule::clone; Android SAF FileOps + threading
  rework; InstallMgr https-first/ftp-fallback fix + mock-transport
  testsuite; dynamic versification (XML + osis2mod/imp2vs -v file.xml);
  SWBuf operator<=>; zlib hard requirement; SWMgr path-traversal guard.
- **API-261 open**: Fr Cyrille's versification_mappings_API261_v2.patch
  (Synodal corrections + new LXX/Catholic/Catholic2 mappings).  Review
  flags: (1) his comment claims rule byte order
  `[from_book, from_ch, from_v, from_v_end, to_ch, to_v, to_v_end]` --
  the engine's actual order is to-then-from; decode and verify, do not
  trust the comment or the data blindly.  (2) canon_catholic fragment
  says vm_catholic verse counts "require fixes" -- that would be a
  forbidden v11n change; the mappings may reference verses the deployed
  v11n lacks.  (3) The patch is malformed (new-file diffs of fragment
  content); manual integration + register-call wiring needed.
  Plan: decode old/new mapping tables, diff, turn the issue's `==`
  correspondences into translateVerse testsuite vectors.
- MOD-474 open: psalm-title verse-number line-break rendering (filters).
- **Mapping-authoring improvements**, Java-first per Troy (crosswire-java
  is the working base to pull from): osisID accepted in map book/
  fromOtherBook attributes (done in Java; C++ port pending review).
  Agreed next: decode-blob-to-named-VerseMap at load in C++,
  a blob<->XML dump tool (also the API-261 review instrument), and a
  test pinning the last-book >= vs > extra-book check shared by both
  implementations.  Real-world mapping example:
  ~/src/vmrcre/contrib/v11ns/MTNU.xml (139 rules, MT<->intermediate);
  the whole VMR v11n family lives in that directory.
  NOTE: C++ pivots mappings through KJV/KJVA, Java through LXXNU --
  unreconciled divergence, Troy to rule which is canonical.
- sword-devel archive mirrored at ~/src/attic/sword-devel-archive
  (monthly .txt.gz, zgrep-able); VMR v11n XML:
  https://coptot.manuscriptroom.com/community/vmr/api/metadata/v11n/get/?format=xml&detail=chapter
