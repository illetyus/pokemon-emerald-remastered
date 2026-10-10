# Production source/data package preparation

One command generates World, Render, Characters and Environment through their
unchanged owners, checks the native R9/R14/R15 catalogs, and seals every produced
file plus source/generator provenance before publishing the output directory.
This is source/data preparation, not Unreal compilation, imports, APK or runtime.

From a complete clean checkout with DEPENDENCIES.md tools:

```sh
python tools/build_production_package.py --output build/production/Generated --receipt build/r20-generation-receipt.json
```

The directories preserve existing formats: World has 518 maps / 441 layouts
and authoritative scripts/encounters/provenance; Render has 75 exact source
tilesets; Characters/Environment contain complete identity/audit/fallback data.
Optional modern models/audio/imports remain private, separately documented inputs.

Generation requires the exact source pin and accepted clean vendor Git tree.
A fresh isolated stage holds all conversion/audit/check work. Failed conversion
or sealing never publishes partial output; existing or concurrently created
output is preserved. Source/core/vendor and symlink output destinations fail.
Inside the checkout use ignored build/out/local/generated subdirectories or
unreal/Content/Generated; elsewhere select a new explicit destination.

## Trusted whole-package verification

production-index.json is canonical UTF-8/LF JSON: complete relative file SHA-256
inventory (excluding the index itself), package digest, source pin/vendor tree,
complete conservative source-input inventory and input digest. Inputs are
tracked vendor/vanillaplus, tools, data, external, core, unreal/Source and Config.
This records a conservative source boundary, not a minimal dependency graph.
Source working changes or source/generator drift are rejected. No absolute host
path, timestamp, pointer, credential, save field or actual-build claim is stored.

Keep the generation receipt outside the package in a trusted local/evidence
location. Verification requires its index_sha256 explicitly:

```sh
python tools/build_production_package.py --output build/production/Generated --verify --expected-index-sha256 <hash-from-trusted-generation-receipt> --receipt build/r20-verification-receipt.json
```

Do not recompute that trusted checksum from an untrusted received index and
treat it as proof. A changed payload plus rewritten adjacent index fails the
external witness check. Missing/extra/corrupt files, noncanonical or unsupported
index, unsafe paths, symlinks and source/input provenance changes fail.
Receipt destinations are fresh, outside the package and never overwrite a
private file. Generation and verification print compact evidence with actual
file/input counts and false actual_unreal_build.

R20 CI runs transaction/integrity negatives, actual complete generation and
verification against its separately retained generation receipt. No generated
commercial payload is uploaded. R19 separately runs the actual full native/
Python/asset/source suite. Tiny unit fixtures test isolation/integrity semantics;
actual source corpus evidence comes from the default hosted generation.
Clean-repeat proof is T1; leak/security is I6 and actual content staging/preflight
is I7. Cross-toolchain/device/import quality is separately measured at R18/runtime.

The aggregate R20 default now runs all R19 checks, two independent clean Git
worktree productions, external-witness verification, nine real negatives and
complete source security/release checks. Command and metadata evidence:
[R20_REPRODUCTION_CONTRACT.md](R20_REPRODUCTION_CONTRACT.md).
