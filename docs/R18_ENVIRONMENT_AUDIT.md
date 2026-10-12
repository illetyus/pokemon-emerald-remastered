# R18 build evidence reconciliation — 2026-10-13

State: **ACTUAL_WIN64_ANDROID_COMPILE_COOK_APK_INTEGRITY_VERIFIED; F1_REVIEW_MAIN_PENDING**.
Accepted compiled source is `fb0bcd57673c059c422dc8596cf1af7dbf945e69`.
This documentation checkpoint changes no production compile input; each of the
three actual build captures has 162 input files, whose bytes must match this
branch and the resulting main revision. The APK remains bound to its compiled
source and private content profile, not to a later documentation commit.

| Acceptance layer | Actual evidence and limits |
| --- | --- |
| Win64 Development Game and Editor | Both actual UHT/C++/link runs exited 0 at the compiled source above; stable input bytes. Game EXE 332975616 bytes, SHA256 `da4bc6065a0727e94c827955c88ee4977586169838dfd41d38fc98e0778827fe`; Editor module DLL 1967616 bytes, SHA256 `7b3a30bad274708ab1cddf7ce5c6b250f68acced260307024e99cddb9373642b`. |
| Android Development ARM64 | Actual native compile/link exited 0; ELF64 AArch64 DYN, four 16KB-aligned load segments. Raw shared library 444423088 bytes, SHA256 `c5363a197ede915912428564490c86d9c19e51c69de227924c81e80ddbb2efb9`. Compile used `-SkipDeploy`; Gradle packaging is separate evidence. |
| Android ASTC cook | Real cook completed 7703 shader jobs, 0 errors / 0 warnings. Successful cook reused for the corrected staging/package retry; it was not fabricated or rerun for the packaging-path closure. |
| Development APK | Actual stage/pak/package/archive exited 0. APK 175048890 bytes, SHA256 `6398e02c82bf6d1e070e886b5f2dc5f1e42f6a839fb35403c200d6cf8935d2b0`. SDK tools verified Android Debug v2 signature, ZIP CRC, package `com.illetyus.emeraldremaster.r0unreal`, minSDK26 / targetSDK35, ARM64-only native libraries, ZIP/ELF 16KB alignment. Main native GNU build ID `4b884bc7951a8a4f9795d3e1c74c7f36e50e1d40` matches the actual accepted native compile. |
| Packaged character content | Actual APK OBB was opened and its Pak/UToc/Ucas hashes matched staged data. All 14 required Brendan pilot assets, Vulkan/OpenGL shader libraries, three Characters profile files and the two packaged map/AssetSet config references were verified. This is a diagnostic pilot APK, not a playable-story acceptance. |
| Windows character runtime | Separate AP02 Windows cooked/visual/motion/fallback/static acceptance is bound to source `12f97e0d`; it is not a new Android runtime proof at the APK source. |
| Source tests / CI | Compiled source completed 31 checks / 17 workflows successfully. Its local Python run was 456 passed / 7 ERROR out of 463, all errors WinError1314 symlink privilege fixtures; no full local suite PASS or skipped-negative reinterpretation. The earlier all-green Windows preparation suite stays bound to historical `edd3469a`. This docs-only checkpoint and resulting main need their own terminal CI. |
| Device / broad gameplay | Not verified. No validated Android emulator was used for optional R18-I5 launch smoke; physical Android, first playable sequence and full story are separate later gates. |

The narrow Android Clang closure preserves first-actor selection, matches C
`uint64_t`/`int64_t` callbacks on LP64, and keeps the named UTF8 conversion alive
through its log call. No warning suppression, vendor edit or gameplay ownership
change was used.

The first signed APK was rejected because required cooked data was absent.
UAT had written OBB to a custom staging path while installed UEDeployAndroid
reads `Project/Saved/StagedBuilds/Android_ASTC.obb` (line4363). The successful
retry used the default project staging path with `-skipcook`, without editing
the engine. Failed APK/logs and historical false fields remain preserved.

SDK native API26, Java compile SDK36 and actual APK targetSDK35 are separate
selectors. Packaging used build-tools36.1.0; verification used36.0.0. Engine
UE5.8.3 CL58210709, NDK27.2.12479018 (r27c), JDK21.0.12.1+1, MSVC14.50.35739 and
Windows SDK10.0.26100.0 retain their actual build receipts.

