#!/usr/bin/env python3
"""Produce resumable per-model preparation jobs from pinned public evidence."""
from __future__ import annotations
import json
from pathlib import Path
import sys

try:
    from tools.build_r6_character_manifest import build_manifest, strict_json_loads
except ModuleNotFoundError:
    from build_r6_character_manifest import build_manifest, strict_json_loads


def build_plan(root: Path) -> dict:
    data = root / 'data/r6'
    manifest = build_manifest(root / 'vendor/vanillaplus', data / 'character_presentation_overrides.json')
    selections = strict_json_loads((data / 'npc_presentation_plan.json').read_text())['records']
    preflight = strict_json_loads((data / 'npc_source_preflight.json').read_text())
    packages = {p['asset_id']:p for p in preflight['packages']}
    jobs = {}
    for entry in manifest['entries']:
        if entry['source_family'] == 'project_placeholder':
            continue
        model_id = entry['model_id']
        if model_id not in jobs:
            jobs[model_id] = {'model_id':model_id,'source_sha256':entry['source_sha256'],
                'provenance_id':entry['provenance_id'],'graphics_ids':[],
                'normalization_status':'pending','normalized_sha256':[],
                'source_submodel_file':None,'source_submodel_sha256':None,
                'skeleton_family':entry['skeleton_family'],
                'retarget_compatibility_status':'unverified','family_compatible_reuse_allowed':False,
                'material_profile':'material.mobile_character','lod_profile':entry['lod_profile'],
                'calibration_status':'placeholder_defaults_uncalibrated',
                'animation_clip_status':'missing','unreal_import_status':'untested','android_status':'untested'}
        jobs[model_id]['graphics_ids'].append(entry['graphics_id'])
    for selection in selections:
        if selection['decision'] != 'selected_source':
            continue
        job = jobs[selection['model_id']]
        package = packages[selection['source_asset_id']]
        model = next(m for m in package['models'] if m['source_submodel_file'] == selection['source_submodel_file'])
        if model['source_submodel_sha256'] != selection['source_submodel_sha256']:
            raise ValueError('selected submodel hash does not match source preflight')
        job.update(source_asset_id=selection['source_asset_id'],
                   source_submodel_file=selection['source_submodel_file'],
                   source_submodel_sha256=selection['source_submodel_sha256'],
                   source_material_slots=model['material_count'],
                   source_joint_count=model['joint_count'],
                   source_scene_sha256=model['source_scene_sha256'],
                   proposed_material_ids=[selection['model_id']+'.material.'+str(i) for i in range(model['material_count'])],
                   material_budget_requires_review=model['material_count']>4)
    return {'schema_version':1,'model_count':len(jobs),
            'outputs_available':0,'status':'preparation_jobs_only_no_conversion_claim',
            'player_source_selection':'use_existing_I2B_private_audit_to_confirm_base_member_before_export',
            'jobs':[jobs[key] for key in sorted(jobs)]}


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: build_r6_normalization_plan.py <output.json>')
    output = Path(sys.argv[1]);output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(build_plan(Path(__file__).resolve().parents[1]),indent=2,sort_keys=True)+'\n')
