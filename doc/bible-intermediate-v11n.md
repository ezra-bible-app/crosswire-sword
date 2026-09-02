# "Bible": a Superset Intermediate Versification for av11n Mapping

*Draft proposal, August 2026 — Troy A. Griffitts, with drafting assistance.
For discussion on sword-devel before adoption.*

## The problem

SWORD's cross-versification mapping (av11n) translates references between
versification systems through a single intermediate ("pivot") system.
Today that pivot is a real, module-bearing versification -- KJV/KJVA in
the C++ engine, LXXNU in the crosswire-java implementation.  This has
three consequences:

1. **The pivot is frozen.**  A versification that modules have been
   built against can never change: its book and verse extents drive the
   byte-offset index files of every deployed module, and any change
   silently corrupts them all.  So the pivot cannot grow to represent
   verses that exist in other traditions (Synodal pluses, Vulgate
   numbering, the Ethiopian canon, ...).
2. **Mapping through it can lose information.**  Expansion and
   contraction are *correct and required* behavior -- real traditions
   split and join verses, and `translateVerse` answers with verse
   ranges so a parallel view driven from either side stays aligned
   (see examples/tasks/parallelBibles: `Vulgate KJV Ps.147` vs
   `KJV Vulgate Ps.147` across the Vulgate Ps 146+147 == KJV Ps 147
   seam).  The pivot's poverty is a different
   matter: where two source verses share one pivot slot because the
   pivot lacks the distinction, they cannot fan back out correctly on
   the far side.  A superset pivot removes the loss while range
   semantics remain exactly as they are.
3. **The two implementations diverged.**  C++ pivots through KJV/KJVA;
   the Java implementation and the VMR's v11n XML family (e.g. MTNU)
   are LXXNU-relative.  The same mapping concept, two incompatible
   coordinate systems.

## The proposal

Introduce one **reserved, synthetic, superset versification** used only
as the mapping intermediate and **never for building modules**.

Because no module is ever built against it, the immutability rule
relaxes: the system may be **extended** at any time.  Because it is a
superset -- containing every verse of every mapped tradition (within reason; excluding white elephants) -- mapping
through it loses nothing.

Precedent: Paratext's "Original" versification serves precisely this
role in that ecosystem -- a scheme no text is published in, used as the
universal mapping hub.

## Why extension is safe

Mapping data correlates books **by osisID** between systems; only
chapter and verse are numeric, and books absent from the reference side
travel by name through `extraBooks`.  No system's mapping data
references the intermediate's book *positions*.  Furthermore, since the
superset bears no modules, its internal book order is cosmetic --
nothing navigates it.

The invariants that must hold from one revision to the next are
therefore simply:

- an osisID, once present, is never removed or renamed;
- a book's `chapterMax` and each chapter's `verseMax` never decrease;
- existing chapters and verses are never logically reassigned meaning.

Additions -- new books, new trailing chapters, new trailing verses --
are always permitted.  A testsuite check comparing each revision of the
definition against the previous can enforce this mechanically.

## Seeding: LXXNU

The superset's first revision is an exact structural clone of **LXXNU**.

Rationale from the current data inventory:

- The Java/VMR ecosystem is already LXXNU-relative, and its live
  mapping data (MTNU, 139 rules) migrates for free.
- The C++ engine's KJVA-relative data is modest: Synodal (~193 blob
  lines -- being replaced by the API-261 corrections regardless), Vulg
  (~285), Calvin (~100), Segond (~112), DarbyFr (~34), NRSV (~6).
- LXX numbering is the nearer starting point for a scholarly superset.

## Migration plan

1. **Seed** the superset as an LXXNU clone under the reserved name.
   The definition lives as versioned XML;
   the C++ engine ships it as a generated built-in header.
2. **Build the KJVA <-> LXXNU bridge mapping once** (chiefly the Psalm
   numbering offsets, plus Kingdoms/Esdras/Daniel structure).  This is
   the only scholarly work in the migration, and is independently the
   most-wanted mapping in the ecosystem which we don't already have.
3. **Convert mechanically**: re-express Vulg, Calvin, Segond, DarbyFr,
   and NRSV rules through the bridge, by script.  This doesn't invalidate
   modules created by these v11n systems because only the mapping data
   if changed and compiled into the library.  Old libsword has the previous
   mapping logic compiled in and next rev libsword will have the proposed mapping
   compiled in.