The base Generated package was independently verified before the private
character override: index SHA256
`28c07f96902c6a57061c8d31e580f04df942253cf43a082e53e928ae68225ad5`,
2473 files / 13100 inputs. Three private Characters files were then separately
hash-verified from the actual APK. The original base index is not claimed to
certify those modified files or the additional private imported Unreal assets.

Private evidence stays under `local/Assets/AP02` and
`local/R18/Evidence/2026-10-12/ap03-*`; F1 reconciliation is under
`local/R18/Evidence/2026-10-13/ap03-f1-v1`. Source configs and original Generated
were restored; raw assets, source saves, APKs, binaries, keys and raw logs are
excluded from public Git. Pokemon-only outputs are under `D:/Pokemon Remaster`;
shared SDK/tool/cache roots remain under `D:/Android` (UE under `D:/Unreal`).

Next is independent F1 review, verified PR merge and exact-main CI/source-byte
reconciliation. Record R18-M2 at REAL UNREAL RUNTIME VALIDATION before any later
phase. The user has now authorized continuing the presented AP04+ plan after
that boundary; this does not turn technical APK proof into gameplay/device or
release-distribution proof. Later device/content/release gates remain required.

The following entries are immutable historical checkpoints at their own source
revisions; their old CookVerified/APK/runtime false fields are not retroactively
changed into this newer artifact's acceptance.

---

# R18-I1 — actual Win64 Game build accepted, 2026-10-12

State: **WIN64_GAME_UHT_CPP_LINK_VERIFIED**. Overall I1 still requires the
Editor host and Android ARM64 compile; cook/APK/runtime remain unverified.
Actual clean compiled source HEAD:047e97df20bef3fc0f8aec25d8ea022e53ac1568.
Its exact-head CI completed31 checks and17 workflows successfully. Actual
installed UE5.8.3 CL58210709 Build.bat produced the Win64 Development Game
with MSVC14.50.35739 (family directory14.50.35717) and SDK10.0.26100.0.
UHT, runtime C++ and real executable link all completed successfully, exit0;
UBT target receipt identifies Game / Development / x64. No runtime was executed.

Produced unreal/Binaries/Win64/PokemonEmeraldRemastered.exe is332961280 bytes,
PE AMD64(0x8664), SHA-256
0efaaa29a400938230f221e82f43e1e9e96728e68aa364f86a53b74928865d6d.
The external Generated verification was checked immediately before compile:
2473 files/13097 source inputs, index SHA-256
5e61036f36fdd0b798872a3f4566826366d59933dbe50466de89cb8129e7783e;
payload SHA-256 remains identical to the two accepted clean Windows productions.
The actual target, project HEAD, engine, tool selectors, CI, index, log/exit and
EXE hash/PE/timestamps are bound in the private artifact-acceptance receipt.
UBA bookkeeping was explicitly on D:/Android/Caches/UnrealBuildAccelerator;
compile/link actions ran NoUba with one parallel action. Vendor tree is unchanged.

C1's real compiler failures were closed narrowly: correct streamable struct
forward declarations, non-shadowing static-mesh local, existing authoritative
core catalog prebuild, separate portable C translation units, equivalent8-bit
mask, correct ticker handle type, and protected focus initialization inside a
minimal UButton subclass constructor. Existing behavior/ownership is preserved.
Source architecture guards and41 relevant regressions passed. This Game build
is additional actual UE evidence; the earlier81/455/171/55 full Windows source
suite remains bound to preparation HEADedd3469a, with its source receipts kept.

Private current Game evidence: local/R18/Evidence/2026-10-12/
unreal-i1-final-3470dbc492d84c228b00685acbf479c2. Failed/exploratory logs are
retained separately under c1-* and earlier unreal-i1-* paths. Generated content,
EXE and raw private evidence stay outside Git. The following documentation-only
checkpoint retains all production input trees and requires its own exact CI.

Next: actual Win64 Editor host compile, Android ARM64 compile, then cook and
Development APK plus artifact integrity. Main stays draft/unmerged. After fully
accepted R18 and exact-main CI, stop at REAL UNREAL RUNTIME VALIDATION;
physical Android/BrowserStack/R21/R22 do not start automatically.

---
# R18-C1 — protected button constructor closure, 2026-10-12

