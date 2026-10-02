# R0 Candidate B — C core + Godot bridge

This experiment tests the current preferred hypothesis.

Godot owns:

- rendering
- camera
- UI
- touch/controller mapping
- Android lifecycle integration
- presentation timing

The portable C core remains authoritative for gameplay truth.

The bridge must expose a narrow API: initialize, submit action, snapshot state, save, load and deterministic test/hash operations.

Godot scene nodes must never directly mutate core gameplay state.
