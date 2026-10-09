#!/usr/bin/env python3
"""Validate deferred Android coverage metadata; never contact or execute a device."""
import configparser
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
SCENARIOS = {'cold_start_save_load', 'movement_connection_warp', 'story_script_continuation',
             'save_store_reload', 'encounter_battle_win_loss', 'ui_touch_controller_focus',
             'character_environment_fallback', 'cry_sfx_bgm_mix', 'background_foreground_save',
             'rtc_clock_boundary'}
MEASUREMENTS = {'cpu_gpu_frame_time_p50_p95_p99', 'peak_resident_memory',
                'thermal_sustained_20min', 'save_commit_latency_and_reload_integrity',
                'audio_dropouts_and_mix', 'crash_anr_and_lifecycle_trace'}


def validate(matrix):
    if (matrix.get('schema') != 'remaster.r19.deferred-android-matrix' or
            matrix.get('version') != 1 or matrix.get('source_commit') !=
            '70db90c9077aed1272e746fc2537d9f12b95a91c'):
        raise ValueError('device metadata schema/source')
    if matrix.get('automatic_execution') is not False or matrix.get('status') != 'DEFERRED_POST_RUNTIME':
        raise ValueError('device matrix must remain deferred')
    if matrix.get('prerequisites') != ['R18_VERIFIED_BUILD_AND_MAIN_CI',
                                      'REAL_UNREAL_RUNTIME_VALIDATION', 'USER_AUTHORIZED_DEVICE_STAGE']:
        raise ValueError('deferred device prerequisite boundary')
    parser = configparser.ConfigParser(interpolation=None, strict=True)
    parser.read(ROOT / 'unreal/Config/DefaultEngine.ini')
    android = parser['/Script/AndroidRuntimeSettings.AndroidRuntimeSettings']
    if (type(matrix.get('minimum_api')) is not int or type(matrix.get('target_api')) is not int or
            matrix['minimum_api'] != int(android['MinSDKVersion']) or
            matrix['target_api'] != int(android['TargetSDKVersion']) or matrix.get('abi') != 'arm64-v8a' or
            not android.getboolean('bBuildForArm64') or android.getboolean('bBuildForX8664') or
            not android.getboolean('bSupportsVulkan') or not android.getboolean('bBuildForES31')):
        raise ValueError('device matrix source configuration drift')
    for field, required in [('scenarios', SCENARIOS), ('measurements', MEASUREMENTS)]:
        if len(matrix[field]) != len(required) or set(matrix[field]) != required:
            raise ValueError('device ' + field + ' coverage')
    targets = matrix.get('targets', [])
    if not targets or len({t['id'] for t in targets}) != len(targets):
        raise ValueError('device target coverage')
    for field, required in [('gpu_family', {'Adreno', 'Mali'}), ('tier', {'low', 'mid', 'high'}),
                            ('backend', {'gles31', 'vulkan'})]:
        if {t[field] for t in targets} != required: raise ValueError('device ' + field + ' coverage')
    apis = {t['api'] for t in targets}
    if not {matrix['minimum_api'], matrix['target_api']} <= apis: raise ValueError('device API coverage')
    for target in targets:
        if target['status'] != 'NOT_RUN': raise ValueError('device target must stay NOT_RUN')
        if any(target.get(k) is not None for k in ['device_model', 'provider', 'result_artifact']):
            raise ValueError('device/provider/results must stay unbound before authorized execution')
        if (type(target['api']) is not int or not matrix['minimum_api'] <= target['api'] <= matrix['target_api'] or
                type(target['minimum_ram_gib']) is not int or target['minimum_ram_gib'] <= 0 or
                len(target['reference_landscape_pixels']) != 2 or
                any(type(n) is not int or n <= 0 for n in target['reference_landscape_pixels']) or
                target['reference_landscape_pixels'][0] <= target['reference_landscape_pixels'][1]):
            raise ValueError('device target constraints')
    return {'status': 'DEFERRED_METADATA_PASS', 'targets': len(targets),
            'scenarios': len(SCENARIOS), 'measurements': len(MEASUREMENTS),
            'devices_executed': 0, 'runtime_verified': False}


if __name__ == '__main__':
    try:
        print(json.dumps(validate(json.loads((ROOT / 'data/r19/android_device_matrix.json').read_text())), sort_keys=True))
    except (ValueError, KeyError, OSError, configparser.Error) as exc:
        print('R19 deferred device metadata failed: ' + str(exc), file=sys.stderr)
        raise SystemExit(1) from exc
