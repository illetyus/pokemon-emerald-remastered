# R18-P3 — Actual Android SDK validation and Windows preflight closure

State: **ANDROID_SDK_VALIDATED**, observed 2026-10-11.
P2 checkpoint `f4b5cfdbdce0f902c69d7123f2e3377720b6b2ef` reached exact-HEAD
terminal success in all 17 workflows / 31 checks before P3 execution.

The exact installed UE 5.8.3 tools ran actual Android validation:

- UBT ValidatePlatforms: Android VALID r27c; command exit 0.
- UAT Turnkey VerifySdk: **Android Status=Valid**, Current_Sdk=r27c,
  supported r27c–r29, InstalledSdk_ValidVersionExists, Support_FullSdk and
  Sdk_HasBestVersion; command exit 0. No invalid-host-prerequisite flag.
- Selected SDK root: `D:\Android\Sdk`; NDK root: its `ndk\27.2.12479018`.
- Selected JAVA_HOME: `D:\Android\Java\jdk-21.0.12.1+1`;
  actual java/javac commands exit 0, Temurin 21.0.12.1+1.
- Actual installation preflight exit 0, INSTALLATION_PREREQUISITES_PRESENT;
  it deliberately retains compatibility/build=false in its presence receipt.
- Android Studio Koala 2024.1.2 Patch 1 / distribution 2024.1.2.13,
  matching this engine's Turnkey manifest, is installed under
  `D:\Android\Tools\AndroidStudio2024.1.2.13\android-studio`.
  Official Google ZIP SHA256 matched
  `e36b2ba026032f10111d2a6bb895bb31628d8a1bc3ffe6718a67b20d4c211a0f`;
  studio64.exe signature Valid / Google LLC. Actual IDE startup is not claimed.

The initial UAT exit-0 result reported **Invalid** because Android Studio was
absent; it was rejected as acceptance. After the genuine verified IDE payload
was installed, a fresh report explicitly became Valid. Verification ran without
UpdateIfNeeded, forced SDK installation or any requested device operation.
UBT's Win64 VALID 10.0.22621.0 output is the engine's main/preferred SDK label,
not proof of a selected installed target compiler or SDK. I1 owns that proof.

SDK platforms 35/36/36.1, build-tools 35.0.1/36.0.0/36.1.0 and SDK CMake3.22.1
are present. Validation does not establish which compile platform/build-tools
or CMake invocation a final target will use; I1/I3 logs must record that choice.
Project intent stays ARM64-only, package com.illetyus.emeraldremaster.r0unreal,
MinSDK26 / TargetSDK35. Host CMake3.31.12 is separate from SDK CMake3.22.1.

The real Windows JDK release file uses CRLF. The bounded preflight parser
incorrectly rejected that metadata despite working java/javac. A regression
reproduced the failure before implementation; the minimum optional-CR parser
fix passed afterward. All **13 relevant release-preflight tests passed, zero
skips**. The Linux executable-bit rejection test retains actual chmod/access
coverage on POSIX; on Windows it explicitly probes the access rejection because
Windows chmod cannot express a POSIX executable bit. No test is skipped.

Private successful logs/receipt are under
`D:\Android\Evidence\R18\2026-10-11\p3-0ece1ad919ef4092a04cd53421d0a569`.
The probe used P2 HEAD plus the uncommitted minimal preflight regression fix;
this checkpoint commits that fix. No production content or gameplay changed.
Native Windows elevation created only a previously absent UAT XML-cache file
symlink into D:\Android. The normal engine cache generation/validation then
produced the real cache there. Fixed UBT, Turnkey and IDE detection paths use
previously absent junctions into D: payloads; existing user data is preserved.
No XML-cache override, fake IDE marker or engine-source patch was used.

This named P3 source/docs checkpoint requires exact-HEAD terminal-success CI.
Next is Windows R19/R20 source reproduction before I1 production compile.
No Windows full-suite acceptance, actual game compile, cook, APK, launch,
runtime/device proof or main merge is claimed. The post-R18 runtime STOP remains.

---
# R18-P2 — Exact installed Unreal and actual tool startup