4. **Re-author Synodal** directly against the new pivot from the
   API-261 correspondence list (human-readable), rather than converting
   existing SWORD data known to be wrong.
5. **Switch the pivot** in `translateVerse` (both implementations) from
   the hardcoded KJV/KJVA (C++) / LXXNU (Java) checks to the reserved
   name.
6. **Refuse module builds** against the reserved name in osis2mod and
   imp2vs (they already resolve -v through the engine, so this is a
   name check).

**Verification**: generate the complete verse-by-verse translation
table for every mapped system under the old pivot and again under the
new, and require identity -- including range results: one-to-many and
many-to-one correspondences (the Ps 146/147 seam is the canonical
vector, exercised both directions as in examples/tasks/parallelBibles)
must survive the migration byte-for-byte.  API-261's intended corrections are applied
as a separate, visible step -- never mixed into the migration diff.

## Compatibility

- Engines and their built-in mapping data ship together in a release;
  within one release the pivot and the data always agree.
- A module shipping its own v11n.xml with mappings should declare the
  pivot its rules target (a `mappings target="..."` attribute, default="Bible").  An
  engine that does not know that pivot ignores the mappings -- degraded
  to no cross-v11n translation -- instead of misapplying them.

## The name: `Bible`, one hub per corpus

The reserved name is **`Bible`** -- because the design generalizes: the
hub is *per corpus*, not global.  The VMR's v11n family already carries
`homer.xml` and `avestan.xml`; Iliad citation schemes and Avestan
editions have exactly the same mapping problem, and their hubs are
naturally named `Homer` and `Avestan`.  CrossWire prior art agrees:
sword-tools/versification/ organizes books into corpus collections as
`canon.bible.xml`, `canon.classical.xml`, `canon.qumran.xml`,
`canon.naghammadi.xml`, and friends.

The one hazard: `Versification=Bible` reads naturally enough that a
module maker might type it believing it a generic default.  The tool
guard makes that a teaching moment -- `osis2mod -v Bible` refuses with
"Bible is the mapping intermediate, not a module versification; give
the system your text actually follows."

## Verifying mappings: collate the free sources

Several projects publish verse-correspondence data.  Rather than trust
any one editor (ourselves included), collate them:

- **Paratext / ubsicap `.vrs`** files (`org.vrs`, `eng.vrs`, `lxx.vrs`,
  `vul.vrs`, and `rsc.vrs` -- Russian Synodal) -- machine-readable
  mapping lines, the ecosystem that has done this longest at scale.
- **Tyndale TVTMS** (and the older STEP data set it descends from,
  which diverges in instructive places) -- verse-level with stated
  methodology.
- **BiblIndex** (https://www.biblindex.org/api) -- reference systems
  grounded in patristic citation practice, valuable for being
  LXX/Vulgate-tradition-first.
- **CCEL** refsys tables.

Harness: parse each into a common form (tradition pair -> verse
correspondence set) and bucket every correspondence as **unanimous**
(adopt wholesale, cite all sources), **majority** (adopt, flagged), or
**conflict** (human review).  The same run validates the API-261
Synodal table against independent witnesses and generates the
KJVA <-> LXXNU bridge of migration step 2.  Mappings are near
uncopyrightable fact, but each adopted rule carries source attribution
in the XML comments regardless.

## Existing tooling to build on

- `tests/parsekey` already drives cross-v11n translation from the
  command line (`parsekey <ref> [locale] [v11n] ... [to_v11n]`) -- the
  verification harness's natural driver for full-table generation.
- The VMR serves v11n definitions at `vmr/api/metadata/v11n/get`
  (xml/json/csv/transcription; book- or chapter-level detail; subset
  filters) -- the `Bible` definition is published there, versioned,
  like the rest of the family. e.g.,
  https://ntvmr.uni-muenster.de/community/vmr/api/metadata/v11n/get/?format=xml&detail=chapter
- `sword-tools/versification/` holds the working toolbox: canon
  collection XMLs with schema (`canon.xsd`), `av11n.py` (guess a text's
  v11n from its osisIDs), `v11n_check.py`, `validate_v11n.py`,
  `v11nexport.pl`, and reference copies of the canon headers.  The
  collation harness and the append-only revision checker belong beside
  them.

