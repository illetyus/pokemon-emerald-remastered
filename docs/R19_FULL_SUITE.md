# R19 — One portable/source full-suite command

From a clean checkout with the pinned vendor source present, run:

```sh
python tools/r19_full_suite.py --build-dir build --receipt build/r19-full-suite-receipt.json
```

Required tools: Python 3 (3.12 hosted), C99/C++ compilers, CMake/CTest with JUnit
output support and FFmpeg/ffprobe. The command requires every dependency and sets
REMASTER_R15_REQUIRE_FFMPEG=1; an absent transform cannot become a skipped PASS.
The dedicated read-only R19 workflow installs FFmpeg and runs this exact command.
No Unreal installation, private save, commercial asset package, ROM or APK is
required for the source suite. See README.md and the owning phase documentation
for original source generation and package prerequisites.

The versioned data/r19/full_suite.json freezes 81 original native test names and
56 Python modules including this orchestration's regression module. Added tests
also execute; deleting/replacing a required owner is rejected. Python must actually
execute at least 400 tests without skip. Native execution uses actual compiled
CTest JSON discovery, full execution with no-tests=error and complete exact JUnit
name coverage. The source suite never accepts only a mock, listing or old receipt.

Seven ordered components execute in one process/command:

1. Configure/compile actual portable authority; run every discovered CTest.
   This includes all six R19 native components: ordered production replay,
   emitted WORLD checkpoint closure, three source story/world slices, complete
   R17 save orchestration, encounter seeds and battle/R16 seeds. Replay fixtures
   retain immutable expectations and first divergent snapshot/domain reporting.
2. Discover/run every Python regression; reject first failure, skip or short run.
3. Execute all 171 current R6/R7/R14/R15 source/synthetic asset audits.
4. Twice regenerate four metadata packages; verify all twelve hashes and reject
   deliberate corruption of every file.
5. Verify actual production Unreal smoke literals/enum/source provenance and the
   seven canonical future packaged REM_SMOKE markers (not emitted/verified yet).
6. Verify deferred Android metadata/authorization boundary; execute zero devices.
7. Run the existing Unreal authority source guard.

START/PASS markers identify each actually executed component. The first failure
stops later work, names its component and preserves the owner's failure detail.
The final JSON receipt is written only after every component passes. It retains
actual counts, executed names and asset/package provenance; no private log/save
path, host pointer, timestamp or inferred engine/device success is included.

CI retains the compiled library/generated replay sources and actual suite/JUnit
receipts after success with exact checkout/tree/core blobs and file SHA-256. These
are test build/evidence artifacts, never repository implementation commits.

This suite certifies the pre-real-Unreal R19 source boundary. Representative
synthetic/typed-host replay setup is described in R19_REGRESSION_IMPLEMENTATION.md;
it is not a full played game or actual host presentation proof. R17's historical
private real-save evidence remains owned by R17; no new private replay is performed
in public CI. Actual UHT/Unreal compile/cook/package/runtime, private normalized
asset fidelity and physical/BrowserStack/final-device execution remain deferred.
