# Historical R0 Unreal presentation reference

These are the original unmodified R0 8x8 HUD and direct prototype controller.
R20 moves them out of unreal/Source so Unreal/UHT no longer compiles unused
classes into the production runtime. This directory is not a build target.

The active R0GameMode is still the production bootstrap selecting the R4/R8/R9
host, and RemasterCoreSubsystem still installs the platform/save dependency.
The portable R0 tests, adapter/embed and historical experiments remain intact.
Use the original R0 commits/evidence when reproducing that historical scene.
Private cooked assets are validated separately at the real-engine stage.