State: **CLEAN_WIN64_RETRY_REQUIRED**. Final retry on
 d79235c632ccab1054c95dbcc7b16ab4d6f196f5 passed UHT and the corrected ticker,
controller and UI subsystem compilation. It then correctly rejected direct calls
to UButton::InitIsFocusable because the installed engine declares it protected.
The earlier initializer-only candidate was insufficient and is not accepted.

A small runtime URemasterUiButton subclass now calls the protected initializer
from its ObjectInitializer constructor, as the engine header requires, before
Slate construction. Both existing row/back buttons instantiate that class; their
UButton references, click delegates, navigation/focus behavior and sizes remain.
No deprecated field access, warning suppression, engine edit or gameplay change
is used. The10 existing UI regressions and source architecture guard pass again.
The existing C1 coverage remains41 relevant tests; this compile retry will bind
its own clean source HEAD, exact-head CI and fresh external trusted source index.

The prior d79235c6 candidate completed31 checks/17 workflows successfully and
had externally verified2473-file/13097-input source content with unchanged payload
hash; it remains historical after this two-file source change. Actual final
Win64 Game/Editor/Android compile, cook/APK/runtime and main merge are not yet
accepted. Private C1/final-run evidence is retained under local/R18/Evidence/
2026-10-12; current-path pointers identify each exact run.

---
# R18-C1 follow-up — ticker and button API closure, 2026-10-12

State: **FINAL_WIN64_RETRY_REQUIRED**. The first C1 candidate
0e829a3d252ac09aab4a90bcd8d6f9956f44ba47 passed31 checks/17 workflows.
The real separate-file Win64 compile built the portable core and remaining
runtime sources, then exposed two additional engine API mismatches: the ticker
uses FTSTicker::FDelegateHandle, and UButton uses InitIsFocusable before its
Slate widget is constructed. These are closed by separating the ticker handle
from general application delegates, explicitly including Containers/Ticker.h,
and replacing only the two button initialization calls. Input lifecycle fences,
repeat policy, gameplay ownership and unfocusable button behavior are preserved.
UHT must regenerate the changed header before the final retry; exploratory
output while that header changed is not accepted as a release build.

In the full compiler environment18 input/lifecycle regressions and10 UI
regressions passed with zero skips. With the prior8 battle source and5 native
regressions, the C1 closure has41 relevant passing tests. Initial input probe
attempts lacked the complete compiler environment and remain as failed evidence;
no missing compiler or native symlink test is called a pass. Architecture guards
also pass. CoreCatalogs match the accepted native CMake outputs, and real UBT
prebuild logs confirm the source producers run. Vendor content is unchanged.

A source package was generated and externally verified for first C1 HEAD0e829a3d:
2473 files/13097 inputs, indexf39bcba7c73d6a6e79005f32e93916707c2fda5f921e0181b6c92f7c55d1c71b,
with payload hash unchanged from the two prior clean Windows productions. That
index is historical after this follow-up source change; a fresh production
output/external receipt and this checkpoint's exact-head CI must bind the final
clean Win64 retry. Prior Generated packages are preserved in ignored local/R18.
No final Unreal compile/cook/APK/runtime proof or main merge is claimed yet.
Private evidence: local/R18/Evidence/2026-10-12/c1-* (current-path pointer).

---
# R18-C1 — real Win64 compiler closure in progress, 2026-10-12

State: **ACTUAL_UNREAL_COMPILER_CLOSURE_IN_PROGRESS**. The source preparation
checkpoint931787ba963c984fcaf26e2db04d9d38b9aa460d passed31 checks/17 workflows,
and preserves accepted Windows source/package input trees from edd3469a.
Actual UE5.8.3 CL58210709 Win64 Development UHT passed. Real MSVC14.50.35739
C++ then failed at FStreamableHandle class/struct mismatch, a shadowed Mesh
local and missing authoritative trainer include. A later real compile exposed
unity collisions between portable C static helpers/enumerators and per-file
JSON helpers, plus the same streamable forward declaration in the character
component and a truncating constant cast warning promoted to an error.

The candidate fix uses the engine's struct declaration in both affected headers,
renames only the static-mesh local, and disables unity for this runtime module
so each portable C embed/local helper keeps its translation-unit boundary.
The bit-clear mask is expressed as the same explicit8-bit0xfd value; gameplay
ownership and behavior are unchanged. Game and Editor targets now run a shared
prebuild helper invoking the existing authoritative R11/R12/R13 generators into
ignored unreal/Intermediate/CoreCatalogs, added to module private include paths.
All three produced includes matched the accepted native CMake suite byte hashes.
The actual UBT log confirms the new prebuild commands ran.

