#!/usr/bin/env python3
"""Bounded tracked-byte and reachable-history audit; reports never contain values."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

VENDOR_TREE = '5a551f1f9e40184278c57dfb8d25f68a0a1c99dc'
MAX_BLOB = 16 * 1024 * 1024
PAYLOAD = re.compile(r'\.(gba|gbc|gb|nds|3ds|cia|nsp|rom|srm|sav|apk|aab|zip|7z|rar|wav|mp3|ogg|flac|fbx|dae|obj|blend|uasset|umap|png|jpg|jpeg|dds|ktx|bin|pem|key|p12|pfx|jks|keystore)$', re.I)
CREDENTIAL = re.compile(r'(^|/)(\.env(?:\.(?!example$)[^/]+)?|credentials[^/]*\.json|service[-_]account[^/]*\.json|google-credentials[^/]*\.json|id_rsa|id_ed25519)$', re.I)
PATTERNS = {
    'private-key': re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----'),
    'github-token': re.compile(rb'(?:gh[pousr]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{60,})'),
    'aws-access-key': re.compile(rb'(?:AKIA|ASIA)[A-Z0-9]{16}'),
    'slack-token': re.compile(rb'xox[baprs]-[A-Za-z0-9-]{20,}'),
    'jwt': re.compile(rb'eyJ[A-Za-z0-9_-]{10,}\.eyJ[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{20,}'),
}
OUTPUT_SAMPLES = [
    'build/production/Generated/World/manifest.json', 'local/private-save.srm',
    'unreal/Content/Generated/World/manifest.json',
    'unreal/Content/Generated/Render/manifest.json',
    'unreal/Content/Generated/production-index.json',
    'unreal/Content/Local/R6/mesh.uasset', 'unreal/Content/Local/R7/mesh.uasset',
    'unreal/Content/Local/R14/mesh.uasset', 'unreal/Content/Local/R15/audio.uasset',
    'private.srm', 'private.sav', 'private.zip', '.env.production', 'private.apk',
]


def git(root, *args, data=None):
    return subprocess.check_output(['git', '-C', str(root), *args], input=data, stderr=subprocess.DEVNULL)


def content_rules(data):
    return sorted(name for name, rule in PATTERNS.items() if rule.search(data))


def path_rules(path):
    if path.startswith('vendor/vanillaplus/'):
        return []  # Allowed only with the exact immutable tree gate in production.
    return ['private-payload-path'] if PAYLOAD.search(path) or CREDENTIAL.search(path) else []


def ignored_outputs(root, samples=OUTPUT_SAMPLES):
    return [p for p in samples if subprocess.run(
        ['git', '-C', str(root), 'check-ignore', '-q', '--no-index', '--', p],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0]


def scan_repository(root, *, history=True, max_blob=MAX_BLOB, require_vendor=True):
    root = Path(root)
    findings = []
    paths = {}
    tracked = 0
    def finding(rule, path):
        # Even malicious filename tokens are redacted, not returned as a path.
        findings.append({'rule': rule, 'path': '[sensitive-path]' if content_rules(path.encode()) else path})
    if subprocess.run(['git', '-C', str(root), 'diff', '--quiet', 'HEAD', '--'],
                      stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
        finding('dirty-tracked-source', 'HEAD')
    if require_vendor:
        entry = git(root, 'ls-tree', 'HEAD', 'vendor/vanillaplus').decode().split()
        if len(entry) != 4 or entry[:3] != ['040000', 'tree', VENDOR_TREE]:
            finding('vendor-tree', 'vendor/vanillaplus')
        for p in ignored_outputs(root):
            finding('unprotected-output', p)
    for entry in git(root, 'ls-tree', '-rz', '--full-tree', 'HEAD').split(b'\0'):
        if not entry:
            continue
        meta, rawpath = entry.split(b'\t', 1)
        mode, kind, oid = meta.decode().split()
        path = rawpath.decode('utf-8', errors='backslashreplace')
        tracked += 1
        if kind != 'blob' or mode not in ('100644', '100755'):
            finding('tracked-mode', path)
        if kind == 'blob':
            paths.setdefault(oid, path)
        for rule in path_rules(path):
            finding(rule, path)
    if history:
        if git(root, 'rev-parse', '--is-shallow-repository').strip() != b'false':
            finding('shallow-history', 'HEAD')
        objects = sorted(set(git(root, 'rev-list', '--objects', '--no-object-names', 'HEAD').decode().splitlines()))
    else:
        objects = sorted(paths)
    metadata = git(root, 'cat-file', '--batch-check', data=('\n'.join(objects) + '\n').encode()).decode().splitlines()
    if len(metadata) != len(objects):
        raise ValueError('incomplete Git object metadata')
    scanned = total_bytes = blocked = 0
    process = subprocess.Popen(['git', '-C', str(root), 'cat-file', '--batch'],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    try:
        for expected, entry in zip(objects, metadata):
            parts = entry.split()
            if len(parts) != 3 or parts[0] != expected:
                raise ValueError('unsupported Git object metadata')
            oid, kind, rawsize = parts
            if kind != 'blob':
                continue
            size = int(rawsize)
            path = paths.get(oid, 'git-blob:' + oid)
            if size > max_blob:
                blocked += 1
                finding('blob-limit', path)
                continue
            process.stdin.write((oid + '\n').encode()); process.stdin.flush()
            header = process.stdout.readline().decode().split()
            if header != [oid, 'blob', str(size)]:
                raise ValueError('incomplete Git blob header')
            data = process.stdout.read(size)
            if len(data) != size or process.stdout.read(1) != b'\n':
                raise ValueError('incomplete Git blob bytes')
            scanned += 1
            total_bytes += size
            for rule in content_rules(data):
                finding(rule, path)
    finally:
        process.stdin.close()
        process.stdout.close()
        if process.wait() != 0:
            raise ValueError('Git blob reader failed')
    return {'schema': 'remaster.r20.public-source-audit', 'version': 1,
            'status': 'PUBLIC_SOURCE_AUDIT_FAIL' if findings else 'PUBLIC_SOURCE_AUDIT_PASS',
            'scope': 'tracked HEAD paths and reachable HEAD blob history' if history else 'tracked HEAD only',
            'tracked_files': tracked, 'scanned_blobs': scanned, 'scanned_bytes': total_bytes,
            'skipped_blobs': blocked, 'findings': sorted(findings, key=lambda x: (x['path'], x['rule'])),
            'actual_unreal_build': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    try:
        report = scan_repository(Path(__file__).resolve().parents[1])
        if args.receipt:
            from build_production_package import destination
            receipt = destination(Path(__file__).resolve().parents[1], args.receipt)
            receipt.parent.mkdir(parents=True, exist_ok=True)
            with receipt.open('x', encoding='utf-8') as handle:
                handle.write(json.dumps(report, sort_keys=True, indent=2) + '\n')
        print(json.dumps(report, sort_keys=True))
        return 0 if report['status'].endswith('_PASS') else 1
    except (OSError, ValueError, subprocess.CalledProcessError):
        print('Public source audit could not read complete trusted Git inputs.', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
