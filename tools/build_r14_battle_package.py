#!/usr/bin/env python3
"""Pinned source identities and explicit missing audit; no commercial payloads."""
from pathlib import Path
import argparse
import hashlib
import json
import re
from build_r11_domain_catalog import DefineResolver

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / 'vendor/vanillaplus'
HEADER = ROOT / 'unreal/Source/PokemonEmeraldRemastered/RemasterBattleCatalog.inl'
AUDIT = ROOT / 'data/r14/species_audit.json'


def dump(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2) + '\n'


def build():
    resolver = DefineResolver(VENDOR / 'include')
    dex_source = (VENDOR / 'include/constants/pokedex.h').read_text().split('enum {', 1)[1].split('};', 1)[0]
    dex = re.findall(r'\bNATIONAL_DEX_(\w+)\s*,', dex_source)
    assert dex[0] == 'NONE' and dex[386] == 'DEOXYS'
    entries = (VENDOR / 'src/data/pokemon/pokedex_entries.h').read_text()
    heights = {key: int(re.search(r'\.height\s*=\s*(\d+)', body).group(1)) * 10
               for key, body in re.findall(r'\[NATIONAL_DEX_(\w+)\]\s*=\s*\{([^}]+)\}', entries)}
    source_names = (VENDOR / 'src/data/text/species_names.h').read_text()
    names = dict(re.findall(r'\[(SPECIES_\w+)\]\s*=\s*_\("([^"\n]+)"\)', source_names))
    graphics = (VENDOR / 'src/data/graphics/pokemon.h').read_text()
    paths = dict(re.findall(r'(gMon(?:StillFrontPic|BackPic)_\w+)\[\]\s*=\s*INCBIN_U32\("([^"\n]+)"\)', graphics))
    front = dict(re.findall(r'SPECIES_SPRITE\((\w+),\s*(gMon\w+)\)',
                           (VENDOR / 'src/data/pokemon_graphics/front_pic_table.h').read_text()))
    back = dict(re.findall(r'SPECIES_SPRITE\((\w+),\s*(gMon\w+)\)',
                          (VENDOR / 'src/data/pokemon_graphics/back_pic_table.h').read_text()))
    rows = []
    for national, key in enumerate(dex[1:387], 1):
        species = resolver.resolve('SPECIES_' + key)
        sprite_paths = []
        for table in (front, back):
            symbol = table[key].replace('gMonFrontPic_', 'gMonStillFrontPic_')
            path = paths[symbol].replace('.4bpp.lz', '.png')
            candidates = [path]
            if key == 'CASTFORM':
                candidates = [path.replace('.png', '_' + form + '_form.png')
                              for form in ['normal', 'sunny', 'rainy', 'snowy']]
            for candidate in candidates:
                file = VENDOR / candidate
                if not file.is_file(): raise ValueError(f'missing canonical sprite {candidate}')
                sprite_paths.append({'path': candidate, 'sha256': hashlib.sha256(file.read_bytes()).hexdigest()})
        rows.append({'national_dex': national, 'core_species': species, 'source_symbol': 'SPECIES_' + key,
                     'name': names['SPECIES_' + key], 'identity': f'pokemon.{national}',
                     'exact_model_keys': [f'pokemon.{national}.{form}.{skin}' for form in
                         ([f'unown.{i}' for i in range(28)] if national == 201 else
                          ['normal','sunny','rainy','snowy'] if national == 351 else
                          ['speed'] if national == 386 else ['spinda'] if national == 327 else ['base'])
                         for skin in ['normal','shiny']],
                     'source_height_cm': heights[key], 'source_sprite_references': sprite_paths,
                     'source_model': 'missing_owned_extraction', 'texture': 'missing_3d', 'skeleton': 'uninspected',
                     'animations': {name: 'missing_explicit_fallback' for name in ['idle', 'entry', 'attack', 'hit', 'faint']},
                     'special_channels': 'unknown_until_source_inspection',
                     'scale': {'policy': 'source_height_clamped_for_placeholder_only', 'ground_offset_cm': 0,
                               'calibration': 'unmeasured_until_local_mesh'}, 'runtime': 'primitive_fallback'})
    assert len(set(r['core_species'] for r in rows)) == 386
    trainer_source = (VENDOR / 'include/constants/trainers.h').read_text()
    pics = {int(value): symbol for symbol, value in re.findall(r'#define\s+(TRAINER_PIC_\w+)\s+(\d+)\b', trainer_source)}
    trainers = [{'trainer_pic': key, 'source_symbol': value, 'identity': f'trainer.pic.{key:03d}',
                 'model': 'missing_battle_mesh', 'fallback': 'labelled_human_primitive',
                 'r6_reuse': 'requires_exact_battle_binding_and_calibration'} for key, value in sorted(pics.items())]
    events = [{'id': int(value), 'identity': 'battle.' + name.lower(), 'vfx': 'vfx.' + name.lower(),
               'vfx_asset': 'missing_explicit_primitive_pulse'}
              for name, value in re.findall(r'REMASTER_EMERALD_BATTLE_EVENT_(\w+)\s*=\s*(\d+)',
                                          (ROOT / 'core/include/remaster/emerald_battle.h').read_text().split('REMASTER_EMERALD_BATTLE_EVENT_NONE = 0,', 1)[1].split('};', 1)[0]) if int(value)]
    sources = ['include/constants/species.h', 'include/constants/pokedex.h', 'include/constants/trainers.h',
               'src/data/pokemon/pokedex_entries.h', 'src/data/text/species_names.h',
               'src/data/pokemon_graphics/front_pic_table.h', 'src/data/pokemon_graphics/back_pic_table.h',
               'src/data/graphics/pokemon.h', 'graphics_file_rules.mk', 'src/pokemon.c', 'include/pokemon.h', 'include/constants/pokemon.h']
    audit = {'schema': 'r14-source-audit-v1', 'source_pin': '70db90c9077aed1272e746fc2537d9f12b95a91c',
             'counts': {'species': 386, 'missing_owned_3d_models': 386, 'verified_unreal_imports': 0,
                        'explicit_model_fallbacks': 386, 'trainer_pictures': len(trainers), 'event_semantics': len(events),
                        'exact_form_skin_bindings': sum(len(r['exact_model_keys']) for r in rows)},
             'source_hashes': {p: hashlib.sha256((VENDOR / p).read_bytes()).hexdigest() for p in sources},
             'species': rows, 'trainers': trainers, 'events': events,
             'special_cases': {'unown': '28 personality-selected forms; exact local form bindings',
                               'spinda': 'personality spots require verified material/mask implementation',
                               'castform': 'use current authoritative type, not UI weather simulation',
                               'deoxys': 'Emerald speed form; other source-game forms are not substitutes',
                               'ditto_transform': 'use current core battle species; retain core personality metadata'}}
    lines = ['// Generated by tools/build_r14_battle_package.py; source identities only.', 'inline constexpr SpeciesEntry SpeciesCatalog[] = {']
    lines += [f'    {{{r["core_species"]}, {r["national_dex"]}, {r["source_height_cm"]}, {json.dumps(r["name"], ensure_ascii=False)}}},' for r in rows]
    lines += ['};', 'inline const char* EventIdentity(unsigned kind) {', '    switch (kind) {']
    lines += [f'    case {e["id"]}: return "{e["identity"]}";' for e in events]
    lines += ['    default: return "battle.unsupported";', '    }', '}', 'inline const char* TrainerIdentity(unsigned pic) {', '    switch (pic) {']
    lines += [f'    case {r["trainer_pic"]}: return "{r["identity"]}";' for r in trainers]
    lines += ['    default: return "trainer.unsupported";', '    }', '}', '']
    return audit, '\n'.join(lines)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    audit, header = build()
    if args.check:
        if AUDIT.read_text() != dump(audit) or HEADER.read_text() != header:
            raise SystemExit('R14 source package is stale')
    elif args.output:
        args.output.mkdir(parents=True, exist_ok=True)
        (args.output / 'species_audit.json').write_text(dump(audit))
        (args.output / 'RemasterBattleCatalog.inl').write_text(header)
    else:
        AUDIT.write_text(dump(audit)); HEADER.write_text(header)