Relevant source architecture validation and8 battle source regressions passed.
A fresh MSVC build passed5 meaningful existing native tests: C++ embed, Pokémon
box codec, party/storage, bag and domain catalog, zero skips. A separate full
24-test battle probe attempt had23 passes and one native symlink privilege error;
that non-elevated failure is retained and is not called an audit pass. Previously
accepted full Windows171 asset audits ran under native elevation on edd3469a.
The current separate-file real UE compile is still running; it is exploratory,
not final clean-HEAD build acceptance. This candidate needs its own exact-head
CI and fresh source-index binding before a final accepted Win64 retry.

The first UE launch selected UBA local bookkeeping under C: despite NoUBA; it
was stopped without acceptance. The retry explicitly selects
D:/Android/Caches/UnrealBuildAccelerator, confirmed by actual UBT logs; NoUba
compile actions remain enabled. Existing shared engine/user-state links stay.
No engine/vendor patch, warning suppression, private payload commit, cook/APK,
device/runtime proof or main merge is included. Private evidence is under
local/R18/Evidence/2026-10-12/c1-* and unreal-i1-* (current-path pointers).

---
# R18-I1 preparation — native Windows source acceptance, 2026-10-12

State: **WINDOWS_SOURCE_AND_GENERATED_PACKAGE_VERIFIED**.
Actual project source HEAD: edd3469adaa8ff9ac368a767f6fca67d91ca1cab.
Its exact-head CI completed31/31 checks and17/17 workflows successfully.
Fresh native Windows run windows-source-4d12e38a11d6431891af5a7929adca8c
completed the actual ordered R20 CLI including the real R19 CLI, exit0.
Native symlink negatives ran with approved UAC through signed system PowerShell.

All seven R19 and five R20 components passed. Coverage was81 native CTests,
455 Python regressions,171 asset-audit tests and55 targeted R20 tests, with
zero skips. Four metadata owners each completed two matching generations;
all12 corruption cases were rejected and restored data verified. Two fresh
independent detached worktrees produced matching2473-file packages with13096
source inputs. All nine package negatives were rejected, including altered
owners, missing/extra files, rewritten trusted index, real symlink and generator
source drift; restored package verification passed. Public source/history
security scanned13384 tracked files and13179 blobs, with zero findings and
zero skipped blobs. The immutable vendor tree remains
5a551f1f9e40184278c57dfb8d25f68a0a1c99dc.

Fresh ignored unreal/Content/Generated was generated and independently verified
using an external trusted generation receipt. It matches both clean packages:
index SHA-256 d480ace0ac3ed581683a7435456be354e330d98f57e60e30f8d3993f611223ee;
package SHA-256 f8b0f10b699829717f751a2ae0c7dd26fe1bdeca6b2a5afb25c03ff73cf11a2b.
Receipts' source_commit remains the pinned Vanilla+ input identity
70db90c9077aed1272e746fc2537d9f12b95a91c; invocation/source-acceptance records
bind these results to the actual project HEAD above. No raw evidence or generated
payload is committed. The subsequent documentation checkpoint must retain the
same production input trees and pass its own exact-head CI.

Private receipts/log/JUnit: local/R18/Evidence/2026-10-12/
windows-source-4d12e38a11d6431891af5a7929adca8c. Current-path pointers identify
the source run and external generation/verification receipts.

Next is actual R18-I1 Unreal Game/Editor Win64 and Android ARM64 UHT/C++/link.
Actual Unreal compile, cook, APK and device/runtime verification remain false;
source/fallback content acceptance does not certify modern asset import or
quality. Main is not merged. After accepted actual R18 and exact-main CI, stop
at REAL UNREAL RUNTIME VALIDATION; physical Android/BrowserStack/R21/R22 do
not start automatically.

---
# R18-I1 preparation — bounded full-source timeout closure, 2026-10-12