State: **EXACT_ENGINE_AND_TOOL_STARTUP_VERIFIED**, observed 2026-10-11.
The preceding P1 checkpoint `4cf38a5f495c0135a21da65e09924dfa0b0441ec`
reached exact-HEAD terminal success: 17 workflows / 31 checks, all success.
P2 actual-tool evidence was produced from that clean source HEAD.

- Exact engine root: `D:\Unreal\UE_5.8.3\UE_5.8`.
- Build.version: UE 5.8.3, CL 58210709, compatible CL 55116800,
  promoted non-licensee build, branch `++UE5+Release-5.8`.
- Build.version SHA256:
  `eab58750e84719489f3ee0d42d05a533ceb357eb3ee511777e9408d27a85f43c`.
- Epic launcher engine item UE_5.8 agrees with the exact root and
  `5.8.3-58210709+++UE5+Release-5.8-Windows`; incomplete=false.
- Bundled Windows .NET actually executed: **10.0.203**, exit 0.
- Installed UnrealBuildTool.dll actually executed `-Help`, exit 0.
- Installed AutomationTool.dll actually executed `-Help`, exit 0.
- Windows batch entry points, installed build marker, UnrealEditor executable
  and Android platform components are present. No editor launch or target
  compilation is claimed by presence or help execution.

The initial UAT batch invocation rejected its wrapper-only
`-noturnkeyvariables` switch when it reached the AutomationTool parser (exit 2).
The verified invocation used the bundled dotnet executable and installed UAT
DLL directly, with no script-module compilation, and completed exit 0.
This invocation error did not require an engine or project source change.

Actual help logs and receipt remain private under
`D:\Android\Evidence\R18\2026-10-11\p2-35078d72681c42e28be373772aced609`.
Fresh per-invocation UAT log directories and process-local saved/cache selectors
use D:\Android. Previously absent fixed UBT AppData directories are junctions
into D:\Android\Caches\UEUserState; no existing user data was moved/replaced.
The existing Unreal 5.5 user directory is preserved.

P2 tool-startup acceptance is complete locally; this named docs checkpoint
requires its own exact-HEAD terminal-success CI before **R18-P3**.
P3 actual Android compatibility and I1 compile/I2 cook/I3 APK remain unverified.
After accepted R18 and exact-main CI, STOP at REAL UNREAL RUNTIME VALIDATION.

---
# R18-P1 — Actual Windows prerequisite audit

State: **LOCAL_PREREQUISITES_VERIFIED**. Named documentation checkpoint CI must
reach terminal success before R18-P2. This is not actual UE build acceptance.
Observed: 2026-10-11, Europe/Istanbul.
Audited branch baseline: `r18-real-unreal-build` at
`ed136002319e61dcc2ee0b2338a39bad20a8a2c5`; verified main remains
`466aa1821c0959a93e04251e46f9d0f551c5b2ce`. PR #25 remains open/draft.

The executing agent now has actual native Windows file/tool access. The old
cloud-only access boundary below is historical and superseded. Current user
execution authorization selects **D:\Android** for all new downloads, SDK/tool
payloads, sources, work and evidence, with an explicit exception only for
mandatory Microsoft system/installer components on C:. Existing licensed Unreal
installation is retained; engine verification is the separate R18-P2 checkpoint.

## Selected paths and source recovery

- Full-history base clone: `D:\Android\Sources\pokemon-emerald-remastered`.
- Isolated phase worktree: `D:\Android\Work\PokemonEmeraldRemastered`.
- Downloads / tools / caches / temporary files: the corresponding directories
  under `D:\Android`.
- Private evidence: `D:\Android\Evidence\R18\2026-10-11`.
- SDK: `D:\Android\Sdk`; JDK: `D:\Android\Java\jdk-21.0.12.1+1`.
- Existing engine candidate: `D:\Unreal\UE_5.8.3\UE_5.8`.

The clone is not shallow (1,318 reachable commits at audit time), CRLF conversion
is disabled, and the phase worktree was clean before this docs-only checkpoint.
The accepted `vendor/vanillaplus` tree is exactly
`5a551f1f9e40184278c57dfb8d25f68a0a1c99dc`; the pinned source commit remains
`70db90c9077aed1272e746fc2537d9f12b95a91c`. No vendor/gameplay/save changes occurred.
Live main, phase ref and PR state were checked again immediately before editing.

## Actual prerequisite evidence

