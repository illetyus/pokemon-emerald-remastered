#!/usr/bin/env python3
"""Generate the four runtime owner packages in a fresh, isolated directory."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'
SOURCE_REPOSITORY = 'illetyus/pokezumrut-vanillaplus'
VENDOR_TREE = '5a551f1f9e40184278c57dfb8d25f68a0a1c99dc'
FOLDERS = ('World', 'Render', 'Characters', 'Environment')


def destination(root, output):
    root = root.resolve()
    output = Path(os.path.abspath(output))
    for ancestor in (output, *output.parents):
        if ancestor.is_symlink():
            raise ValueError('output symlink boundary')
    if output.is_relative_to(root):
        allowed = ('build', 'out', 'local', 'generated', 'unreal/Content/Generated')
        if not any(output != root / name and output.is_relative_to(root / name)
                   or name == 'unreal/Content/Generated' and output == root / name
                   for name in allowed):
            raise ValueError('protected source output path')
    if output.exists():
        raise ValueError('output already exists; use a new clean destination')
    return output


def source_lock(root):
    lock = json.loads((root / 'external/sources.lock.json').read_text())['vanillaplus']
    if lock['commit'] != PIN or lock['repository'] != SOURCE_REPOSITORY:
        raise ValueError('production source pin drift')


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


def generate(root, stage):
    entry = git(root, 'ls-tree', 'HEAD', 'vendor/vanillaplus').split()
    if len(entry) != 4 or entry[:3] != ['040000', 'tree', VENDOR_TREE]:
        raise ValueError('vendored source tree drift')
    if git(root, 'status', '--porcelain', '--untracked-files=all', '--', 'vendor/vanillaplus'):
        raise ValueError('vendored source working tree is not clean')
    sys.path.insert(0, str(root / 'tools'))
    from convert_world import convert_world
    from audit_generated_content import audit
    from build_r5_render_package import build_render_package
    from build_r6_character_package import build_package as characters, verify_package as verify_characters
    from build_r7_environment_package import build_package as environment
    vendor = root / 'vendor/vanillaplus'
    world = convert_world(vendor, stage / 'World', source_commit=PIN,
                          source_repository=SOURCE_REPOSITORY)
    if world['map_count'] != 518 or world['layout_count'] != 441:
        raise ValueError('full authoritative world coverage')
    errors = audit(stage / 'World')
    if errors:
        raise ValueError('world owner audit failed: ' + '; '.join(errors[:8]))
    render = build_render_package(vendor, stage / 'Render')
    if render['tileset_count'] != 75:
        raise ValueError('full authoritative render coverage')
    characters(root, stage / 'Characters')
    verify_characters(stage / 'Characters')
    environment(vendor, stage / 'Environment')
    environment(vendor, stage / 'Environment', verify=True)
    # These catalogs are compiled source identities, not private runtime assets.
    for script in ('build_r9_ui_catalog.py', 'build_r14_battle_package.py', 'build_r15_audio_catalog.py'):
        subprocess.run([sys.executable, str(root / 'tools' / script), '--check'],
                       cwd=root, check=True)


def build(root, output, generator=generate):
    root = root.resolve()
    output = destination(root, output)
    source_lock(root)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.r20-stage-', dir=output.parent) as tmp:
        stage = Path(tmp) / 'Generated'
        stage.mkdir()
        generator(root, stage)
        for name in FOLDERS:
            if not (stage / name / 'manifest.json').is_file():
                raise ValueError('owner manifest missing: ' + name)
        if output.exists() or output.is_symlink():
            raise ValueError('output appeared during generation')
        stage.rename(output)
    return {'schema': 'remaster.r20.production-layout', 'version': 1,
            'status': 'SOURCE_LAYOUT_READY', 'source_commit': PIN,
            'folders': list(FOLDERS), 'actual_unreal_build': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--expected-index-sha256')
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    if args.verify != bool(args.expected_index_sha256):
        parser.error('--verify requires --expected-index-sha256 from a trusted generation receipt')
    try:
        receipt_path = destination(ROOT, args.receipt) if args.receipt else None
        if receipt_path and receipt_path.is_relative_to(args.output.resolve()):
            raise ValueError('trusted receipt must stay outside the package')
        from r20_package_integrity import build_indexed, verify_package
        result = (verify_package(args.output, args.expected_index_sha256, root=ROOT)
                  if args.verify else build_indexed(ROOT, args.output))
        if receipt_path:
            receipt_path.parent.mkdir(parents=True, exist_ok=True)
            with receipt_path.open('x', encoding='utf-8') as stream:
                stream.write(json.dumps(result, indent=2, sort_keys=True) + '\n')
        print(json.dumps(result, sort_keys=True))
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as exc:
        print('R20 production package failed: ' + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
