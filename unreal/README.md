# Unreal R0

The Unreal project is the selected production runtime direction.

## Engine

- Unreal Engine 5.8
- Android-first
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
