#!/usr/bin/env python3
"""Source render recipes and MIDI timelines, never rendered PCM loop claims."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / 'vendor/vanillaplus'
OUT = ROOT / 'data/r15/bgm_source_plan.json'
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'


def dump(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2) + '\n'


def recipes(text):
    reverb = re.findall(r'^STD_REVERB\s*=\s*(\d+)\s*$', text, re.M)
    if len(reverb) != 1:
        raise ValueError('one authoritative STD_REVERB required')
    result = {}
    pattern = r'^\$\(MID_SUBDIR\)/(\w+)\.s: %\.s: %\.mid\n\t\$\(MID\) \$< \$@ ([^\n]+)'
    for symbol, command in re.findall(pattern, text, re.M):
        if symbol in result:
            raise ValueError('duplicate song recipe')
        values = dict(voicegroup=0, master_volume=127, priority=0, reverb=-1,
                      exact_gate_time=False, compression=True, clocks_per_beat=24)
        seen = set()
        for token in command.split():
            token = token.replace('$(STD_REVERB)', reverb[0])
            match = re.fullmatch(r'-([EVGPRNXevgprnx])(\d*)', token)
            if not match:
                raise ValueError('unsupported source conversion option: ' + token)
            flag, number = match.groups(); flag = flag.upper()
            if flag in seen:
                raise ValueError('duplicate conversion option')
            seen.add(flag)
            if flag in 'ENX':
                if number:
                    raise ValueError('unexpected flag argument')
                key, value = {'E': ('exact_gate_time', True), 'N': ('compression', False),
                              'X': ('clocks_per_beat', 48)}[flag]
            else:
                if not number:
                    raise ValueError('missing conversion argument')
                key = dict(G='voicegroup', V='master_volume', P='priority', R='reverb')[flag]
                value = int(number)
                if value > (999 if flag == 'G' else 255 if flag == 'P' else 127):
                    raise ValueError('out of range conversion option')
            values[key] = value
        result[symbol] = values
    return result


class Reader:
    def __init__(self, data):
        self.data = data; self.pos = 0

    def take(self, count):
        if count < 0 or self.pos + count > len(self.data):
            raise ValueError('truncated MIDI')
        data = self.data[self.pos:self.pos + count]; self.pos += count
        return data

    def byte(self):
        return self.take(1)[0]

    def vlq(self):
        value = 0
        for _ in range(4):
            b = self.byte(); value = (value << 7) | (b & 127)
            if b < 128:
                return value
        raise ValueError('MIDI VLQ exceeds four bytes')


def midi_timeline(data):
    if len(data) > 2 * 1024 * 1024:
        raise ValueError('MIDI exceeds audit bound')
    reader = Reader(data)
    if reader.take(8) != b'MThd\x00\x00\x00\x06':
        raise ValueError('unsupported MIDI header')
    fmt, tracks, division = struct.unpack('>HHH', reader.take(6))
    if fmt not in (0, 1) or not 1 <= tracks <= 256 or (fmt == 0 and tracks != 1):
        raise ValueError('unsupported MIDI format/track count')
    if not 0 < division < 0x8000:
        raise ValueError('unsupported MIDI time division')
    markers, tempos, ends, post_eot, nonstandard, ignored = [], [], [], [], [], 0
    for track in range(tracks):
        if reader.take(4) != b'MTrk':
            raise ValueError('missing MIDI track')
        size = struct.unpack('>I', reader.take(4))[0]
        r = Reader(reader.take(size)); tick = 0; running = 0; ended = False
        while r.pos < len(r.data):
            tick += r.vlq(); status = r.byte()
            if status < 128:
                if not running:
                    raise ValueError('missing MIDI running status')
                r.pos -= 1; status = running
            if 0x80 <= status <= 0xef:
                running = status
                payload = r.take(1 if status >> 4 in (12, 13) else 2)
                if any(b >= 128 for b in payload):
                    # One pinned song has CC7=128. mid2agb reads its byte
                    # literally; do not normalize it to 127 or certify a
                    # renderer that silently masks it to seven bits.
                    if status >> 4 != 11 or payload[0] >= 128:
                        raise ValueError('invalid MIDI channel data')
                    nonstandard.append(dict(track=track, tick=tick,
                                            controller=payload[0], value=payload[1]))
            elif status in (0xf0, 0xf7):
                running = 0; r.take(r.vlq())
            elif status == 0xff:
                running = 0; kind = r.byte(); payload = r.take(r.vlq())
                if kind == 0x2f:
                    if payload:
                        raise ValueError('invalid MIDI end-of-track')
                    # Pinned mid2agb stops at the first EOT and uses the declared
                    # chunk size for the next track. Some source tracks have two
                    # EOT events; preserve evidence of ignored bytes explicitly.
                    trailing = r.data[r.pos:]
                    post_eot.append(dict(bytes=len(trailing),
                        sha256=hashlib.sha256(trailing).hexdigest() if trailing else None))
                    ended = True; break
                if kind == 0x51:
                    if len(payload) != 3 or not int.from_bytes(payload, 'big'):
                        raise ValueError('invalid MIDI tempo')
                    if track == 0:
                        tempos.append(dict(tick=tick, microseconds_per_quarter=int.from_bytes(payload, 'big')))
                    else:
                        ignored += 1
                if 1 <= kind <= 7 and payload in (b'[', b']', b'][', b':'):
                    if track == 0:
                        markers.append(dict(tick=tick, text=payload.decode('ascii'), meta_type=kind))
                    else:
                        ignored += 1
            else:
                raise ValueError('unsupported MIDI event')
        if not ended:
            raise ValueError('MIDI track missing end-of-track')
        ends.append(tick)
    if reader.pos != len(data):
        raise ValueError('unexpected trailing MIDI bytes')
    return dict(format=fmt, division_ticks_per_quarter=division, markers=markers,
                tempo_events=tempos, track_end_ticks=ends, ignored_post_eot=post_eot,
                nonstandard_controller_values=nonstandard,
                ignored_other_track_sequence_events=ignored)


def build():
    catalog = json.loads((ROOT / 'data/r15/source_audio_catalog.json').read_text())
    if catalog['source_pin'] != PIN:
        raise ValueError('unexpected source pin')
    table = (VENDOR / 'sound/song_table.inc').read_text()
    rows = re.findall(r'^\s*song\s+(\w+),\s*(\d+),\s*(\d+)', table, re.M)
    if len(rows) != len(catalog['songs']):
        raise ValueError('song table coverage drift')
    if not re.search(r'dummy_song_header:\s*\.byte\s+0,\s*0,\s*0,\s*0\s*$', table):
        raise ValueError('reserved zero-track source header changed')
    rules = recipes((VENDOR / 'songs.mk').read_text())
    jobs, reserved = [], []
    for i, (symbol, player, group) in enumerate(rows):
        s = catalog['songs'][i]
        if (s['source_id'], s['source_symbol'], s['source_player'], s['source_group']) != (i, symbol, int(player), int(group)):
            raise ValueError('catalog differs from source song table')
        if symbol == 'dummy_song_header':
            reserved.append(dict(source_id=i, identity=s['identity'], source_player=int(player),
                                 source_group=int(group), track_count=0, render_required=False,
                                 owner_behavior='source zero-track start; host player/priority policy remains pending'))
            continue
        if s['category'] not in ('music', 'jingle'):
            continue
        expected = 'sound/songs/midi/' + symbol + '.mid'
        if len(s['source_files']) != 1 or s['source_files'][0]['path'] != expected or symbol not in rules:
            raise ValueError('unsupported music source recipe: ' + symbol)
        source = s['source_files'][0]; data = (VENDOR / expected).read_bytes()
        if hashlib.sha256(data).hexdigest() != source['sha256']:
            raise ValueError('music source hash mismatch: ' + symbol)
        timeline = midi_timeline(data)
        jobs.append(dict(source_id=i, identity=s['identity'], source_symbol=symbol,
            category=s['category'], source_player=int(player), source_group=int(group),
            source_file=source, conversion=rules[symbol], midi=timeline,
            loop_pcm_frames=None, render_verified=False,
            loop_status='source_markers_pending_render' if timeline['markers'] else 'no_source_loop_markers_pending_render',
            transition=dict(crossfade_seconds=s['crossfade_seconds'],
                            authority='remaster presentation policy; not source MPlay fade timing')))
    sources = ['sound/song_table.inc', 'songs.mk', 'src/m4a.c'] + [
        'tools/mid2agb/' + name + ext
        for name in ['main', 'midi', 'agb', 'tables', 'error'] for ext in ['.cpp', '.h']]
    return dict(schema='r15-bgm-source-plan-v1', source_pin=PIN,
        source_hashes={p: hashlib.sha256((VENDOR / p).read_bytes()).hexdigest() for p in sources},
        counts=dict(renderable_music=sum(j['category'] == 'music' for j in jobs),
                    renderable_jingles=sum(j['category'] == 'jingle' for j in jobs),
                    reserved_zero_track_rows=len(reserved)),
        sequence_authority='mid2agb ReadSeqEvents reads track 0; text meta types 1..7; ticks are MIDI units, not PCM frames',
        compatibility='additive planning artifact; existing 610-row catalog, 5627 resolver keys and historical cry pack manifests unchanged',
        silence_id=0, stop_sentinel=65535, reserved=reserved, render_jobs=jobs,
        unreal_import_validated=False, hardware_audio_equivalence_verified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('--check', action='store_true')
    args = parser.parse_args(); plan = build(); text = dump(plan)
    if args.check:
        if not OUT.is_file() or OUT.read_text() != text:
            raise SystemExit('stale BGM source plan')
    else:
        OUT.parent.mkdir(parents=True, exist_ok=True); OUT.write_text(text)
    print(dump(plan['counts']), end='')
