# Pokémon Emerald Remastered

Android-first technical remaster project built around preserving Pokémon Emerald gameplay behavior while replacing platform, rendering, UI, input, audio, and presentation layers with modern equivalents.

## Project status

The project is in **R0 — Architecture Prototype**.

R0 exists to prove the technical direction before production art or large-scale gameplay work begins.

## Core principles

- Preserve proven Emerald gameplay behavior wherever practical.
- Separate gameplay truth from presentation and platform code.
- Treat Android as a first-class target, not an emulator wrapper.
- Keep game data portable and machine-convertible.
- Replace presentation progressively instead of rewriting the entire game at once.
- Require regression tests before replacing legacy behavior.
- Do not commit copyrighted commercial game assets or ROM images to this repository.

## Development track

- R0 — Architecture prototype
- R1 — Core/platform separation
- R2 — Android runtime
- R3 — Data conversion pipeline
- R4 — Modern overworld renderer
- R5 — Visual art system
- R6 — Camera, lighting and world presentation
- R7 — Modern UI/UX
- R8 — Android input
- R9 — Battle presentation
- R10 — Mechanical modernization
- R11 — Game feel
- R12 — Audio remaster
- R13 — Performance and device scaling
- R14 — Regression and compatibility
- R15 — Full content conversion and final polish

Development work for R0 lives on `remaster/r0-architecture`.
