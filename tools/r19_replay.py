#!/usr/bin/env python3
"""Verify immutable ordered-input observations against a compiled Emerald probe."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from r19_state import DOMAINS, fingerprint, validate_expected

ROOT = Path(__file__).resolve().parents[1]
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'
MATRIX = ROOT / 'tests/fixtures/r19/movement.json'
DIRECTIONS = ('south', 'north', 'west', 'east')
ENCOUNTER_RECIPES = ('land-route101-v1', 'repel-expiry-v1', 'disabled-route101-v1', 'water-route102-v1',
                     'old-rod-route102-v1', 'good-rod-route102-v1', 'super-rod-route102-v1', 'rocks-route111-v1')
BATTLE_RECIPES = ('wild-win-v1', 'trainer-replacement-v1', 'trainer-loss-v1', 'qol-held-reward-v1')
ACTION_FIELDS = ('kind', 'move_slot', 'target', 'party_slot', 'item_id')
RECIPES = {'tests/r19_replay_probe.c': ('synthetic-movement-v1',),
           'tests/r19_story_probe.c': ('opening-male-v1', 'opening-female-v1', 'house-exit-v1'),
           'tests/r19_encounter_probe.c': ENCOUNTER_RECIPES,
           'tests/r19_battle_probe.c': BATTLE_RECIPES}
STORY_FIELDS = {'x', 'y', 'map_group', 'map_num', 'kind', 'collision', 'intro', 'rival',
                'town', 'route101', 'lab', 'clock', 'rescued', 'pokemon_get', 'party_count',
                'objective', 'script_status', 'pending_type', 'pending_action', 'pending_sequence',
                'pending_resource', 'host_lock'}
ENCOUNTER_FIELDS = {'occurred', 'repel_wore_off', 'kind', 'area', 'rod', 'level', 'nature', 'gender',
                    'ability_num', 'species', 'modified_rate', 'rng_calls_before', 'rng_calls_after',
                    'pokemon', 'repel', 'map_group', 'map_num', 'immunity', 'rng_state', 'rng_calls'}
BATTLE_FIELDS = {'attached', 'ended', 'outcome', 'turn', 'rng_state', 'rng_calls', 'event_count',
                 'money_multiplier', 'player_hp', 'foe_hp', 'foe_slot', 'money', 'trainer_defeated',
                 'held0', 'held1', 'save_hp', 'save_level', 'whiteout', 'qol_result', 'committed'}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate(matrix):
    require(matrix.get('schema') == 'remaster.r19.replay-matrix' and type(matrix.get('version')) is int and matrix['version'] == 1,
            'unsupported matrix schema/version')
    require(matrix.get('source') == {'repository': 'illetyus/pokezumrut-vanillaplus', 'commit': PIN},
            'source provenance mismatch')
    source = matrix.get('probe_source', {})
    require(type(source) is dict and source.get('path') in RECIPES, 'unsupported probe source')
    raw = (ROOT / source['path']).read_bytes().replace(b'\r\n', b'\n')
    blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
    require(source == {'path': source['path'], 'blob_sha': blob},
            'probe source/recipe drift')
    if source['path'] == 'tests/r19_story_probe.c':
        require(type(matrix.get('inputs')) is dict and set(matrix['inputs']) == {'r2', 'r4'}
                and all(type(v) is str and len(v) == 64 and all(c in '0123456789abcdef' for c in v)
                        for v in matrix['inputs'].values()), 'missing source-generated fixture inputs')
    if source['path'] == 'tests/r19_battle_probe.c':
        raw = (ROOT / 'data/r19/battle_runtime_schema.json').read_bytes().replace(b'\r\n', b'\n')
        require(matrix.get('runtime_schema') == {'path': 'data/r19/battle_runtime_schema.json',
                'blob_sha': hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()},
                'battle serializer schema drift')
    cases = matrix.get('cases')
    require(isinstance(cases, list) and 1 <= len(cases) <= 128, 'invalid cases count')
    ids = set()
    for case in cases:
        identity = case.get('id')
        require(isinstance(identity, str) and identity and identity not in ids, 'duplicate/invalid case ID')
        ids.add(identity)
        require(case.get('recipe') in RECIPES[source['path']], identity + ': unsupported recipe')
        if source['path'] in ('tests/r19_encounter_probe.c', 'tests/r19_battle_probe.c'):
            require(type(case.get('seed')) is int and 0 <= case['seed'] <= 0xFFFFFFFF,
                    identity + ': explicit unsigned seed required')
        else:
            require('seed' not in case, identity + ': unexpected seed for inactive RNG')
        commands = case.get('commands')
        require(isinstance(commands, list) and 1 <= len(commands) <= 4096, identity + ': invalid commands count')
        for index, command in enumerate(commands, 1):
            require(isinstance(command, dict), f'{identity}: unsupported command {index}')
            op = command.get('op')
            if op == 'step':
                require(case['recipe'] in ('synthetic-movement-v1', 'house-exit-v1')
                        and set(command) == {'op', 'direction'} and command['direction'] in DIRECTIONS,
                        f'{identity}: invalid direction/command {index}')
            elif op == 'script_start':
                value = command.get('script_id')
                require(case['recipe'].startswith('opening-') and set(command) == {'op', 'script_id'}
                        and type(value) is str and 1 <= len(value) <= 127
                        and all(c.isascii() and (c.isalnum() or c == '_') for c in value),
                        f'{identity}: invalid script command {index}')
            elif op == 'script_run':
                require(case['recipe'].startswith('opening-') and set(command) == {'op', 'budget'}
                        and type(command['budget']) is int and 1 <= command['budget'] <= 4096,
                        f'{identity}: invalid script budget {index}')
            elif op == 'script_complete':
                require(case['recipe'].startswith('opening-') and set(command) == {'op', 'type', 'action', 'value'}
                        and all(type(command[k]) is int and 0 <= command[k] <= 65535 for k in ('type', 'action', 'value'))
                        and 1 <= command['type'] <= 16, f'{identity}: invalid completion command {index}')
            elif op in ('warp', 'connection', 'save_roundtrip'):
                require(set(command) == {'op'} and (case['recipe'] == 'house-exit-v1' or
                        (op == 'save_roundtrip' and case['recipe'].startswith('opening-'))),
                        f'{identity}: invalid transition command {index}')
            elif op == 'encounter_step':
                require(case['recipe'] in ENCOUNTER_RECIPES[:4] and set(command) == {'op'},
                        f'{identity}: invalid encounter step {index}')
            elif op == 'encounter_fishing':
                rod_recipes = ENCOUNTER_RECIPES[4:7]
                require(case['recipe'] in rod_recipes and set(command) == {'op', 'rod'}
                        and type(command['rod']) is int and command['rod'] == rod_recipes.index(case['recipe']) + 1,
                        f'{identity}: invalid fishing rod/command {index}')
            elif op == 'encounter_rock_smash':
                require(case['recipe'] == 'rocks-route111-v1' and set(command) == {'op'},
                        f'{identity}: invalid Rock Smash command {index}')
            elif op in ('battle_start', 'battle_commit'):
                require(case['recipe'] in BATTLE_RECIPES and set(command) == {'op'},
                        f'{identity}: invalid battle lifecycle command {index}')
            elif op == 'qol_swap_held':
                require(case['recipe'] == 'qol-held-reward-v1' and set(command) == {'op', 'first', 'second'}
                        and all(type(command[k]) is int and 0 <= command[k] < 6 for k in ('first', 'second'))
                        and command['first'] != command['second'], f'{identity}: invalid held-item command {index}')
            elif op == 'battle_turn':
                actions = command.get('actions')
                require(case['recipe'] in BATTLE_RECIPES and set(command) == {'op', 'actions'}
                        and type(actions) is list and len(actions) == 4, f'{identity}: invalid battle actions {index}')
                for action in actions:
                    require(type(action) is dict and set(action) == set(ACTION_FIELDS)
                            and all(type(action[k]) is int and 0 <= action[k] <= maximum
                                    for k, maximum in zip(ACTION_FIELDS, (4, 3, 3, 5, 65535)))
                            and (action['kind'] != 0 or all(action[k] == 0 for k in ACTION_FIELDS[1:])),
                            f'{identity}: invalid typed battler action {index}')
            else:
                raise ValueError(f'{identity}: unsupported command {index}')
        require(isinstance(case.get('expected'), list) and len(case['expected']) == len(commands) + 1,
                identity + ': expected snapshot count mismatch')
        for expected in case['expected']:
            validate_expected(expected)
            observations = expected['observations']
            if case['recipe'] in BATTLE_RECIPES:
                require(set(observations) == BATTLE_FIELDS and all(type(v) is int for v in observations.values()),
                        identity + ': invalid battle observation')
            elif case['recipe'] in ENCOUNTER_RECIPES:
                pokemon = observations.get('pokemon')
                require(set(observations) == ENCOUNTER_FIELDS
                        and all(type(v) is int for k, v in observations.items() if k != 'pokemon')
                        and (pokemon is None or (type(pokemon) is str and len(pokemon) == 200
                             and all(c in '0123456789abcdef' for c in pokemon))),
                        identity + ': invalid encounter observation')
            else:
                fields = {'x', 'y', 'kind', 'collision'} if case['recipe'] == 'synthetic-movement-v1' else STORY_FIELDS
                require(set(observations) == fields and all(type(v) in (int, str, bool) for v in observations.values()),
                        identity + ': invalid expected observation')


def execute(probe, case, mode=None):
    lines = []
    for c in case['commands']:
        op = c['op']
        if op == 'step': lines.append('step ' + c['direction'])
        elif op == 'script_start': lines.append('script_start ' + c['script_id'])
        elif op == 'script_run': lines.append('script_run ' + str(c['budget']))
        elif op == 'script_complete': lines.append(f"script_complete {c['type']} {c['action']} {c['value']}")
        elif op == 'encounter_fishing': lines.append('encounter_fishing ' + str(c['rod']))
        elif op == 'qol_swap_held': lines.append(f"qol_swap_held {c['first']} {c['second']}")
        elif op == 'battle_turn':
            lines.append('battle_turn ' + ' '.join(str(action[k]) for action in c['actions'] for k in ACTION_FIELDS))
        else: lines.append(op)
    commands = '\n'.join(lines) + '\n'
    try:
        result = subprocess.run([str(probe), case['recipe']] + ([str(case['seed'])] if 'seed' in case else [])
                                + ([mode] if mode else []), input=commands, text=True,
                                capture_output=True, timeout=30, check=False)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise ValueError(f"{case['id']}: probe execution failed: {error}") from error
    require(result.returncode == 0, f"{case['id']}: probe failed: {result.stderr.strip()}")
    try:
        return [json.loads(line) for line in result.stdout.splitlines()]
    except json.JSONDecodeError as error:
        raise ValueError(f"{case['id']}: invalid probe JSON: {error}") from error


def verify(case, actual):
    require(len(actual) == len(case['expected']), f"{case['id']}: snapshot count: expected {len(case['expected'])}, actual {len(actual)}")
    for index, (expected, observed) in enumerate(zip(case['expected'], actual)):
        try:
            result = fingerprint(observed)
        except ValueError as error:
            raise ValueError(f"{case['id']}: snapshot {index}: {error}") from error
        observations = result['observations']
        require(set(observations) == set(expected['observations']), f"{case['id']}: snapshot {index}: observation schema mismatch")
        for field, value in expected['observations'].items():
            require(type(observations[field]) is type(value) and observations[field] == value,
                    f"{case['id']}: snapshot {index}, command {case['commands'][index-1] if index else 'initial'}, "
                    f"field {field}: expected {value}, actual {observations[field]}")
        for domain in DOMAINS:
            require(result['domain_hashes'][domain] == expected['domain_hashes'][domain],
                    f"{case['id']}: snapshot {index}, command {case['commands'][index-1] if index else 'initial'}, "
                    f"domain {domain}: expected {expected['domain_hashes'][domain]}, actual {result['domain_hashes'][domain]}")
        require(result['state_hash'] == expected['state_hash'],
                f"{case['id']}: snapshot {index}, state hash: expected {expected['state_hash']}, actual {result['state_hash']}")


def run(probe, matrix, inputs=None):
    validate(matrix)
    if matrix.get('inputs'):
        require(inputs is not None and set(inputs) == set(matrix['inputs']), 'required generated fixture inputs missing')
        for name, path in inputs.items():
            raw = path.read_bytes().replace(b'\r\n', b'\n')
            require(hashlib.sha256(raw).hexdigest() == matrix['inputs'][name], 'generated source fixture drift: ' + name)
    for case in matrix['cases']:
        actual = execute(probe, case)
        verify(case, actual)
        require(actual == execute(probe, case), case['id'] + ': repeat execution differs')
        if case['recipe'] in BATTLE_RECIPES:
            verify(case, execute(probe, case, '--transport-noise'))
            active_index = next(i for i, c in enumerate(case['commands'], 1) if c['op'] == 'battle_start')
            try:
                verify(case, execute(probe, case, '--runtime-noise'))
            except ValueError as error:
                require(f'snapshot {active_index}' in str(error) and 'domain battle' in str(error),
                        'battle runtime mutation failed at wrong boundary: ' + str(error))
            else:
                raise ValueError('battle runtime mutation was not detected')
        if case['recipe'] in ENCOUNTER_RECIPES:
            verify(case, execute(probe, case, '--transport-noise'))
            try:
                verify(case, execute(probe, case, '--runtime-noise'))
            except ValueError as error:
                require('snapshot 0' in str(error) and 'domain encounter' in str(error),
                        'encounter runtime mutation failed at wrong boundary: ' + str(error))
            else:
                raise ValueError('encounter runtime mutation was not detected')
        if case['recipe'] != 'synthetic-movement-v1':
            continue
        verify(case, execute(probe, case, '--transport-noise'))
        try:
            verify(case, execute(probe, case, '--persistent-noise'))
        except ValueError as error:
            require('snapshot 0' in str(error) and 'domain save_block1' in str(error),
                    'persistent mutation failed at wrong boundary: ' + str(error))
        else:
            raise ValueError('persistent mutation was not detected')
        mutated = dict(case, commands=[dict(c) for c in case['commands']])
        mutated['commands'][0]['direction'] = 'south'
        try:
            verify(case, execute(probe, mutated))
        except ValueError as error:
            require('snapshot 1' in str(error), 'command mutation failed at wrong index: ' + str(error))
        else:
            raise ValueError('command mutation was not detected')
    # The native boundary independently rejects commands not accepted by Python.
    first = matrix['cases'][0]
    malformed_recipe = 'house-exit-v1' if matrix['probe_source']['path'] == 'tests/r19_story_probe.c' else first['recipe']
    native_args = [str(probe), malformed_recipe] + ([str(first['seed'])] if 'seed' in first else [])
    for bad in ('step diagonal\n', 'set_story_flag 1\n', 'step north extra\n', '\n'):
        result = subprocess.run(native_args, input=bad,
                                text=True, capture_output=True, timeout=30, check=False)
        require(result.returncode != 0, 'native probe accepted malformed command')
    if matrix['probe_source']['path'] == 'tests/r19_encounter_probe.c':
        for recipe, bad in (('land-route101-v1', 'encounter_step extra\n'),
                            ('old-rod-route102-v1', 'encounter_fishing 4\n'),
                            ('rocks-route111-v1', 'encounter_rock_smash extra\n')):
            result = subprocess.run([str(probe), recipe, '0'], input=bad,
                                    text=True, capture_output=True, timeout=30, check=False)
            require(result.returncode != 0, 'native encounter accepted invalid typed command')
    if matrix['probe_source']['path'] == 'tests/r19_battle_probe.c':
        for recipe, bad in (('wild-win-v1', 'battle_commit\n'), ('wild-win-v1', 'battle_start extra\n'),
                            ('wild-win-v1', 'battle_start\nbattle_turn 99 ' + '0 ' * 19 + '\n'),
                            ('qol-held-reward-v1', 'battle_start\nqol_swap_held 0 1\n')):
            result = subprocess.run([str(probe), recipe, '0'], input=bad,
                                    text=True, capture_output=True, timeout=30, check=False)
            require(result.returncode != 0, 'native battle accepted invalid lifecycle/typed command')
    if matrix['probe_source']['path'] == 'tests/r19_story_probe.c':
        for bad in ('script_complete 16 2 0\n', 'script_start UnknownScript\n',
                    'script_start LittlerootTown_EventScript_StepOffTruckMale\nscript_run 4096\nscript_complete 16 4 0\n'):
            result = subprocess.run([str(probe), 'opening-male-v1'], input=bad,
                                    text=True, capture_output=True, timeout=30, check=False)
            require(result.returncode != 0, 'source script accepted missing/wrong request completion')
    print(f"R19 ordered production replay: {len(matrix['cases'])} cases, "
          f"{sum(len(c['expected']) for c in matrix['cases'])} observations PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--matrix', type=Path, default=MATRIX)
    parser.add_argument('--r2-input', type=Path)
    parser.add_argument('--r4-input', type=Path)
    args = parser.parse_args()
    try:
        inputs = {'r2': args.r2_input, 'r4': args.r4_input} if args.r2_input and args.r4_input else None
        run(args.probe.resolve(), json.loads(args.matrix.read_text()), inputs)
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        print(f'R19 replay FAIL: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
