# R19 — Deferred final Android matrix

data/r19/android_device_matrix.json defines five planned coverage slots, not
available devices or completed runs. They cover configured minimum/target API
26/35 with intermediate 29/31/33, ARM64 only, low/mid/high tiers, Adreno/Mali and
both configured GLES 3.1/Vulkan backends. RAM/resolution fields are planned
selection constraints; no handset capability or provider availability is verified.
UE 5.8.3 may impose a different supported minimum; reconcile this source config
against the actual engine/toolchain in R18 before selecting real devices.

`python tools/r19_device_matrix.py` checks source config/coverage and the deferred
boundary. Its receipt is DEFERRED_METADATA_PASS, devices_executed=0 and
runtime_verified=false. It imports no ADB/device/cloud client and runs no tests
on a physical device, emulator or BrowserStack. Every target remains NOT_RUN,
with unbound device_model/provider/result_artifact. No credential or APK belongs
in the public source tree.

Ten required scenarios cover usable startup, map/script transitions, save reload,
encounter/battle outcomes, touch/controller/focus, character/environment fallbacks,
audio mix, background/foreground save and RTC boundaries. Six measurement groups
cover CPU/GPU frame distributions, peak memory, sustained 20-minute thermal load,
save latency/integrity, audio dropouts and crash/ANR/lifecycle traces. Actual target
budgets and acceptance thresholds must be reviewed against R20 release settings
and R18 real-engine baseline before device execution; metadata PASS is not a
performance or compatibility approval.

Execution requires an independently verified R18 build plus exact main CI,
completed real Unreal runtime validation and a new user-authorized device stage.
The current automation stops at REAL UNREAL RUNTIME VALIDATION. After renewed
authorization, bind actual model/OS/API/GPU/backend/ABI/provider availability and
private artifact/provenance receipts; then perform physical smoke/BrowserStack
and the final R19 matrix. R21/R22 are not authorized by this source checkpoint.
