"""Version-1 canonical production state fingerprints; never serialize C structs."""
from __future__ import annotations

import hashlib
import json
import re

DOMAINS = ('save_block2', 'save_block1', 'storage', 'script', 'objects', 'encounter', 'battle')
LENGTHS = {'vanillaplus': {'save_block2': 0xF44, 'save_block1': 0x3DC8, 'storage': 0x83D0}}


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
    # I3/I5/I6 introduce separately audited active serializers. I2 cannot certify them.
    for domain in ('script', 'objects', 'encounter', 'battle'):
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
