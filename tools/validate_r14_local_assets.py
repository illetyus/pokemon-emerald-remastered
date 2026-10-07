#!/usr/bin/env python3
"""Bounded local glTF geometry/channel probe, not a full glTF or UE validator."""
from pathlib import Path
import argparse
import hashlib
import json
import math
import re
import struct
from urllib.parse import unquote
from validate_r7_local_assets import png_dimension
from build_r14_battle_package import AUDIT

SEMANTICS = ['idle', 'entry', 'attack', 'hit', 'faint']
SPECIAL = ['material', 'visibility', 'morph', 'UV', 'texture_pattern']


def indexed(rows, index):
    if type(index) is not int or not 0 <= index < len(rows):
        raise ValueError('invalid nonnegative index')
    return rows[index]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def strict(path):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result: raise ValueError('duplicate JSON key')
            result[key] = value
        return result
    return json.loads(path.read_text(), object_pairs_hook=pairs,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError('non-finite JSON')))


def local(root, raw):
    if not isinstance(raw, str) or not raw or ':' in raw or '\\' in raw:
        raise ValueError('invalid local relative path')
    path = (root / unquote(raw)).resolve()
    if Path(raw).is_absolute() or not path.is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError('asset escaped local root or is missing')
    if path.stat().st_size > 32 * 1024 * 1024: raise ValueError('asset exceeds bounded probe size')
    return path


def checked(root, record):
    path = local(root, record['path'])
    if not re.fullmatch('[0-9a-f]{64}', record.get('sha256', '')) or digest(path.read_bytes()) != record['sha256']:
        raise ValueError('file hash mismatch')
    return path


