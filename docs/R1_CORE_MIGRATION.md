# R1 — Vanilla+ Core Migration

Production gameplay behavior is sourced from:

- repository: `illetyus/pokezumrut-vanillaplus`
- pinned baseline: `70db90c9077aed1272e746fc2537d9f12b95a91c`

Native-platform reference:

- `gradenGnostic/pokeemerald-multiplatform`

The multiplatform project is a reference for separating GBA hardware concerns. It is not the authoritative game-design source.

## Migration rule

Do not rewrite gameplay systems from memory.

For each subsystem:

1. identify the Vanilla+ source files and regression behavior;
2. inventory direct GBA/platform dependencies;
3. replace hardware calls with the portable platform boundary;
4. compile the subsystem natively;
5. expose only commands, snapshots and presentation events to Unreal;
6. compare deterministic outcomes against the Vanilla+ reference behavior;
7. only then allow Unreal presentation work to depend on that subsystem.

## Order

1. save state and RTC;
2. map/event state;
3. player movement and collision;
4. scripts/flags/variables;
5. encounters;
6. party/Pokémon data;
7. items/bag/PC;
8. battle state machine;
9. quest/navigation state;
10. remaining menus/system behavior.

## Platform-coupling inventory

Run:

```text
python tools/scan_platform_coupling.py <path-to-pokezumrut-vanillaplus> --output build/platform-coupling.json
```

The report identifies source files that directly reference hardware registers, VRAM/palette/OAM, DMA, interrupts, BIOS calls or the GBA audio driver. Those files receive explicit adapters rather than being copied blindly into Unreal.

## Save compatibility

The migration keeps old Vanilla+ save interpretation separate from Unreal's outer container/lifecycle storage. Save-domain bytes remain authoritative; Unreal only owns where and when those bytes are persisted.

## Exit criteria

R1 is complete only when a real Vanilla+ save can load into the native core, report equivalent player/map/party state, execute a deterministic input/script sequence, save and reload without GBA hardware access.
