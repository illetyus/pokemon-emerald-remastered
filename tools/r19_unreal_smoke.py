#!/usr/bin/env python3
"""Prepare/inspect production smoke log markers without certifying actual execution."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / 'data/r19/unreal_smoke_contract.json'
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'


def validate(config):
    if (config.get('schema') != 'remaster.r19.unreal-smoke' or config.get('version') != 1 or
            config.get('source_commit') != PIN or config.get('actual_runtime_verified') is not False or
            config.get('requires_real_engine') != '5.8.3'):
        raise ValueError('smoke source/runtime contract')
    if [m['id'] for m in config['markers']] != ['load', 'store', 'map_ready', 'rendered', 'connection', 'warp']:
        raise ValueError('smoke marker coverage')
    if config['cases'] != {'save_roundtrip': ['load', 'map_ready', 'rendered', 'store'],
                           'map_transition': ['load', 'map_ready', 'rendered', 'connection', 'warp']}:
        raise ValueError('smoke case coverage')
    sources = {}
    for producer in config['producers']:
        path = producer['path']
        if not path.startswith('unreal/Source/PokemonEmeraldRemastered/') or '..' in Path(path).parts:
            raise ValueError('smoke producer path')
        data = (ROOT / path).read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if blob != producer['blob_sha']: raise ValueError('smoke producer source drift: ' + path)
        sources[path] = data.decode('utf-8')
    for marker in config['markers']:
        literals = re.findall(r'UE_LOG\(\s*\w+,\s*\w+,\s*TEXT\("([^"\n]+)"\)',
                              sources[marker['producer']], re.S)
        if not any(s.startswith(marker['prefix']) for s in literals):
            raise ValueError('smoke marker producer absent: ' + marker['id'])
        re.compile(marker['pattern'])
    header = sources.get('unreal/Source/PokemonEmeraldRemastered/RemasterVanillaPlusSaveSubsystem.h', '')
    values = re.search(r'enum class ERemasterLegacySaveStatus : uint8\s*\{([^}]+)\}', header)
    if not values or [s.strip() for s in values[1].split(',')] != [
            'Empty', 'Ok', 'Degraded', 'Corrupt', 'IoError', 'Unsupported']:
        raise ValueError('smoke save status source enum changed')


def inspect_log(text, case, config=None):
    config = config or json.loads(CONFIG.read_text())
    if case not in config['cases']: raise ValueError('unknown smoke case')
    if len(text.encode('utf-8')) > 8 * 1024 * 1024: raise ValueError('bounded smoke log size')
    if re.search(r'Fatal error:|Assertion failed:|Log\w+:\s*Error:', text):
        raise ValueError('smoke runtime failure marker')
    hits = {m['id']: [] for m in config['markers']}
    for index, line in enumerate(text.splitlines()):
        for marker in config['markers']:
            match = re.search(r'LogTemp:\s*Display:\s*' + marker['pattern'] + r'\s*$', line)
            if match:
                fields = {k: int(v) for k, v in match.groupdict().items()}
                if 'status' in fields and fields['status'] != 1: raise ValueError('unusable smoke save status')
                if ('counter' in fields and not 0 <= fields['counter'] <= 0xFFFFFFFF or
                        'slot' in fields and fields['slot'] not in (0, 1)):
                    raise ValueError('smoke save counter/slot bounds')
                hits[marker['id']].append((index, fields))
    for name in config['cases'][case]:
        if not hits[name]: raise ValueError('missing marker: ' + name)
    if case == 'save_roundtrip':
        if len(hits['load']) != 2 or len(hits['store']) != 1: raise ValueError('smoke reload/store count')
        first, second = hits['load']; stored = hits['store'][0]
        if not (first[0] < hits['map_ready'][0][0] < stored[0] < second[0] and
                first[0] < hits['rendered'][0][0] < stored[0]):
            raise ValueError('smoke reload/store ordering')
        if (stored[1]['counter'] != (first[1]['counter'] + 1) & 0xFFFFFFFF or
                stored[1]['slot'] != first[1]['slot'] ^ 1 or second[1] != first[1] | stored[1]):
            raise ValueError('smoke reload counter/slot divergence')
    elif not (hits['load'][0][0] < hits['connection'][0][0] and hits['load'][0][0] < hits['warp'][0][0]):
        raise ValueError('smoke transition before save load')
    return {'schema': 'remaster.r19.unreal-smoke-log', 'version': 1, 'case': case,
            'status': 'LOG_CONTRACT_MATCH', 'markers': {k: len(v) for k, v in hits.items()},
            'actual_runtime_verified': False, 'external_build_provenance_required': True}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', type=Path)
    parser.add_argument('--case', choices=('save_roundtrip', 'map_transition'), default='save_roundtrip')
    args = parser.parse_args()
    try:
        contract = json.loads(CONFIG.read_text()); validate(contract)
        receipt = inspect_log(args.log.read_text(), args.case, contract) if args.log else {
            'status': 'SOURCE_CONTRACT_PASS', 'markers': len(contract['markers']),
            'actual_runtime_verified': False, 'engine_gate': 'EXTERNAL_ENV_REQUIRED'}
        print(json.dumps(receipt, sort_keys=True))
    except (ValueError, KeyError, OSError) as exc:
        print('R19 Unreal smoke contract failed: ' + str(exc), file=sys.stderr)
        raise SystemExit(1) from exc