def gltf_probe(path):
    doc = strict(path)
    if doc.get('asset', {}).get('version') != '2.0': raise ValueError('glTF 2.0 required')
    # Unsupported extensions may contain omitted geometry or animation. Fail visibly.
    if doc.get('extensionsUsed') or doc.get('extensionsRequired'): raise ValueError('unsupported glTF extension; explicit inspection required')
    root = path.parent
    buffers = []
    files = {path.name: digest(path.read_bytes())}
    for item in doc.get('buffers', []):
        file = local(root, item['uri']); data = file.read_bytes()
        if len(data) != item['byteLength']: raise ValueError('buffer byte length mismatch')
        buffers.append(data); files[item['uri']] = digest(data)
    nodes = doc.get('nodes', [])
    parents = {}
    for i, node in enumerate(nodes):
        for child in node.get('children', []):
            indexed(nodes, child)
            if child in parents: raise ValueError('multiple node parents')
            parents[child] = i
    # Probe coordinates directly; baked mesh node transforms avoid false ground bounds.
    used_meshes = set()
    for i, node in enumerate(nodes):
        ancestors = set(); cursor = i
        while cursor is not None:
            if cursor in ancestors: raise ValueError('cyclic node graph')
            ancestors.add(cursor); cursor = parents.get(cursor)
        if 'mesh' in node:
            indexed(doc.get('meshes', []), node['mesh'])
            if node['mesh'] in used_meshes: raise ValueError('instanced mesh requires separate bounds probe')
            used_meshes.add(node['mesh'])
            if any(any(k in nodes[a] for k in ['matrix', 'translation', 'rotation', 'scale']) for a in ancestors):
                raise ValueError('mesh ancestor transforms must be baked for calibrated probe')
            if 'skin' in node: indexed(doc.get('skins', []), node['skin'])
    if used_meshes != set(range(len(doc.get('meshes', [])))): raise ValueError('unreferenced mesh geometry')
    def accessor(index, kind=None):
        row = indexed(doc['accessors'], index)
        if row.get('sparse') or row.get('normalized'): raise ValueError('unsupported accessor encoding')
        if kind and row['type'] != kind: raise ValueError('accessor type mismatch')
        types = {5121: ('B', 1), 5123: ('H', 2), 5125: ('I', 4), 5126: ('f', 4)}
        code, width = types[row['componentType']]
        elements = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[row['type']]
        view = indexed(doc['bufferViews'], row['bufferView'])
        blob = indexed(buffers, view['buffer'])
        if any(type(v) is not int or v < 0 for v in [view.get('byteOffset', 0), row.get('byteOffset', 0), view['byteLength']]):
            raise ValueError('invalid buffer offset/length')
        start = view.get('byteOffset', 0) + row.get('byteOffset', 0)
        stride = view.get('byteStride', width * elements)
        count = row['count']
        if type(count) is not int or not 0 < count <= 200000 or type(stride) is not int or stride < width * elements:
            raise ValueError('accessor count/stride invalid')
        end = start + (count - 1) * stride + width * elements
        if start < view.get('byteOffset', 0) or end > view.get('byteOffset', 0) + view['byteLength'] or end > len(blob):
            raise ValueError('accessor outside buffer')
        values = [struct.unpack_from('<' + code * elements, blob, start + i * stride) for i in range(count)]
        if any(not all(math.isfinite(v) for v in value) for value in values): raise ValueError('non-finite accessor')
        return values, row['componentType']
    triangles = 0; positions = []; material_slots = set(); skinned = False
    for mesh_index, mesh in enumerate(doc.get('meshes', [])):
        for primitive in mesh.get('primitives', []):
            if primitive.get('targets'): raise ValueError('morph geometry requires verified special-channel host')
            if primitive.get('mode', 4) != 4: raise ValueError('triangle primitives required')
            vertices, component = accessor(primitive['attributes']['POSITION'], 'VEC3')
            if component != 5126: raise ValueError('float positions required')
            positions.extend(vertices)
            if 'indices' in primitive:
                indices, component = accessor(primitive['indices'], 'SCALAR')
                if component not in [5121, 5123, 5125] or any(v[0] >= len(vertices) for v in indices):
                    raise ValueError('invalid triangle indices')
                count = len(indices)
            else: count = len(vertices)
            if count % 3: raise ValueError('incomplete triangle')
            triangles += count // 3
            if 'material' in primitive:
                indexed(doc.get('materials', []), primitive['material'])
                material_slots.add(primitive['material'])
            attrs = primitive['attributes']
            if ('JOINTS_0' in attrs) != ('WEIGHTS_0' in attrs): raise ValueError('incomplete skin attributes')
            if 'JOINTS_0' in attrs:
                joints, joint_type = accessor(attrs['JOINTS_0'], 'VEC4'); weights, weight_type = accessor(attrs['WEIGHTS_0'], 'VEC4')
                if joint_type not in [5121, 5123] or weight_type != 5126: raise ValueError('invalid skin attribute types')
                mesh_nodes = [node for node in nodes if node.get('mesh') == mesh_index]
                if any('skin' not in node for node in mesh_nodes): raise ValueError('skin attributes without node skin')
                if any(max(value) >= len(indexed(doc.get('skins', []), node['skin']).get('joints', [])) for node in mesh_nodes for value in joints):
                    raise ValueError('joint outside skeleton')
                if len(joints) != len(vertices) or len(weights) != len(vertices): raise ValueError('skin vertex count mismatch')
                if any(abs(sum(value) - 1) > 0.001 or min(value) < 0 for value in weights): raise ValueError('invalid skin weights')
                skinned = True
    if not positions or not triangles: raise ValueError('empty normalized geometry')
    mins = [min(p[i] for p in positions) for i in range(3)]
    maxs = [max(p[i] for p in positions) for i in range(3)]
    if abs(mins[1]) > 0.001 or not 0 < maxs[1] - mins[1] <= 20:
        raise ValueError('Y-up metre ground/height calibration invalid')
    bones = 0
    for skin in doc.get('skins', []):
        if not skin.get('joints') or any(type(j) is not int or j < 0 or j >= len(nodes) for j in skin['joints']):
            raise ValueError('invalid skeleton joints')
        bones = max(bones, len(skin['joints']))
    if skinned and not bones: raise ValueError('skin attributes without skeleton')
    animation_names = []; channels = set()
    for animation in doc.get('animations', []):
        name = animation.get('name')
        if not name or name in animation_names or not animation.get('channels'): raise ValueError('unnamed/duplicate/empty animation')
        animation_names.append(name)
        seen = set()
        for channel in animation['channels']:
            target = channel['target']; node = target.get('node'); kind = target.get('path')
            if type(node) is not int or not 0 <= node < len(nodes) or kind not in ['translation', 'rotation', 'scale', 'weights']:
                raise ValueError('unsupported animation channel')
            if (node, kind) in seen or ('matrix' in nodes[node] and kind != 'weights'): raise ValueError('ambiguous animation target')
            seen.add((node, kind)); channels.add(kind)
            sampler = indexed(animation['samplers'], channel['sampler'])
            times, component = accessor(sampler['input'], 'SCALAR')
            if component != 5126 or times[0][0] < 0 or any(a[0] >= b[0] for a, b in zip(times, times[1:])):
                raise ValueError('invalid animation timeline')
            output, output_type = accessor(sampler['output'], {'translation':'VEC3','rotation':'VEC4','scale':'VEC3','weights':'SCALAR'}[kind])
            if output_type != 5126: raise ValueError('float animation output required')
            interpolation = sampler.get('interpolation', 'LINEAR')
            if interpolation not in ['LINEAR', 'STEP', 'CUBICSPLINE']: raise ValueError('unsupported interpolation')
            expected = len(times) * (3 if interpolation == 'CUBICSPLINE' else 1)
            if kind != 'weights' and len(output) != expected: raise ValueError('animation output count mismatch')
            if kind == 'weights': raise ValueError('morph animation requires separate verified host; probe refuses to omit it')
    for image in doc.get('images', []):
        file = local(root, image['uri']); png_dimension(file); files[image['uri']] = digest(file.read_bytes())
    for texture in doc.get('textures', []): indexed(doc.get('images', []), texture['source'])
    for material in doc.get('materials', []):
        texture = material.get('pbrMetallicRoughness', {}).get('baseColorTexture')
        if texture: indexed(doc.get('textures', []), texture['index'])
    return {'triangles': triangles, 'material_slots': max(1, len(material_slots)), 'bones': bones,
            'texture_status': 'inspected_external_png' if doc.get('images') else 'missing_explicit_fallback',
            'bounds_metres': [mins, maxs], 'animations': sorted(animation_names), 'channels': sorted(channels),
            'files_sha256': files, 'normalized_sha256': digest(json.dumps(files, sort_keys=True).encode())}


