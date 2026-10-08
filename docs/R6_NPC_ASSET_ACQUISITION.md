# R6-I3B NPC / Trainer Asset Acquisition

Status: R6-I3B complete.

R6-I3B downloaded, integrity-checked, structurally inspected and stored the
unique ORAS packages selected by R6-I3A.

## Acquisition result

- unique packages: 43
- total raw ZIP bytes: 4101236
- ZIP integrity failures: 0
- private re-read SHA-256 mismatches: 0
- packages with skin controllers: 43/43
- packages containing actual embedded animation clips: 0

All 43 private ZIPs were read back after commit and SHA-256 checked against the
downloaded source bytes.

The public repository stores only package metadata and SHA-256 hashes. It does
not store the ZIP binaries or private repository paths.

## Package inspection corrections

### Swimmer

Asset 299535 contains one model whose archive path explicitly says
Swimmer female/rstr0113_00_fi.dae.

Therefore OBJ_EVENT_GFX_SWIMMER_F can use this package as its verified source
model. OBJ_EVENT_GFX_SWIMMER_M cannot; it returns to the owned-ORAS extraction
or secondary-source queue.

### Interviewers

Pinned Emerald source maps identify OBJ_EVENT_GFX_REPORTER_F as Gabby and
OBJ_EVENT_GFX_CAMERAMAN as Ty. OBJ_EVENT_GFX_REPORTER_M is an independent
generic male reporter.

ORAS asset 299514 contains only two models. It is therefore a candidate pair
for Gabby and Ty, not a three-way source for all reporter identities. Exact
submodel assignment inside the two-file archive remains pending.

### Liza & Tate

Asset 296325 contains two DAE models. The package is sufficient as a source
pair, but which DAE is Liza versus Tate remains deliberately unresolved.

### Triathlete

Asset 299537 contains two DAE models while Emerald has four graphics identities:
male/female running and male/female cycling. The two files are treated as base
character candidates; gender and run/cycle presentation mapping is deferred.

### Tuber Boy

Asset 299538 contains one rigged base model. TUBER_M_SWIMMING is not treated as
evidence for a second full model.

### Wally and Youngster

Wally asset 296336 contains two DAE variants. Youngster asset 299541 contains a
base DAE plus two alternate DAEs. Both require explicit variant selection.

## Animation result

None of the 43 packages provides an actual animation clip. Some include SMD
files, but inspected SMD data contains no animation frames beyond bind pose.
DAE files contain skin controllers but no embedded animation library.

## Next slice

R6-I3C - NPC model-family and submodel mapping.

Use the acquired package metadata plus source behavior to assign named
characters, trainer classes, generic NPC families, and unresolved extraction
requirements before manifest binding.