State: **FULL_WINDOWS_SOURCE_RETRY_REQUIRED**. Source HEAD
10bf06cb94ca49a4210d0fca158eb42d2218631e passed all31 checks/17 workflows.
Its fresh elevated Windows run built with MSVC19.50.35739 and passed81 CTests
and454 Python tests with zero skips. R6 asset audit also passed52 tests; the
run ended during R7 asset audits at the R20 parent's fixed1200-second budget.
Exit1 and no R19/R20 acceptance receipt were retained. The native symlink
cases ran with approved UAC elevation; no security-policy change was needed.

The aggregate parent budget is now bounded at3600 seconds, covering native,
Python, asset and metadata stages on slower Windows hosts. Existing individual
stage timeouts, required coverage, zero-skip checks, corruption rejection and
receipt validation remain enforced. A timeout now reports the failing component
and elapsed limit explicitly, without creating an acceptance receipt. The new
negative regression reproduced RED before this diagnostic fix. All11 focused runner regressions passed afterwards with zero skips;
the55 related regressions await native UAC symlink execution. Vendor content is unchanged.

Private failed-run evidence: local/R18/Evidence/2026-10-12/
windows-source-d1cc993460be4bbdb251d8f063023b5d. RED/GREEN closure evidence:
local/R18/Evidence/2026-10-12/i1-prep-5-* (current-path pointer retained).
Next: this checkpoint's exact-HEAD terminal-success CI, then a fresh elevated
ordered R20 CLI including real R19 and two clean production worktrees.
Full Windows source acceptance, Unreal compile/cook/APK and main merge remain
unverified. REAL UNREAL RUNTIME VALIDATION remains the post-R18 stop.

---
# R18-I1 preparation — R15 owner identity closure, 2026-10-12

State: **METADATA_INTEGRITY_VERIFIED**, full Windows source acceptance pending.
Baseline f6ad25b58b34ff54f29f7a6104927b6278db57db has29/31 successful checks;
R19 full-suite and R20 source-packages reject R15 owner source drift. The
previous Windows catalog path fix changed build_r15_audio_catalog.py, while
its existing package-integrity Git blob ownership pin still named the old file.
The real validator reproduced the same source-drift rejection locally.

The only integrity change refreshes that exact R15 owner Git blob from
fd9a933becba77f7c7c8bd6df4407b83d1b6f63b to
75a89998b945636be0b89a2b30d0de9ee7678665. Source-drift rejection, expected
file coverage, corruption detection and committed generated catalog comparison
are preserved; no generated source, private asset or vendor file was changed.

The actual Windows metadata integrity CLI passed all four owners: R6, R7,
R14 and R15. Each completed two matching generations; all12 file-corruption
cases were rejected (3/5/2/2), restored bytes verified, exit0. These are real
source metadata package results, not Unreal cook/import/runtime acceptance.
Private RED/GREEN evidence is under local/R18/Evidence/2026-10-12/i1-prep-4-*;
the support evidence pointer identifies the exact run.

Next: exact-HEAD CI terminal-success, then a fresh clean elevated native Windows
R20 full CLI including its real R19 CLI, Python/asset/security audits and two
complete clean production worktrees. Real symlink cases remain enabled; no
Developer Mode/security policy change is made. Verified production content for
Unreal is generated only into a fresh destination with an external trusted hash.
No full Windows source pass, actual Unreal compile/cook/ARM64 APK, device/runtime
proof or main merge is claimed. The final R18 stop remains REAL UNREAL RUNTIME
VALIDATION; physical Android/BrowserStack/R21/R22 do not start automatically.

---
# R18-I1 preparation — Windows manifest fixture byte closure, 2026-10-12

State: **FOCUSED_WINDOWS_REGRESSION_VERIFIED**; full source acceptance pending.
The local repository is now D:\Pokemon Remaster. Shared tool/SDK installations
remain under D:\Android. Historical pre-move evidence retains its original
paths; the relocation receipt records the current mapping.

On baseline HEAD0278567bb543781e7e7bdae12a0388b46251be8d the exact R6
manifest negative regression failed again on Windows: write_text(raw) converted
LF to CRLF, while the replacement byte hash used raw.encode(). The validator
correctly rejected the actual staged bytes before reaching the intended
canonical-content rejection. This is a test fixture transport mismatch.

The one-line test-only fix writes raw.encode('utf-8') with write_bytes().
Both existing byte-hash and canonical-content rejection assertions remain.
All15 relevant R6 presentation closure tests passed, failures=0, skipped=0.
Production validators, source/gameplay/assets and vendor content are unchanged.
The changed test's existing R19 ownership Git blob pin was refreshed to its
exact new bytes after CI reproduced R6 audit source drift. All6 ownership
regressions passed with zero skips; source-drift rejection remains enforced.