| Item | Verified observation |
| --- | --- |
| Windows | Native Windows, OS build 26100; installation and compiler execution accessible |
| Physical RAM | 11.93 GiB at initial Windows API inspection; build scheduling needs a fresh capacity check |
| Free storage after setup | C: approximately 14.0 GiB; D: approximately 394.5 GiB |
| Git / Python | 2.55.0.windows.5 / 3.12.10, actual version commands exit 0 |
| CMake / CTest | 3.31.12 under Tools; publisher archive SHA256 matched; actual commands exit 0 |
| FFmpeg / ffprobe | 9.0.2 essentials under Tools; publisher archive SHA256 matched; actual commands exit 0 |
| Microsoft Build Tools | VS 2026 18.10.3, installation version 18.10.12224.181; installer exit 0; registered complete/launchable, no reboot required |
| Production compiler candidate | Actual cl.exe ProductVersion **14.50.35739.0**, FileVersion 19.50.35739.0; x64 host/target selected |
| Toolset family directory | **14.50.35717**; this directory name does not identify the compiler patch version |
| Windows SDK | Installer 10.1.26100.9457 exit 0; 10.0.26100.0 headers/libs under Tools/WindowsKits/10; registered KitsRoot10 selects D: |
| Native MSVC probes | C99/C11 designated initialization + Win32 header/link/execution PASS; C++17 optional/filesystem + execution PASS |
| Portable test compiler adapters | clang/clang++ 22.1.3 on Windows; cc/gcc and c++/g++ adapters compile/run the same C99/C++17 probes; these are not GCC installations |
| Java selection | Process-local JDK 21.0.12.1+1; java/javac commands exit 0; machine Java 17 is retained |
| Android SDK CMake / Ninja | SDK manager install exit 0; 3.22.1 / 1.10.2 actually executed |
| Existing Android components | API 35 and 36, build-tools 35.0.1 and 36.0.0, NDK 27.2.12479018; actual UE selection/compatibility belongs to P3 |

MSVC binary version 14.50.35739 is above the installed engine metadata's banned
14.50.0–14.50.35722 range. The engine's real UBT selection still needs verification;
installed metadata or standalone compiler probes cannot substitute for it.
The newer LLVM is for portable test command compatibility, not a claim that it
is the selected/supported Unreal production compiler. Later native source suites
must execute with zero skips; they have not run on this Windows worktree yet.

## Installation integrity and execution controls

Microsoft SDK/Build Tools bootstrapper signatures were Valid / Microsoft before
execution. Both installers completed exit 0. The official VS installer recorded
successful SHA256 verification for its own downloaded packages. A separately
fetched VS catalog did not match its channel's advertised SHA256, including one
fresh retry; it was not treated as trusted or supplied to installation, and no
verification was bypassed. Published CMake/FFmpeg archive hashes matched.

The local `Enter-R18Environment.ps1` selects the pinned MSVC family through the
installed `Common7/Tools/VsDevCmd.bat`, imports only public compiler selectors,
and scopes SDK/JDK, PATH, temp, Gradle, Python, NuGet/.NET and Unreal data caches
to this run. Global Java/Android PATH settings were not changed. Microsoft system
installer/shared components retain their mandatory C: locations under the user
exception; main SDK/tool payloads and VS package cache use the selected D: paths.
Private logs, installer results, source recovery, full version receipts, and the
MSVC/Clang native compiler probe receipts remain outside public tracking.

## Acceptance boundary and next checkpoint

R18-P1 local prerequisite preparation is verified; this commit's exact-HEAD CI
is a separate publication gate. After terminal-success, next is **R18-P2**.
R18-P2/P3/I1-I5/G1/D1/F1/M1/M2 remain NOT_STARTED at this P1 checkpoint.
No Unreal target compile, cook, ARM64 APK, launch, runtime/device acceptance or
Windows full source-suite pass is claimed by these setup/compiler checks.

After actual R18 acceptance and exact-main CI terminal-success, STOP at
**REAL UNREAL RUNTIME VALIDATION**. Do not automatically run physical Android
smoke, BrowserStack, final R19 device matrix, R21 or R22.

---

# Historical cloud audit — 2026-10-10 (superseded access boundary)
## Original R18-P1 cloud prerequisite audit

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
