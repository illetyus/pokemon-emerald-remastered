"""Version-1 canonical production state fingerprints; never serialize C structs."""
from __future__ import annotations

import hashlib
import json
import re

DOMAINS = ('save_block2', 'save_block1', 'storage', 'script', 'objects', 'encounter', 'battle')
LENGTHS = {'vanillaplus': {'save_block2': 0xF44, 'save_block1': 0x3DC8, 'storage': 0x83D0}}
ENCOUNTER_U16 = {'species_bag_count', 'species_bag_cursor', 'species_bag_habitat_mask', 'last_species'}
ENCOUNTER_U8 = {'species_bag_map_group', 'species_bag_map_num', 'species_bag_area', 'species_bag_rod',
                'species_bag_valid', 'roamer_map_group', 'roamer_map_num', 'roamer_location_valid',
                'wild_immunity_steps', 'previous_behavior', 'previous_behavior_valid'}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def canonical(value):
    def validate(item):
        if item is None or type(item) in (str, bool, int):
            return
        if type(item) is list:
            for child in item:
                validate(child)
            return
        if type(item) is dict and all(type(k) is str for k in item):
            for child in item.values():
                validate(child)
            return
        raise ValueError('noncanonical type (including float): ' + type(item).__name__)
    validate(value)
    return (json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False) + '\n').encode('utf-8')


def digest(value):
    return hashlib.sha256(canonical(value)).hexdigest()


def unsigned(value, bits):
    return type(value) is int and 0 <= value < 1 << bits


def encounter_domain(value):
    if value is None:
        return
    fields = ENCOUNTER_U16 | ENCOUNTER_U8 | {'version', 'rng', 'species_bag', 'roamer_location_history'}
    require(type(value) is dict and set(value) == fields, 'encounter runtime field schema')
    require(type(value['version']) is int and value['version'] == 1, 'encounter runtime version')
    require(all(unsigned(value[k], 16) for k in ENCOUNTER_U16)
            and all(unsigned(value[k], 8) for k in ENCOUNTER_U8), 'encounter runtime scalar range')
    rng = value['rng']
    require(type(rng) is dict and set(rng) == {'state', 'calls'}
            and unsigned(rng['state'], 32) and unsigned(rng['calls'], 64), 'encounter RNG state/calls')
    require(type(value['species_bag']) is list and len(value['species_bag']) == 64
            and all(unsigned(s, 16) for s in value['species_bag']), 'encounter full species bag')
    history = value['roamer_location_history']
    require(type(history) is list and len(history) == 3 and all(type(row) is list and len(row) == 2
            and all(unsigned(n, 8) for n in row) for row in history), 'encounter roamer history')
    require(value['species_bag_cursor'] <= value['species_bag_count'] <= 64, 'encounter species bag bounds')


def fingerprint(snapshot):
    require(type(snapshot) is dict and set(snapshot) == {'version', 'format', 'domains', 'observations'},
            'snapshot schema mismatch')
    require(type(snapshot['version']) is int and snapshot['version'] == 1
            and snapshot['format'] in LENGTHS, 'snapshot version/format mismatch')
    domains = snapshot['domains']
    require(type(domains) is dict and set(domains) == set(DOMAINS), 'snapshot domains mismatch')
    for domain, size in LENGTHS[snapshot['format']].items():
        raw = domains[domain]
        require(type(raw) is str and len(raw) == size * 2 and re.fullmatch('[0-9a-f]+', raw) is not None,
                'invalid source-layout domain: ' + domain)
    script = domains['script']
    require(script is None or (type(script) is str and len(script) == 298 * 2
            and script.startswith('523253430100') and re.fullmatch('[0-9a-f]+', script) is not None),
            'invalid version-1 script checkpoint domain')
    encounter_domain(domains['encounter'])
    # Active object/battle domains require their separately audited serializers.
    for domain in ('objects', 'battle'):
        require(domains[domain] is None, 'unsupported active runtime domain: ' + domain)
    require(type(snapshot['observations']) is dict, 'snapshot observations mismatch')
    return {'observations': snapshot['observations'],
            'domain_hashes': {domain: digest(domains[domain]) for domain in DOMAINS},
            'state_hash': digest(snapshot)}


def validate_expected(expected):
    require(type(expected) is dict and set(expected) == {'observations', 'domain_hashes', 'state_hash'},
            'expected snapshot schema mismatch')
    require(type(expected['observations']) is dict and type(expected['domain_hashes']) is dict
            and set(expected['domain_hashes']) == set(DOMAINS), 'expected domains/observations mismatch')
    for value in [expected['state_hash'], *expected['domain_hashes'].values()]:
        require(type(value) is str and re.fullmatch('[0-9a-f]{64}', value) is not None,
                'invalid expected digest')
    canonical(expected['observations'])
