# R0 Android Device Validation

The Android finalists must be tested on real hardware after CI has produced installable APKs.

## What the test proves

The same procedure is applied to both finalists:

- cold launch;
- canonical gameplay input sequence;
- Android background transition;
- resume;
- forced process death;
- fresh process launch;
- persistent state reload;
- canonical state-hash observation;
- frame-pacing telemetry.

The authoritative canonical hash for the common R0 scenario is:

`7218695048241891488`

## Automated ADB test

Use:

```bash
tools/r0_android_device_test.sh <apk> <package>
```

SDL3 package:

```text
com.illetyus.emeraldremaster.r0sdl
```

Godot+C package:

```text
com.illetyus.emeraldremaster.r0godot
```

The script clears previous application data first, so results are reproducible.

## Acceptance

A finalist passes lifecycle/save validation when its report contains:

- the canonical hash after the common input sequence;
- a lifecycle/background save marker;
- a persistent-load marker after force-stop and relaunch;
- R0 performance telemetry.

## Performance observations

For each APK record at least:

- device model and Android version;
- display refresh rate;
- approximate FPS;
- average frame time;
- worst frame time over several 5-second windows;
- cold-launch subjective delay;
- touch responsiveness;
- pause/resume correctness;
- any visible rendering glitches.

R0 does not choose a production architecture using synthetic desktop performance alone. Real Android behavior is an exit criterion.
