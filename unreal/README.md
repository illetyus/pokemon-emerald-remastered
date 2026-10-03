# Unreal R0

The Unreal project is the selected production runtime direction.

## Engine

- Unreal Engine 5.8
- Linux-first build host
- Android-first runtime target
- ARM64
- Target SDK 35
- Minimum install SDK 26
- Android NDK r27c

## R0 objective

Do not build production Hoenn content yet.

The first Unreal milestone must prove the exact same synthetic contract already used by the portable core:

1. blocked movement;
2. movement to (3,1);
3. interaction event;
4. deterministic encounter;
5. canonical state hash;
6. save/load continuation;
7. touch input;
8. Android pause/resume persistence.

## Architecture

`FRemasterCoreAdapter` is the only presentation-facing entry point during R0.

The portable C implementation is embedded from the root `core/` directory. Unreal must not create a second implementation of movement, collision, event or encounter rules.

## Production direction

After R0:

- world data importer;
- Unreal world-presentation layer;
- HD/2.5D environment;
- camera;
- UMG UI;
- Niagara VFX;
- battle presentation;
- Android device profiles and scalability.

SDL3 remains a regression/reference baseline. Godot is frozen as architecture evidence.


## Linux build host

The automated engine build path is Linux-first.

Expected runner labels:

```text
self-hosted
linux
unreal-5.8
```

The runner must have Unreal Engine 5.8 installed. Set `UE_ROOT` to the engine root or install it in one of these conventional locations:

- `/opt/UnrealEngine-5.8`
- `/opt/UnrealEngine`
- `~/UnrealEngine-5.8`
- `~/UnrealEngine`

Before compiling, CI runs:

```text
bash tools/unreal_linux_preflight.sh "$UE_ROOT"
```

The workflow then builds the Linux Development target and uses `RunUAT.sh BuildCookRun` to cook/package the Android ARM64 Development build. The generated APK is uploaded as a GitHub Actions artifact.
