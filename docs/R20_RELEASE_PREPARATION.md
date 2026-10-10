# R20 release source preparation

DefaultGame stages the four Generated owner directories: World, Render,
Characters and Environment. Entry is explicitly listed for cook; engine basic
shapes are retained for visible character/Pokémon fallbacks. Optional private
imports are not enabled or claimed ready. Existing application identity
com.illetyus.emeraldremaster.r0unreal, ARM64-only intent and Min/Target SDK 26/35
are preserved. These SDK settings require reconciliation with the exact real
engine/toolchain at R18. Shipping/signing/Play release acceptance is later work.

```sh
python tools/unreal_preflight.py --source-only --receipt build/r20-release-receipt.json
```

This checks bootstrap, owner staging, Entry/basic-shape cook intent, identity,
ABI and source engine association. It returns SOURCE_RELEASE_PREPARATION_PASS
with actual_unreal_build and toolchain_compatibility_verified both false.

On an actual installed host, use --engine-root and --host Linux or Win64.
For Android add --android and SDK/NDK/JDK roots via --sdk-root/--ndk-root/
--java-root or ANDROID_SDK_ROOT (ANDROID_HOME fallback), ANDROID_NDK_ROOT
(NDKROOT fallback) and JAVA_HOME. The Linux shell wrapper performs this strict
Android installation preflight. Missing roots/metadata/platform/SDK tools/NDK
compiler/JDK or non-executable Linux launchers fail. Build.version must name
exactly 5.8.3; 5.8 family/another patch cannot pass. Actual metadata hash and
present tool versions are recorded without workstation paths.

INSTALLATION_PREREQUISITES_PRESENT means presence/metadata only. It does not
execute the launchers, authenticate an engine installation, establish Android
toolchain compatibility, compile, cook or package. Exact SDK/NDK/JDK compatibility
and actual engine identity/build evidence remain R18-P2/P3 and I1-I4. No guessed
compatible versions are baked into this source check.

The existing manual Unreal workflow is trusted-main only. It audits installation,
generates/externally verifies production content in the expected runtime path,
then executes its real build/package commands if deliberately dispatched on a
configured private runner. No dispatch occurs in R20 and no APK/private payload
is uploaded. Outputs/receipts remain local. R18 must reconcile real errors and
APK integrity; presence checks and source tests cannot close that phase.

Twelve regressions use explicit temporary fake installations whose launchers
would fail if invoked. They verify wrong patch/types, missing/non-executable
launchers, each Android root/tool, source staging/cook drift and Linux/Win64
host requirements. They are source/negative evidence only.
