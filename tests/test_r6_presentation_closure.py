from __future__ import annotations
import hashlib
import io
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from tools.build_r6_character_manifest import ManifestError, build_manifest, resolve_entry, strict_json_loads
from tools.build_r6_character_package import build_package, verify_package
from tools.build_r6_normalization_plan import build_plan
from tools.inspect_r6_character_archive import inspect_archive

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'data/r6'


class R6PresentationClosureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest=build_manifest(ROOT/'vendor/vanillaplus',DATA/'character_presentation_overrides.json')
        cls.entries={r['graphics_name']:r for r in cls.manifest['entries']}
        cls.plan=json.loads((DATA/'npc_presentation_plan.json').read_text())
        cls.records={r['graphics_name']:r for r in cls.plan['records']}
        cls.preflight=json.loads((DATA/'npc_source_preflight.json').read_text())
        cls.packages={p['asset_id']:p for p in cls.preflight['packages']}

    def test_all_121_npcs_and_18_players_have_distinct_source_decisions(self):
        self.assertEqual(len(self.records),121)
        self.assertEqual(self.plan['decision_counts'],{'generic_source_deferred':45,'intentional_player_base_reuse':12,'legacy_secondary_source_deferred':13,'selected_source':46,'source_gap':5})
        human={r['graphics_name'] for r in self.manifest['entries'] if r['presentation_kind']=='human'}
        players={r['graphics_name'] for r in json.loads((DATA/'player_presentation_states.json').read_text())['records']}
        self.assertEqual(human,set(self.records)|players)
        self.assertFalse(set(self.records)&players)

    def test_pinned_43_package_49_model_preflight_matches_acquisition(self):
        acquisition=json.loads((DATA/'npc_asset_acquisition.json').read_text())
        self.assertEqual(len(self.packages),43)
        self.assertEqual(sum(p['model_count'] for p in self.packages.values()),49)
        self.assertEqual(self.plan['source_preflight_sha256'],hashlib.sha256((DATA/'npc_source_preflight.json').read_bytes()).hexdigest())
        for source in acquisition['packages']:
            package=self.packages[source['asset_id']]
            self.assertEqual(package['source_archive_sha256'],source['source_archive_sha256'])
            self.assertTrue(package['zip_integrity_verified'])
            self.assertTrue(package['private_provenance_record_verified'])
            self.assertEqual(package['model_count'],source['model_count'])
            for model in package['models']:
                self.assertGreater(model['skin_controller_count'],0)
                self.assertGreater(model['joint_count'],0)
                self.assertEqual(model['embedded_animation_count'],0)
                self.assertFalse(model['family_compatible_reuse_allowed'])

    def test_every_selection_has_exact_package_and_member_hash(self):
        for record in self.records.values():
            entry=self.entries[record['graphics_name']]
            self.assertEqual(entry['graphics_id'],record['graphics_id'])
            self.assertEqual(entry['model_id'],record['model_id'])
            if record['decision']=='selected_source':
                package=self.packages[record['source_asset_id']]
                member=next(m for m in package['models'] if m['source_submodel_file']==record['source_submodel_file'])
                self.assertEqual(record['source_submodel_sha256'],member['source_submodel_sha256'])
                self.assertEqual(entry['source_sha256'],[package['source_archive_sha256']])
            self.assertEqual(entry['normalized_sha256'],[])
            self.assertEqual(entry['fallback_id'],'fallback.human')

    def test_multi_model_decisions_are_explicit_not_sorted_file_guessing(self):
        expected={'LIZA':'rstr0024_00_fi.dae','TATE':'rstr0023_00_fi.dae','REPORTER_F':'rstr0147_00_fi.dae','CAMERAMAN':'rstr0148_00_fi.dae','WALLY':'rstr0010_00_fi.dae','YOUNGSTER':'rstr0101_00_fi.dae','RUNNING_TRIATHLETE_M':'rstr0140_00_fi.dae'}
        for suffix, member in expected.items():
            record=self.records['OBJ_EVENT_GFX_'+suffix]
            self.assertEqual(record['source_submodel_file'],member)
            self.assertTrue(record['reason'].startswith(('visual_inspection','project_policy')))
        self.assertNotEqual(self.entries['OBJ_EVENT_GFX_LIZA']['model_id'],self.entries['OBJ_EVENT_GFX_TATE']['model_id'])

    def test_source_gaps_never_borrow_other_gender_or_named_model(self):
        for suffix in ['REPORTER_M','SWIMMER_M','RUNNING_TRIATHLETE_F','CYCLING_TRIATHLETE_F','CYCLING_TRIATHLETE_M']:
            record=self.records['OBJ_EVENT_GFX_'+suffix]
            self.assertEqual(record['decision'],'source_gap')
            self.assertEqual(record['model_id'],'fallback.human')
        for record in self.records.values():
            if record['decision'] in ['generic_source_deferred','legacy_secondary_source_deferred']:
                self.assertEqual(record['model_id'],'fallback.human')
                self.assertFalse(record['family_compatible_reuse_allowed'])

    def test_explicit_base_reuse_does_not_assert_state_prop_or_retarget_readiness(self):
        for record in self.records.values():
            if record['decision']=='intentional_player_base_reuse':
                self.assertEqual(record['normalized_sha256'],[])
                self.assertEqual(record['animation_clip_status'],'missing')
                self.assertFalse(record['family_compatible_reuse_allowed'])
        self.assertEqual(self.records['OBJ_EVENT_GFX_TUBER_M_SWIMMING']['model_id'],self.records['OBJ_EVENT_GFX_TUBER_M']['model_id'])

    def test_animation_contract_keeps_all_five_semantics_and_empty_verified_clips(self):
        contract=json.loads((DATA/'animation_contract.json').read_text())
        self.assertEqual(contract['semantics'],['idle','walk','run','turn','interact'])
        self.assertEqual(contract['available_clips'],[])
        self.assertEqual(contract['verified_shared_retarget_pairs'],[])
        self.assertEqual(contract['root_motion_policy'],'disabled_presentation_never_moves_gameplay')

    def test_preparation_plan_is_deterministic_and_has_47_unique_local_jobs(self):
        plan=build_plan(ROOT)
        self.assertEqual(plan,build_plan(ROOT))
        self.assertEqual(plan['model_count'],47)
        self.assertEqual(plan['outputs_available'],0)
        self.assertEqual(sum(len(j['graphics_ids']) for j in plan['jobs']),76)
        for job in plan['jobs']:
            self.assertEqual(job['normalized_sha256'],[])
            self.assertEqual(job['calibration_status'],'placeholder_defaults_uncalibrated')
            self.assertFalse(job['family_compatible_reuse_allowed'])

    def test_package_is_byte_deterministic_and_all_681_defaults_visible(self):
        with tempfile.TemporaryDirectory() as directory:
            a=Path(directory)/'a'; b=Path(directory)/'b'
            audit=build_package(ROOT,a);build_package(ROOT,b)
            self.assertEqual(audit['human_source_selected'],76)
            self.assertEqual(audit['human_explicit_source_gaps_or_deferred'],63)
            self.assertEqual(audit['identity_count'],681)
            for name in ['manifest.json','manifest.sha256','coverage.json']:
                self.assertEqual((a/name).read_bytes(),(b/name).read_bytes())
            self.assertTrue(all(r['runtime_default']=='visible_placeholder' for r in audit['records']))
            raw=(a/'manifest.json').read_text().replace('npc.prof_birch','npc.wrong_birch')
            (a/'manifest.json').write_text(raw)
            with self.assertRaisesRegex(ManifestError,'byte SHA-256 mismatch'):verify_package(a)
            (a/'manifest.sha256').write_text(hashlib.sha256(raw.encode()).hexdigest())
            with self.assertRaisesRegex(ManifestError,'canonical content'):verify_package(a)

    def test_resolver_rejects_boolean_fraction_and_negative_identity(self):
        for bad in [True,False,64.0,64.5,'64',-1,None]:
            self.assertIsNone(resolve_entry(self.manifest,bad))

    def test_staged_aggregate_metadata_cannot_disagree_with_records(self):
        with tempfile.TemporaryDirectory() as directory:
            output=Path(directory)
            build_package(ROOT,output)
            manifest=json.loads((output/'manifest.json').read_text())
            manifest['kind_counts']['human']=140
            raw=(json.dumps(manifest)+'\n').encode()
            (output/'manifest.json').write_bytes(raw)
            (output/'manifest.sha256').write_text(hashlib.sha256(raw).hexdigest())
            with self.assertRaisesRegex(ManifestError,'aggregate counts'):verify_package(output)

    def test_nonfinite_numeric_overrides_and_missing_provenance_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'input.json'
            for field,value in [('scale',float('nan')),('ground_offset_cm',float('inf')),('yaw_offset_deg',1e9),('scale',True),('scale',0)]:
                path.write_text(json.dumps({'schema_version':1,'overrides':{'OBJ_EVENT_GFX_BRENDAN_NORMAL':{field:value}}}))
                with self.assertRaises(ManifestError):build_manifest(ROOT/'vendor/vanillaplus',path)
            path.write_text(json.dumps({'schema_version':1,'overrides':{'OBJ_EVENT_GFX_BRENDAN_NORMAL':{'source_family':'oras'}}}))
            with self.assertRaisesRegex(ManifestError,'hash and provenance'):build_manifest(ROOT/'vendor/vanillaplus',path)

    def test_duplicate_json_keys_are_rejected(self):
        with self.assertRaisesRegex(ManifestError,'duplicate JSON key'):
            strict_json_loads('{"schema_version":1,"schema_version":2}')

    def test_archive_preflight_hashes_bytes_and_rejects_unsafe_members(self):
        dae=b'<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema"><asset><up_axis>Z_UP</up_axis></asset></COLLADA>'
        def archive(name):
            buffer=io.BytesIO()
            with zipfile.ZipFile(buffer,'w') as z:z.writestr(name,dae)
            return buffer.getvalue()
        raw=archive('safe/model.dae')
        report=inspect_archive(raw,hashlib.sha256(raw).hexdigest())
        self.assertEqual(report['models'][0]['source_submodel_sha256'],hashlib.sha256(dae).hexdigest())
        self.assertEqual(report,inspect_archive(raw))
        with self.assertRaisesRegex(ValueError,'mismatch'):inspect_archive(raw,'0'*64)
        for name in ['../escape.dae','/absolute.dae','C:\\bad.dae']:
            with self.assertRaisesRegex(ValueError,'unsafe'):inspect_archive(archive(name))

    def test_public_metadata_has_no_local_paths_or_payloads(self):
        for filename in ['npc_presentation_plan.json','npc_source_preflight.json','animation_contract.json','normalization_contract.json']:
            raw=(DATA/filename).read_text()
            for token in ['pokemon-emerald-remastered-assets','R6/NPCs/source/','/home/','/Users/','/workspace/','/mnt/','/tmp/','BEGIN PRIVATE','token=']:
                self.assertNotIn(token,raw)


if __name__=='__main__':unittest.main()
