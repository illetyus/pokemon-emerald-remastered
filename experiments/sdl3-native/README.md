# R0 Candidate A — C core + SDL3/OpenGL

This experiment provides the native baseline using the current stable SDL 3.x line.

It must:

- link the shared `remaster_core`;
- render the synthetic R0 map;
- interpolate visual movement without changing logical tile state;
- map keyboard, controller and Android touch to `RemasterInput`;
- demonstrate save/reload;
- report frame timing and input latency observations.

The existing SDL2-based Emerald native-port work remains a useful compatibility reference, but new remaster platform code targets SDL3.

No production art belongs in this experiment.
