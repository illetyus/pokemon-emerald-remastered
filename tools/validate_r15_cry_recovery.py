#!/usr/bin/env python3
"""Replay historical cry hashes against private packs and rebuilt ZIPs."""
import argparse
import hashlib
import stat
import zipfile
from pathlib import Path, PurePosixPath

from build_r15_audio_catalog import ROOT, dump
from build_r15_special_cries import checked_normal_pack
from validate_r15_audio_pack import strict, validate


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def check_archive(archive, pack):
    archive, pack = Path(archive), Path(pack).resolve()
    expected = {}
    for path in pack.rglob('*'):
        if path.is_symlink():
            raise ValueError('pack contains a symlink')
        if path.is_file():
            expected[path.relative_to(pack).as_posix()] = path
    with zipfile.ZipFile(archive) as zipped:
        infos = zipped.infolist()
        names = [info.filename for info in infos]
        if len(names) != len(set(names)):
            raise ValueError('duplicate ZIP member')
        if len(names) > 10000 or sum(info.file_size for info in infos) > 512 * 1024 * 1024:
            raise ValueError('cry archive budget exceeded')
        for info in infos:
            path = PurePosixPath(info.filename)
            if (path.is_absolute() or '..' in path.parts or '\\' in info.filename
                    or path.as_posix() != info.filename or info.is_dir()
                    or stat.S_ISLNK(info.external_attr >> 16) or info.flag_bits & 1
                    or info.file_size > 64 * 1024 * 1024):
                raise ValueError('unsupported ZIP member')
        if set(names) != set(expected):
            raise ValueError('ZIP members differ from the actual pack')
        for info in infos:
            path = expected[info.filename]
            with zipped.open(info) as stream:
                actual = hashlib.file_digest(stream, 'sha256').hexdigest()
            if info.file_size != path.stat().st_size or actual != digest(path):
                raise ValueError('ZIP content differs: ' + info.filename)
    return {'sha256': digest(archive), 'bytes': archive.stat().st_size, 'members': len(names)}


def check_normal_history(entries, pack, rows):
    expected = {f'cry.{row["national_dex"]}.mode.0': row for row in rows}
    if len(expected) != len(rows) or set(entries) != set(expected):
        raise ValueError('historical normal identity coverage differs')
    for identity, row in expected.items():
        entry = entries[identity]
        if entry.get('sha256') != row['pcm_sha256'] or entry.get('frames') != row['frames']:
            raise ValueError('historical normal PCM differs: ' + identity)
        payload = (Path(pack) / 'masters' / f'{row["national_dex"]:03d}.ogg').read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(payload)).encode() + b'\0' + payload).hexdigest()
        if (len(payload) != row['source_size'] or blob != row['source_blob_sha']
                or hashlib.sha256(payload).hexdigest() != row['source_sha256']):
            raise ValueError('historical source master differs: ' + identity)
    return len(expected)


def verify(normal_pack, special_pack, normal_archive, special_archive):
    normal_pack, special_pack = Path(normal_pack), Path(special_pack)
    normal_evidence = strict((ROOT / 'data/r15/modern_pack_evidence.json').read_text())
    special_evidence = strict((ROOT / 'data/r15/special_pack_evidence.json').read_text())
    rows = normal_evidence['species']
    if len(rows) != 386 or {row['national_dex'] for row in rows} != set(range(1, 387)):
        raise ValueError('historical species coverage differs')
    normals, _, normal_probe = checked_normal_pack(normal_pack)
    normal_count = check_normal_history(normals, normal_pack, rows)
    manifest = strict((special_pack / 'modern_manifest.json').read_text())
    manifest_sha = hashlib.sha256(dump(manifest).encode()).hexdigest()
    if manifest_sha != special_evidence['manifest_sha256']:
        raise ValueError('historical special manifest differs')
    recipe_sha = digest(ROOT / 'data/r15/cry_mode_recipes.json')
    if recipe_sha != special_evidence['recipe_sha256']:
        raise ValueError('historical cry recipes differ')
    special_probe = validate(manifest, special_pack)
    candidates = {entry['identity']: entry for entry in manifest['entries'] if entry['status'] == 'candidate'}
    targets = {f'cry.{dex}.mode.{mode}' for dex in range(1, 387) for mode in range(13)}
    if set(candidates) != targets or normal_probe['unrepresented_semantics'] or special_probe['unrepresented_semantics']:
        raise ValueError('cry recovery semantic coverage differs')
    check_normal_history({key: value for key, value in candidates.items() if key.endswith('.mode.0')}, special_pack, rows)
    missing = sum(entry['status'] == 'missing' for entry in manifest['entries'])
    if missing != 609:
        raise ValueError('non-cry missing semantics changed')
    archives = {}
    for label, archive, pack, evidence in [('normal', normal_archive, normal_pack, normal_evidence),
                                           ('special', special_archive, special_pack, special_evidence)]:
        archives[label] = check_archive(archive, pack)
        archives[label]['historical_sha256'] = evidence['archive_sha256']
        archives[label]['matches_historical_container'] = archives[label]['sha256'] == evidence['archive_sha256']
    return {'schema': 'r15-cry-recovery-audit-v1',
            'source_commit': normal_evidence['source_commit'],
            'source_parameter_pin': special_evidence['source_parameter_pin'],
            'normal_wavs_match_history': normal_count, 'source_masters_match_history': normal_count,
            'special_manifest_matches_history': True, 'manifest_sha256': manifest_sha,
            'recipe_sha256': recipe_sha, 'prepared_special_candidates': 4632,
            'total_cry_candidates': len(candidates), 'missing_other_audio_semantics': missing,
            'archives': archives, 'individual_listening_verified': False,
            'hardware_audio_equivalence_verified': False, 'unreal_import_validated': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    for name in ['normal-pack', 'special-pack', 'normal-archive', 'special-archive']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = verify(args.normal_pack, args.special_pack, args.normal_archive, args.special_archive)
    if args.report:
        args.report.write_text(dump(report))
    print(dump(report), end='')
