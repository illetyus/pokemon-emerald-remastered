# Unreal production runtime

Unreal hosts presentation, input, audio and platform services. The portable core
owns gameplay, save domains, world movement/collision, scripts, progression,
encounters, Pokémon/items and battle results.

## Engine and current evidence

The selected real engine is **UE 5.8.3**. The uproject's 5.8 association identifies
a family; it does not prove the installed patch. ARM64 Android is the primary
target. Verify SDK/NDK/JDK/UBT against that exact engine installation at R18;
source configuration values do not establish toolchain compatibility.

The completed main phases are portable/source preparation. No licensed UE/UHT
compile, imported modern model/audio readiness, rendered-frame acceptance,
cooked Android package, APK launch or device result is currently claimed.
[BUILD.md](../docs/BUILD.md) lists tools, complete checkout and private boundaries.

## Production content preparation

Use the complete clean repository; output and receipt must be fresh:

```sh
python tools/build_production_package.py --output build/production/Generated --receipt build/r20-generation-receipt.json
python tools/build_production_package.py --output build/production/Generated --verify --expected-index-sha256 <hash-from-trusted-generation-receipt> --receipt build/r20-verification-receipt.json
```

Keep the trusted receipt outside the package. World, Render, Characters and
Environment retain their owning formats; the index seals every file and source
input. Source/native R9/R14/R15 catalogs are checked by the entry point.
[Package contract](../docs/R20_PRODUCTION_PACKAGE.md) explains transactionality
and integrity. [Source staging and installation preflight](../docs/R20_RELEASE_PREPARATION.md)
closes R20-I7; actual compile/cook/package remains R18.

Default characters use visible engine basic shapes; environment identities use
source-backed R5 descriptors or explicit fallback. Optional private model/audio
bindings require exact provenance, normalization hashes and real editor import
validation. No private account or file is required for public source acceptance.

## Actual R18 gate

On the project PC, audit access and the exact engine/toolchain, compile the
production target, cook, package Android ARM64 and verify the actual APK.
A local PC/emulator launch may be a build smoke only within the validated
environment. Close real build failures through the normal source branch/PR gate.

If actual project-PC control is unavailable, R18-P1 stops EXTERNAL_ENV_REQUIRED.
After R18 acceptance and main CI, stop at REAL UNREAL RUNTIME VALIDATION.
Physical Android smoke, BrowserStack, final device matrix and releases are later
gates; they do not start automatically and are not substitutes for an R18 build.

## Remaining presentation obligations

R6/R7/R8/R9/R14/R15 evidence records actual imports, visuals, animation, audio
listening/loops, touch/lifecycle, device performance and host attachment that
source tests cannot certify. Story/script resource dispatch, battle request
ownership and naming/field-item hosts need their documented actual integration.
Do not mark those ready merely because a source validator or package passes.

Any future self-hosted Unreal runner must execute only trusted main/manual code,
with private payloads and APKs retained locally. SDL/Godot and
`experiments/unreal-r0-reference` preserve historical evidence.
[Roadmap](../docs/ROADMAP.md) and [architecture](../docs/ARCHITECTURE.md)
remain authoritative.

[R20 source completion](../docs/R20_PRODUCTION_COMPLETION.md) records actual
portable/source/reproduction/security acceptance. It does not close the real
engine, import, host or device obligations above.
