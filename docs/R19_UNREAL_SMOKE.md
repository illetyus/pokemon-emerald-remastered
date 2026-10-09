# R19 — Production Unreal smoke preparation

This is a source-level harness and log contract. The real engine gate requires
Unreal 5.8.3 on the project PC after R20. No portable/source CI result here is
an Unreal compile, UHT, cook, package, executable, rendered image or device proof.

`python tools/r19_unreal_smoke.py` verifies the fixed Vanilla+ pin, exact producer
Git blobs, six actual UE_LOG/TEXT production literals and the declared save enum.
It returns SOURCE_CONTRACT_PASS with actual_runtime_verified=false and the
external-engine requirement. No production gameplay or Unreal instrumentation
is changed. R0 core/hash/lifecycle markers cannot satisfy this production contract.

The versioned contract is data/r19/unreal_smoke_contract.json. Its producers are
the actual Vanilla+ save subsystem, world gameplay subsystem and R5 world renderer;
the save status enum is additionally bound. Required markers name usable load,
successful store, authoritative gameplay map, authoritative render, connection
and warp. The renderer can log before map-ready during the broadcast; no invented
ordering between those two source callbacks is imposed.

After an independently verified real build, capture logs privately while the
operator exercises the following scenarios with a usable authorized private save:

1. Load the save, reach the authoritative rendered map, store once and reload.
   Capture exactly two usable status=1 loads and one successful store. Both map
   observations precede store. Counter advances by one with uint32 wrap; slot
   alternates; reload preserves the successfully stored counter and slot.
2. Starting from a usable save/map, exercise both a source connection and a source
   warp. Both actual transition markers must follow the usable load.

Use `python tools/r19_unreal_smoke.py --log PRIVATE_CAPTURE --case save_roundtrip`
or `--case map_transition`. Missing/truncated markers, Error/Fatal/assertion lines,
unusable status, ordering and counter/slot divergence fail. Logs are bounded to
8 MiB. Receipts retain marker counts and no private path or raw log content.
The return status is only LOG_CONTRACT_MATCH, always actual_runtime_verified=false:
synthetic strings can satisfy a parser. R18 must separately bind real engine
version, exact checkout, executable/build artifacts and capture provenance before
accepting an actual smoke result. This tool cannot manufacture that evidence.

Character mesh/animation, camera/occlusion, UI/input, battle presentation, actual
audio playback, lifecycle and Android frame pacing need independent real-runtime
observations. These six log producers do not certify them. Physical Android,
BrowserStack and final device-matrix execution remain deferred at the user's
REAL UNREAL RUNTIME VALIDATION stop. No such work runs in R19 source CI.
