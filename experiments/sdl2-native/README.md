# R0 Candidate A — C core + SDL2/OpenGL

This experiment will provide the native baseline.

It must:

- link the shared `remaster_core`;
- render the synthetic R0 map;
- interpolate visual movement without changing logical tile state;
- map keyboard, controller and Android touch to `RemasterInput`;
- demonstrate save/reload;
- report frame timing and input latency observations.

No production art belongs in this experiment.