Baseline CI was inspected live:29 of31 checks succeeded; source-packages and
full-suite failed independently at package_integrity with R15 owner source drift.
Those logs are retained separately from the local RED/GREEN evidence. This
checkpoint does not claim exact-HEAD green CI or full Windows R19/R20 acceptance.
The R15 integrity closure and fresh ordered full suite belong to the next step.

Private RED/GREEN evidence is under local/R18/Evidence/2026-10-12/i1-prep-3-*;
i1-prep-3-current-path.txt identifies the exact run. No actual Unreal compile,
cook, APK, device/runtime proof or main merge is claimed. The post-R18 stop
remains REAL UNREAL RUNTIME VALIDATION.

---
# R18-I1 preparation — Windows audio catalog path closure

State: **SOURCE_REPRODUCTION_IN_PROGRESS**, observed 2026-10-11.
The previous preparation HEAD947b51697c5d4aae02300f4aa10ff3165eb00e2e
completed17 workflows /31 checks successfully. The fresh elevated Windows run
passed all81 native CTests and the real symlink negative fixtures, then stopped
at Python test114: source audio catalog paths contained Windows backslashes
instead of the existing checked-in canonical slash format. ExitCode1 and no
R19/R20 acceptance receipt were retained.

The minimum fix uses Path.as_posix() for the existing relative source-file
path. It changes no audio data, catalog identity, quality/import acceptance,
gameplay or vendor content. All7 actual SourceCoverage regressions passed,
including exact catalog/header reproduction, with zero skips. The whole-Python
probe is supplementary; a fresh clean ordered R20 run remains required after
this named checkpoint's exact-HEAD terminal-success CI.

Private failed-run evidence:
D:\Android\Evidence\R18\2026-10-11\windows-source-052eeca1af914195a40fc53243cc25e9.
Relevant green source regression: windows-audio-catalog-source-green.log in the
same date evidence root. Native UAC was approved; no global Developer Mode or
user security setting was changed. No full Windows source pass, actual Unreal
compile/cook/ARM64 APK, runtime or main merge is claimed. The final stop remains
REAL UNREAL RUNTIME VALIDATION after actual accepted R18 and exact-main CI.

---
# R18-I1 preparation — Windows script diagnostic path closure

State: **SOURCE_REPRODUCTION_IN_PROGRESS**, observed 2026-10-11.
P3 exact HEAD `9e4e6ff6c7fe16cba0464fcbd8714e68e68b8c5b` completed all
17 workflows / 31 checks successfully. Actual Windows R20 full-suite then
invoked the real R19 CLI on a fresh build/receipt destination.

The Windows MSVC 19.50.35739.0 native build completed and all **81 CTest cases
passed, failures=0, skipped=0** in its actual JUnit report. The ordered Python
component stopped at test19: an unclassified-script diagnostic used native
Windows backslashes while its existing contract requires canonical slash paths.
No R19/R20 full acceptance receipt was produced, and that failure was retained.

The minimum converter closure uses Path.as_posix() consistently in five existing
script error messages; classification, source data, dependency ownership and
command behavior are unchanged. The existing regression reproduced RED in the
actual Windows run; all **21 script converter tests passed** after the fix.
The accepted vendor tree remains unchanged.

An additional failfast Python probe with the diagnostic fix advanced to test84,
then hit **WinError1314** when its real path-escape/symlink rejection fixture
attempted to create a symbolic link. Windows requires native elevation for that
fixture; this is an environment permission boundary, not a validator failure or
a skipped test. No Developer Mode or global user security setting was changed.
A fresh full source run must execute with the needed privilege and zero skips.

Private baseline evidence:
`D:\Android\Evidence\R18\2026-10-11\windows-source-14cb64895f894dc6a3fc908ab3b4d51c`.
Relevant converter regression and the rejected Python privilege probe remain
in the same external evidence root. This named preparation closure requires
its own exact-HEAD terminal-success CI before the next full ordered source run.
It does not close actual I1 UHT/C++/link acceptance. No full Windows R19/R20
pass, production UE compile/cook/APK, runtime or main merge is claimed.
The post-R18 REAL UNREAL RUNTIME VALIDATION stop boundary remains.

---
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
