#!/usr/bin/env python3
"""Extract pinned cry parameters and an explicitly authored modern render recipe.

This verifies source parameter semantics, not GBA mixer or emulator equivalence.
"""
import argparse
import hashlib
import re
from build_r15_audio_catalog import ROOT, VENDOR, PIN, dump

OUT = ROOT / 'data/r15/cry_mode_recipes.json'
SOURCES = ['src/sound.c', 'src/m4a.c', 'src/m4a_tables.c', 'src/m4a_1.s',
           'asm/macros/music_voice.inc', 'sound/cry_tables.inc',
           'include/constants/sound.h']
RATE = 48000
CLOCK = 16777216
FRAME_CYCLES = 280896

def function(text, name):
    start = re.search(r'\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', text)
    if not start:
        raise ValueError('source function missing: ' + name)
    depth = 1
    i = start.end()
    while depth and i < len(text):
        depth += (text[i] == '{') - (text[i] == '}')
        i += 1
    if depth:
        raise ValueError('unclosed source function')
    return text[start.start():i]

def numeric_table(text, name):
    body = re.search(r'\b' + name + r'\[\]\s*=\s*\{([^}]+)\}', text).group(1)
    return [int(v.strip().rstrip('uU'), 0) for v in body.split(',') if v.strip()]

def assignments(text, values):
    text = re.sub(r'//[^\n]*', '', text)
    for field, value in re.findall(r'\b(length|reverse|release|pitch|chorus|volume)\s*=\s*(\w+)\s*;', text):
        values[field] = {'TRUE': True, 'FALSE': False}.get(value, int(value) if value.isdigit() else None)
        if values[field] is None:
            raise ValueError('unsupported cry assignment')

def note(pitch, chorus=0):
    # SetPokemonCryPitch then SetPokemonCryChorus, followed by ply_tune and
    # TrkVolPitSet. In particular, 192 is signed -64, followed by 7-bit wrap.
    b = pitch + 0x80
    key = (b >> 8) & 0x7f
    tune = (((b >> 1) & 0x7f) + chorus) & 0x7f
    adjustment = (tune - 64) * 4
    return key + (adjustment >> 8), adjustment & 0xff

def frequency(key, fine, scale, freq):
    def value(k):
        encoded = scale[k]
        return freq[encoded & 15] >> (encoded >> 4)
    a, b = value(key), value(key + 1)
    return a + ((b - a) * fine >> 8)

def build():
    sound = (VENDOR / 'src/sound.c').read_text()
    body = function(sound, 'PlayCryInternal')
    constants = (VENDOR / 'include/constants/sound.h').read_text()
    symbols = [(name, int(i)) for name, i in re.findall(r'#define\s+(CRY_MODE_\w+)\s+(\d+)', constants)]
    if {i for _, i in symbols} != set(range(13)):
        raise ValueError('unexpected cry-mode IDs')
    volume = int(re.search(r'#define\s+CRY_VOLUME\s+(\d+)', constants).group(1))
    defaults = {'volume': volume}
    assignments(body.split('switch (mode)')[0], defaults)
    if set(defaults) != {'length', 'reverse', 'release', 'pitch', 'chorus', 'volume'}:
        raise ValueError('incomplete source cry defaults')
    switch = body.split('switch (mode)')[1].split('SetPokemonCryVolume')[0]
    cases = re.split(r'case\s+(CRY_MODE_\w+)\s*:', switch)[1:]
    ordered = list(zip(cases[::2], cases[1::2]))
    table = (VENDOR / 'src/m4a_tables.c').read_text()
    scale, freq = numeric_table(table, 'gScaleTable'), numeric_table(table, 'gFreqTable')
    baseline = frequency(*note(defaults['pitch']), scale, freq)
    rows = []
    for symbol, mode in sorted(symbols, key=lambda x: x[1]):
        values = dict(defaults)
        start = next(i for i, (s, _) in enumerate(ordered) if s == symbol)
        for _, instructions in ordered[start:]:
            assignments(instructions, values)
            if re.search(r'\bbreak\s*;', instructions):
                break
        tracks = []
        for chorus in ([0, values['chorus']] if values['chorus'] else [0]):
            key, fine = note(values['pitch'], chorus)
            factor = frequency(key, fine, scale, freq)
            tracks.append({'midi_key': key, 'fine_256': fine,
                           'relative_rate': factor / baseline,
                           'render_input_rate': round(RATE * factor / baseline)})
        rows.append({'id': mode, 'source_symbol': symbol, 'source_parameters': values,
                     'tracks': tracks, 'gate_seconds': values['length'] * FRAME_CYCLES / CLOCK,
                     'relative_volume': values['volume'] / volume})
    forward, reverse = (VENDOR / 'sound/cry_tables.inc').read_text().split('gCryTable_Reverse::')
    if re.findall(r'^\s*cry\s+(\w+)', forward, re.M) != re.findall(r'^\s*cry_reverse\s+(\w+)', reverse, re.M):
        raise ValueError('reverse cry table differs from forward sample identities')
    return {'schema': 'r15-cry-mode-recipes-v1', 'source_pin': PIN,
            'source_hashes': {p: hashlib.sha256((VENDOR/p).read_bytes()).hexdigest() for p in SOURCES},
            'clock': {'cycles_per_second': CLOCK, 'cycles_per_frame': FRAME_CYCLES,
                      'default_mplay_tempo': 150, 'tick_per_frame': 1},
            'render_policy': {'kind': 'source_parameter_derived_authored_modern_candidate',
                'pitch': 'source integer MIDI frequency tables, rounded asetrate then SWR 48k; speed changes with pitch',
                'reverse': 'reverse modern decoded sample before resampling, no invented echo delay',
                'chorus': 'two simultaneous source-tuned tracks; signed conversion and 7-bit wrap retained',
                'release': '255-level recurrence (level * release) >> 8 per GBA frame after note gate; interpolate gain over each frame to avoid steps',
                'headroom': 'fixed 0.5 before each resampler; restore in wide accumulation; attenuation only if mixed peak exceeds -3dBFS',
                'level': 'source volume ratio retained; do not normalize each mode back to equal RMS',
                'boundaries': 'modern sample length may end before source gate; no looping or padding to invent a cry',
                'limitations': 'not the GBA compressed decoder, integer mixer, reverb or frame-phase output; emulator A/B and listening remain required'},
            'source_parameter_oracle': 'test_r15_special_cries compiles pinned PlayCryInternal/setters/TrkVolPitSet/MidiKeyToFreq and compares all 13 modes',
            'hardware_audio_equivalence_verified': False,
            'individual_listening_verified': False, 'unreal_import_validated': False,
            'modes': rows}

if __name__ == '__main__':
    p = argparse.ArgumentParser(); p.add_argument('--check', action='store_true'); a = p.parse_args()
    rendered = dump(build())
    if a.check:
        if not OUT.is_file() or OUT.read_text() != rendered:
            raise SystemExit('stale cry-mode recipes')
    else:
        OUT.write_text(rendered)
    print('13 pinned-source cry-mode recipes verified; hardware/listening/import gates remain open')
