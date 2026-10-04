#!/usr/bin/env python3
"""Generate the real Vanilla+ Littleroot -> Route 101 R4 C fixture."""

from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

from convert_world import convert_world

SOURCE_COMMIT = "70db90c9077aed1272e746fc2537d9f12b95a91c"
SOURCE_REPOSITORY = "illetyus/pokezumrut-vanillaplus"

MAP_NAMES = [
    "LittlerootTown_BrendansHouse_1F",
    "LittlerootTown",
    "Route101",
]


def c_u16_array(name: str, values: list[int]) -> str:
    if not values:
        return f"static const uint16_t {name}[1] = {{0}};\n"
    rows = []
    for start in range(0, len(values), 12):
        chunk = ", ".join(f"0x{value:04X}" for value in values[start:start + 12])
        rows.append(f"    {chunk},")
    return (
        f"static const uint16_t {name}[] = {{\n"
        + "\n".join(rows)
        + "\n};\n"
    )


def c_string(value: str) -> str:
    return json.dumps(value)


def emit_fixture(source_root: Path, output_c: Path) -> None:
    with tempfile.TemporaryDirectory() as temp:
        out = Path(temp) / "world"
        manifest = convert_world(
            source_root,
            out,
            source_commit=SOURCE_COMMIT,
            source_repository=SOURCE_REPOSITORY,
        )

        manifest_by_id = {item["id"]: item for item in manifest["maps"]}
        docs_by_id: dict[str, dict] = {}

        def load_by_id(map_id: str) -> dict:
            if map_id not in docs_by_id:
                entry = manifest_by_id[map_id]
                docs_by_id[map_id] = json.loads(
                    (out / entry["file"]).read_text(encoding="utf-8")
                )
            return docs_by_id[map_id]

        selected = [
            json.loads(
                (out / f"maps/{name}.json").read_text(encoding="utf-8")
            )
            for name in MAP_NAMES
        ]

        lines = [
            '#include "r4_littleroot_fixture.h"',
            "",
        ]

        for index, doc in enumerate(selected):
            layout = doc["layout"]
            map_doc = doc["map"]
            prefix = f"r4_map_{index}"

            lines.append(c_u16_array(
                f"{prefix}_blocks",
                layout["raw_blocks_u16"],
            ))
            lines.append(c_u16_array(
                f"{prefix}_border",
                layout["border_active_words_u16"],
            ))
            lines.append(c_u16_array(
                f"{prefix}_primary_attrs",
                layout["primary_metatile_attributes_u16"],
            ))
            lines.append(c_u16_array(
                f"{prefix}_secondary_attrs",
                layout["secondary_metatile_attributes_u16"],
            ))

            warps = map_doc.get("warp_events", [])
            if warps:
                lines.append(
                    f"static const RemasterEmeraldWarpEventDef {prefix}_warps[] = {{"
                )
                for warp in warps:
                    if warp.get("dynamic_target", False):
                        raise ValueError(
                            f"{map_doc['id']}: dynamic warp not supported in R4 fixture"
                        )
                    lines.append(
                        "    {"
                        f"{int(warp['x'])}, {int(warp['y'])}, "
                        f"{int(warp['elevation'])}, "
                        f"{int(warp['dest_warp_id_u16'])}, "
                        f"{int(warp['dest_map_num'])}, "
                        f"{int(warp['dest_group_num'])}"
                        "},"
                    )
                lines.append("};")
            else:
                lines.append(
                    f"static const RemasterEmeraldWarpEventDef {prefix}_warps[1] = {{{{0}}}};"
                )

            connections = map_doc.get("connections", [])
            if connections:
                lines.append(
                    f"static const RemasterEmeraldConnectionDef {prefix}_connections[] = {{"
                )
                for connection in connections:
                    target = load_by_id(connection["map"])
                    target_layout = target["layout"]
                    lines.append(
                        "    {"
                        f"{int(connection['direction_id'])}, "
                        f"{int(connection['offset'])}, "
                        f"{int(connection['dest_group_num'])}, "
                        f"{int(connection['dest_map_num'])}, "
                        f"{int(target_layout['width'])}, "
                        f"{int(target_layout['height'])}"
                        "},"
                    )
                lines.append("};")
            else:
                lines.append(
                    f"static const RemasterEmeraldConnectionDef {prefix}_connections[1] = {{{{0}}}};"
                )

            lines.append("")

        lines.append("const RemasterR4FixtureMap gRemasterR4LittlerootMaps[] = {")
        for index, doc in enumerate(selected):
            layout = doc["layout"]
            map_doc = doc["map"]
            prefix = f"r4_map_{index}"
            secondary_count = len(layout["secondary_metatile_attributes_u16"])
            lines.extend([
                "    {",
                f"        {c_string(map_doc['id'])},",
                f"        {c_string(map_doc['name'])},",
                f"        {int(map_doc['group_num'])},",
                f"        {int(map_doc['map_num'])},",
                f"        {int(map_doc['layout_num'])},",
                f"        {int(map_doc['weather_id'])},",
                f"        {int(map_doc['map_type_id'])},",
                f"        {1 if map_doc.get('requires_flash', False) else 0},",
                "        {",
                f"            {int(layout['width'])},",
                f"            {int(layout['height'])},",
                f"            {prefix}_blocks,",
                f"            {len(layout['raw_blocks_u16'])},",
                f"            {prefix}_border,",
                f"            {len(layout['border_active_words_u16'])},",
                f"            {prefix}_primary_attrs,",
                f"            {len(layout['primary_metatile_attributes_u16'])},",
                (
                    f"            {prefix}_secondary_attrs,"
                    if secondary_count
                    else "            0,"
                ),
                f"            {secondary_count}",
                "        },",
                f"        {prefix}_warps,",
                f"        {len(map_doc.get('warp_events', []))},",
                f"        {prefix}_connections,",
                f"        {len(map_doc.get('connections', []))}",
                "    },",
            ])
        lines.append("};")
        lines.append("")
        lines.append(
            "const size_t gRemasterR4LittlerootMapCount = "
            "sizeof(gRemasterR4LittlerootMaps) / "
            "sizeof(gRemasterR4LittlerootMaps[0]);"
        )
        lines.append("")
        lines.extend([
            "const RemasterR4FixtureMap *remaster_r4_fixture_find_map(",
            "    int32_t group_num,",
            "    int32_t map_num)",
            "{",
            "    size_t i;",
            "",
            "    for (i = 0; i < gRemasterR4LittlerootMapCount; ++i) {",
            "        if (gRemasterR4LittlerootMaps[i].group_num == group_num",
            "            && gRemasterR4LittlerootMaps[i].map_num == map_num)",
            "            return &gRemasterR4LittlerootMaps[i];",
            "    }",
            "",
            "    return 0;",
            "}",
            "",
        ])

        output_c.parent.mkdir(parents=True, exist_ok=True)
        output_c.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 3:
        print(
            "usage: build_r4_littleroot_world_fixture.py "
            "<vanillaplus-source-root> <output-c>",
            file=sys.stderr,
        )
        return 2

    emit_fixture(Path(sys.argv[1]), Path(sys.argv[2]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
