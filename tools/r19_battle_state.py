"""Validate frozen explicit battle members/arrays; no C memory-layout hashing."""
import json
from pathlib import Path
import re

SCHEMA = json.loads((Path(__file__).resolve().parents[1] / 'data/r19/battle_runtime_schema.json').read_text())


def require(ok, message):
    if not ok:
        raise ValueError(message)


def field(value, spec, label):
    shape = spec.get('shape', [])
    if shape:
        require(type(value) is list and len(value) == shape[0], 'battle array shape: ' + label)
        for i, child in enumerate(value):
            field(child, dict(spec, shape=shape[1:]), label + '[' + str(i) + ']')
        return
    kind = spec['type']
    if kind == 'party-codec':
        require(type(value) is str and len(value) == 200 and re.fullmatch('[0-9a-f]+', value) is not None,
                'battle party codec: ' + label)
    elif kind in SCHEMA['structs']:
        struct(value, kind, label)
    elif kind == 'active-events':
        require(type(value) is list and len(value) <= 256, 'battle active event capacity')
        for i, event in enumerate(value):
            struct(event, 'RemasterEmeraldBattleEvent', label + '[' + str(i) + ']')
    elif kind == 'count':
        require(type(value) is int and 0 <= value <= 256, 'battle event count')
    else:
        low, high = (-(1 << 31), 1 << 31) if kind == 'i32' else (0, 1 << int(kind[1:]))
        require(type(value) is int and low <= value < high, 'battle scalar range: ' + label)


def struct(value, name, label):
    fields = SCHEMA['structs'][name]
    require(type(value) is dict and set(value) == set(fields), 'battle member schema: ' + label)
    for key, spec in fields.items():
        field(value[key], spec, label + '.' + key)


def validate_battle(value):
    if value is None:
        return
    require(SCHEMA['schema'] == 'remaster.r19.battle-runtime' and SCHEMA['version'] == 1,
            'battle serializer schema/version')
    require(type(value) is dict and type(value.get('version')) is int and value['version'] == 1,
            'battle domain version')
    state = {k: v for k, v in value.items() if k != 'version'}
    struct(state, 'RemasterEmeraldBattleState', 'state')
    require(state['event_count'] == len(state['events']), 'battle active event count mismatch')
    require(all(c <= 6 for c in state['party_count']), 'battle party count bounds')