def validate(path):
    doc = strict(path); root = path.parent
    if doc.get('schema') != 'r14-local-bindings-v1': raise ValueError('local binding schema mismatch')
    results = []; seen = set()
    audit = strict(AUDIT)
    allowed = {key for row in audit['species'] for key in row['exact_model_keys']}
    allowed.update(row['identity'] for row in audit['trainers'])
    for binding in doc['models']:
        key = binding['identity']
        if not isinstance(key, str) or not re.fullmatch(r'(pokemon\.[1-9]\d{0,2}\.[a-z0-9_.]+\.(normal|shiny)|trainer\.pic\.\d{3})', key):
            raise ValueError('invalid exact model identity')
        if key.startswith('pokemon.') and not 1 <= int(key.split('.')[1]) <= 386: raise ValueError('species outside source scope')
        if key not in allowed: raise ValueError('identity/form absent from pinned source manifest')
        if '.spinda.' in key: raise ValueError('Spinda spots require verified material host; retain fallback')
        if key in seen: raise ValueError('duplicate local identity')
        seen.add(key)
        source = checked(root, binding['source']); receipt = strict(checked(root, binding['provenance']))
        if (receipt.get('identity') != key or receipt.get('source_sha256') != digest(source.read_bytes())
            or receipt.get('user_owned_source') is not True or receipt.get('source_family') != 'oras'):
            raise ValueError('source provenance mismatch')
        required = receipt.get('required_special_channels')
        if not isinstance(required, list) or any(c not in SPECIAL for c in required): raise ValueError('special channels must be explicitly inspected')
        if required: raise ValueError('special channel host not yet validated; retain explicit fallback')
        if binding.get('intermediate_units') != 'metres' or binding.get('intermediate_up_axis') != 'Y':
            raise ValueError('glTF axes/units must be declared correctly')
        lods = [gltf_probe(checked(root, record)) for record in binding['lods']]
        if len(lods) != 3: raise ValueError('three LODs required')
        counts = [lod['triangles'] for lod in lods]
        if counts != sorted(counts, reverse=True) or any(c > cap for c, cap in zip(counts, [20000,10000,5000])):
            raise ValueError('LOD triangle budget exceeded')
        if max(lod['material_slots'] for lod in lods) > 4 or max(lod['bones'] for lod in lods) > 128: raise ValueError('mobile rig/material budget exceeded')
        mapping = binding['animation_semantics']
        if set(mapping) != set(SEMANTICS): raise ValueError('complete animation semantic audit required')
        for semantic, name in mapping.items():
            if name is not None and name not in lods[0]['animations']: raise ValueError('animation clip missing from inspected model')
        scale = binding['engine_scale']; ground = binding['engine_ground_offset_cm']
        if type(scale) not in [int,float] or not math.isfinite(scale) or not 0.01 <= scale <= 100:
            raise ValueError('invalid calibrated scale')
        if type(ground) not in [int,float] or not math.isfinite(ground) or abs(ground) > 300:
            raise ValueError('invalid ground offset')
        results.append({'identity':key, 'source_sha256':digest(source.read_bytes()), 'lods':lods,
                        'animation_semantics':mapping, 'missing_animations':[s for s,n in mapping.items() if n is None],
                        'status':'PREIMPORT_VERIFIED', 'unreal_import_validated':False,
                        'engine_scale':scale,'engine_ground_offset_cm':ground})
    return {'schema':'r14-local-audit-v1','models':results,'verified_unreal_imports':0}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('bindings',type=Path); parser.add_argument('output',type=Path)
    args = parser.parse_args()
    try: report = validate(args.bindings)
    except (ValueError, KeyError, IndexError, TypeError) as error: raise SystemExit(f'R14 local audit failed: {error}')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,ensure_ascii=False,sort_keys=True,indent=2)+'\n')
