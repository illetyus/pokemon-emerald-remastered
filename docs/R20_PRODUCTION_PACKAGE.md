# Production source/data package preparation

R20-I3 establishes one fresh generation entry point. I4 adds aggregate
hash/provenance verification; T1 proves clean reproducibility. None is an
Unreal compile, asset import, APK or rendered runtime result.

From a complete clean source checkout with the dependencies in DEPENDENCIES.md:

```sh
python tools/build_production_package.py --output build/production/Generated
```

The four directories retain their existing runtime owner formats:
World (518 maps / 441 layouts, scripts/encounters/source provenance),
Render (75 tilesets with exact source render payloads),
Characters (full identity/fallback/coverage metadata),
Environment (full source identity/audit/fallback metadata).
R9 UI, R14 battle and R15 audio native source catalogs are checked with their
existing --check commands; they are not copied private model/audio assets.

Source lock, exact accepted vendor Git tree and clean vendor working status
must pass. Each actual owner generator/auditor runs; a missing owner manifest,
failed source audit/check or conversion error fails the command. A complete
clean checkout is required: a hand-copied partial text/source cache is not one.

Generation uses a private fresh staging directory. Existing output is rejected,
including local private packages; failed conversion never publishes partial
output, and a destination created concurrently is preserved. Source/core/vendor
destinations and symlink ancestors are rejected. Inside the checkout use ignored
build/out/local/generated roots or unreal/Content/Generated; outside it select
a new explicit destination. Existing package update is not an implicit repair.

Default outputs contain no normalized modern meshes/audio. Source-derived render
payloads are local/ephemeral generated data, never public Git contents or
workflow uploads. Optional private imports remain separately documented and
ignored. Staging into the actual Unreal content tree/configuration is I7 work.

R20 Production Preparation runs the transaction tests and one actual complete
generation on GitHub. R19 separately runs the actual full native/Python/asset/
source suite. The isolated layout tests use explicit tiny synthetic owner
callbacks to test transaction behavior; those are not source-generation evidence.
