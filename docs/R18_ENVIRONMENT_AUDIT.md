# R18-P1 — Actual environment prerequisite audit

State: **EXTERNAL_ENV_REQUIRED / BLOCKED**.
Observed: 2026-10-10 12:33:58 Europe/Istanbul.
Branch: `r18-real-unreal-build`, based on verified main `466aa1821c0959a93e04251e46f9d0f551c5b2ce`.
This is a versioned read-only environment audit, not completion of R18-P1/P2,
installation, compile, cook, package, launch or runtime acceptance.

## Verified recovery and R20 integration

Live main/branch, recent commits, compare, open PRs, exact-main CI and canonical
ROADMAP/ARCHITECTURE/PHASE_EXECUTION_PLAN plus active Unreal source/build/config
files were reread. The new phase branch started 0 ahead / 0 behind main.
No concurrent conflicting drift was present.

R20-M1 PR #24 merged `56e14d744fe4ec933a0da98559190c6c98ac42b1` into main
`466aa1821c0959a93e04251e46f9d0f551c5b2ce`. Accepted/main tree is
`dfad5d246e039d8547dcbf461cd01fb7fbd2277c`, identical to F1's reviewed tree.
R20-M2 is VERIFIED_COMPLETE: all 17 exact-main workflows / 30 check runs reached
terminal success, including four CodeQL language checks. Actual main R20 workflow
[38039922255](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/38039922255)
/ job 114177936315 and independent R19 workflow
[38039922248](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/38039922248)
/ job 114177936279 passed 81 CTest / 453 Python / 171 asset audits and 53 R20 tests.
Two real clean productions matched 2,473 files / 13,096 inputs, trusted index
fd21e4c4b5b8a6250adad87c874a59edd93dd22d518d07e74f8299df750458b7;
all nine corruption/drift cases rejected and restored verification passed.
Security: 13,383 tracked files / 13,140 reachable blobs, zero findings/skips.
R20 source gates and PR/main integration are complete; actual-engine obligations
were not closed by source acceptance. See R20_PRODUCTION_COMPLETION.md.

## Current selected execution environment

The attached managed instance is cloud Linux x86_64, not the project PC.
Fresh runtime configuration observations are current and the instance is running/
connected. No project-PC capability/access, runtime variable, secret binding or
outbound identity is configured. This is not a request for renewed Git permission.

Local safe presence probes, without printing environment/credential values:

| Prerequisite | Observed state |
| --- | --- |
| UE_ROOT selector | Not configured |
| UE Build.version at configured/known engine roots | 0 present |
| ANDROID_SDK_ROOT / ANDROID_HOME | Not configured |
| ANDROID_NDK_ROOT / NDKROOT | Not configured |
| JAVA_HOME | Not configured |
| adb / sdkmanager / javac | Not on PATH |
| Java runtime | Present; does not establish a JDK/toolchain |
| CMake / CTest / clang | Not on PATH in this cloud cache |
| C/C++ compilers, FFmpeg, ffprobe | Present |
| Current cloud workspace free storage | 26683912192 bytes; not project-PC capacity evidence |

No accessible verified licensed UE 5.8.3 installation or Android toolchain was
provided. The probe covers selected prerequisites/known installation roots; it
does not claim an exhaustive search of another computer. Project-PC OS/storage/
tool access and exact official-engine toolchain compatibility cannot be verified
from this instance. No installer, manual self-hosted workflow, engine build,
Android package or device command was started.

The 5.8 uproject association, source-release pass, fake-installation unit tests
and hosted portable/source CI are not actual-engine proof. The engine/preflight
metadata presence gate is documented in R20_RELEASE_PREPARATION.md; it still
cannot substitute for real R18 compile/cook/ARM64 APK evidence.

## Required resume and STOP

The execution plan requires actual project-PC control at R18-P1; when unavailable,
it explicitly requires EXTERNAL_ENV_REQUIRED here. R18-P1 remains BLOCKED;
R18-P2/P3/I1-I5/G1/D1/F1/M1/M2 are NOT_STARTED. This audit's source-only CI, even
if green, must not promote the actual R18 gate.

Resume R18-P1 only after the actual project-PC/UE/Android environment is attached
and accessible to the executing agent. Re-read live refs/PR/CI/docs, audit the
real OS/storage/tool access, then verify exactly UE 5.8.3 and the SDK/NDK/JDK
requirements of that actual installation. Do not guess compatible versions,
reuse stale readiness, or upload engine/APK/private commercial content/secrets.

After actual R18 acceptance and main terminal-success, STOP at
REAL UNREAL RUNTIME VALIDATION. Physical Android smoke, BrowserStack, final
device matrix, R21 and R22 remain outside automatic authorization. The current
STOP is earlier, at the missing R18-P1 environment. Do not merge this phase as
VERIFIED_COMPLETE while its real prerequisite/build gate is blocked.
