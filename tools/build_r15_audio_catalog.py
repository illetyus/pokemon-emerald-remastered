#!/usr/bin/env python3
"""Build source identities only. Never copy commercial sound payloads into git."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from build_r11_domain_catalog import DefineResolver

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / 'vendor/vanillaplus'
OUT = ROOT / 'data/r15/source_audio_catalog.json'
HEADER = ROOT / 'unreal/Source/PokemonEmeraldRemastered/RemasterAudioCatalog.inl'
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'
PILOT = [6,25,94,125,150,201,252,282,306,351,384,386]

def dump(v):
    return json.dumps(v, ensure_ascii=False, sort_keys=True, indent=2) + '\n'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def build():
    resolver = DefineResolver(VENDOR / 'include')
    song_text = (VENDOR / 'include/constants/songs.h').read_text()
    symbols = re.findall(r'#define\s+((?:MUS|SE|PH)_\w+)\s+', song_text)
    names = {}
    for symbol in symbols:
        try: value = resolver.resolve(symbol)
        except (ValueError, KeyError): continue
        names.setdefault(value, []).append(symbol)
    table = re.findall(r'^\s*song\s+(\w+),\s*(\d+),\s*(\d+)',
                       (VENDOR / 'sound/song_table.inc').read_text(), re.M)
    fans = re.findall(r'\[(FANFARE_\w+)\]\s*=\s*\{\s*(MUS_\w+),\s*(\d+)\s*\}',
                      (VENDOR / 'src/sound.c').read_text())
    fan_ids = {resolver.resolve(s) for _,s,_ in fans}
    songs = []
    for i,(symbol,player,group) in enumerate(table):
        files = [p for p in [VENDOR/f'sound/songs/{symbol}.s', VENDOR/f'sound/songs/midi/{symbol}.mid'] if p.is_file()]
        category = ('silence' if i == 0 else 'jingle' if i in fan_ids else
                    'sfx' if symbol.startswith('se_') else 'phoneme' if symbol.startswith('ph_') else 'music')
        songs.append({'source_id': i, 'identity': f'song.{i}', 'source_symbol': symbol,
            'constant_aliases': sorted(names.get(i, [])), 'category': category,
            'source_player': int(player), 'source_group': int(group),
            'source_files': [{'path':str(p.relative_to(VENDOR)), 'sha256':sha(p)} for p in files],
            'original': 'silent' if i==0 else 'missing_local_render',
            'modern': 'silent' if i==0 else 'missing_local_render',
            'loop': {'status':'requires_rendered_frame_metadata' if category=='music' else 'one_shot',
                     'start_frame':None,'end_frame':None},
            'crossfade_seconds': 0.5 if category=='music' else 0.0})
    raw = (VENDOR / 'sound/cry_tables.inc').read_text().split('gCryTable_Reverse::')[0]
    cries = re.findall(r'^\s*cry\s+(\w+)', raw, re.M)
    paths = dict(re.findall(r'(Cry_\w+)::\s*\.incbin\s+"([^"]+)"',
                           (VENDOR / 'sound/direct_sound_data.inc').read_text()))
    remap = {resolver.resolve(symbol):int(index) for symbol,index in re.findall(
        r'\[(SPECIES_\w+)\s*-\s*277\]\s*=\s*(\d+)', (VENDOR/'src/data/pokemon/cry_ids.h').read_text())}
    species = json.loads((ROOT/'data/r14/species_audit.json').read_text())['species']
    selection_path = ROOT/'data/r15/modern_selection.json'
    selected = selection_path.is_file() and json.loads(selection_path.read_text()).get('pilot_style_approved') is True
    pack_path = ROOT/'data/r15/modern_pack_evidence.json'
    pack = json.loads(pack_path.read_text()) if pack_path.is_file() else {}
    prepared = {r['national_dex'] for r in pack.get('species', [])}
    if prepared and (prepared != set(range(1,387)) or pack.get('source_commit') != 'ef687b18f0ce17169b4b4c09175819f7ade92f0f'):
        raise ValueError('invalid full modern pack evidence')
    special_path = ROOT/'data/r15/special_pack_evidence.json'
    special = json.loads(special_path.read_text()) if special_path.is_file() else {}
    special_count = special.get('counts', {}).get('prepared_special_candidates', 0)
    if special_count:
        from build_r15_cry_modes import build as mode_recipes
        recipe_sha = hashlib.sha256(dump(mode_recipes()).encode()).hexdigest()
        if (not prepared or special_count != 4632 or special.get('source_commit') != pack.get('source_commit')
                or special.get('source_parameter_pin') != PIN or special.get('recipe_sha256') != recipe_sha
                or special.get('counts', {}).get('total_cry_candidates') != 5018
                or [(r['mode'], r['files']) for r in special.get('per_mode', [])] != [(m,386) for m in range(1,13)]
                or special.get('unreal_import_validated') is not False
                or special.get('hardware_audio_equivalence_verified') is not False):
            raise ValueError('invalid special-mode candidate evidence')
    rows = []
    for s in species:
        core = s['core_species']; idx = core-1 if core <= 251 else remap[core]
        symbol = cries[idx]; path = paths[symbol].removesuffix('.bin')+'.aif'
        file = VENDOR/path
        if not file.is_file(): raise ValueError(f'missing cry source: {path}')
        rows.append({'core_species':core,'national_dex':s['national_dex'],'name':s['name'],
            'identity':f'cry.{s["national_dex"]}','cry_table_index':idx,'cry_symbol':symbol,
            'source_pcm':{'path':path,'sha256':sha(file)},
            'original':'source_pcm_available_not_verified_hardware_render',
            'modern':'prepared_local_all_modes_candidates_fidelity_pending_not_imported' if special_count else 'prepared_local_normal_only_not_imported' if s['national_dex'] in prepared else 'pilot_candidate_pending_listening' if s['national_dex'] in PILOT else 'missing_pending_pilot',
            'form_policy':'shared_species_cry; visual form and shiny keys do not multiply cry assets'})
    modes = [{'id':resolver.resolve(s),'source_symbol':s} for s in re.findall(
        r'#define\s+(CRY_MODE_\w+)\s+', (VENDOR/'include/constants/sound.h').read_text())]
    fanfares = [{'id':resolver.resolve(n),'source_symbol':n,'song_id':resolver.resolve(s),
                'source_wait_frames':int(d),'identity':f'fanfare.{resolver.resolve(n)}'} for n,s,d in fans]
    sources = ['include/constants/songs.h','include/constants/sound.h','sound/song_table.inc',
        'sound/cry_tables.inc','sound/direct_sound_data.inc','src/data/pokemon/cry_ids.h','src/pokemon.c','src/sound.c']
    categories = {k:sum(s['category']==k for s in songs) for k in sorted({s['category'] for s in songs})}
    catalog = {'schema':'r15-source-audio-v1','source_pin':PIN,
        'source_hashes':{p:sha(VENDOR/p) for p in sources},
        'counts':{'song_table_entries':len(songs),'species_cries':len(rows),'cry_modes':len(modes),
                  'fanfares':len(fanfares),'categories':categories,'modern_pilot_candidates':len(PILOT),
                  'verified_unreal_audio_imports':0,'approved_modern_cries':0,
                  'modern_prepared_normal_cries':len(prepared),'modern_prepared_special_candidates':special_count,
                  'verified_special_mode_hardware_equivalence':0,'pilot_style_approved':selected},
        'songs':songs,'cries':rows,'cry_modes':modes,'fanfares':fanfares,
        'ambience':{'policy':'source weather/ambient SE_* IDs; no invented authoritative event IDs',
                    'candidate_song_ids':[s['source_id'] for s in songs if any(x in s['source_symbol'] for x in ['rain','thunderstorm','downpour'])]},
        'sentinels':{'0':'explicit silence','65535':'MUS_NONE: explicit stop/no music'},
        'unsupported_species':'egg, species 0 and old Unown placeholders; extended 413..439 alias cry.201',
        'bridge':{'core_script':['PLAY_SOUND','PLAY_FANFARE','PLAY_BGM','WAIT_SOUND','WAIT_FANFARE'],
                 'ownership':'runtime owner dispatches committed semantic requests; waits remain source/core-owned',
                 'status':'host attachment required; never infer events from polling UI/animation'},
        'cry_mode_processing':'all 13 exact mode keys required in a playable pack; raw GBA pitch/release numbers are not UE multipliers'}
    lines = ['// Generated source identities; no audio paths.', 'inline constexpr SongEntry Songs[] = {']
    lines += [f'    {{{s["source_id"]}, Category::{s["category"].capitalize()}, "{s["identity"]}"}},' for s in songs]
    lines += ['};','inline constexpr CryEntry Cries[] = {']
    lines += [f'    {{{s["core_species"]}, {s["national_dex"]}, {s["cry_table_index"]}, "{s["identity"]}"}},' for s in rows]
    lines += ['};','inline constexpr FanfareEntry Fanfares[] = {']
    lines += [f'    {{{s["id"]}, {s["song_id"]}, {s["source_wait_frames"]}}},' for s in fanfares]
    lines += ['};','']
    return catalog, '\n'.join(lines)

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
    data, header = build()
    for file,text in [(OUT,dump(data)),(HEADER,header)]:
        if a.check:
            if not file.is_file() or file.read_text()!=text: raise SystemExit(f'stale generated audio catalog: {file}')
        else: file.parent.mkdir(parents=True,exist_ok=True);file.write_text(text)
    print(dump(data['counts']),end='')
