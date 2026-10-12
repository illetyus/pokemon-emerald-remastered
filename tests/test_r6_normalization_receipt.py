from pathlib import Path
import hashlib,json,tempfile,unittest
from tools.normalize_r6_character import validate_receipt, NormalizationError

class R6NormalizationReceiptTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        for name in ['mesh.fbx','idle.fbx','walk.fbx']:
            (self.root/name).write_bytes(('actual-output-'+name).encode())
        self.r={
            'schema_version':1,'model_id':'character.brendan.oras_overworld',
            'source_archive_sha256':'a'*64,'source_member_sha256':'b'*64,
            'target_unit':'centimeter','target_up_axis':'Z','target_forward_axis':'X',
            'root_motion_enabled':False,'animation_origin':'project_authored',
            'material_slots':3,'max_weights_per_vertex':4,
            'height_cm':83.793,'foot_min_cm':0.0,
            'mesh':'mesh.fbx',
            'outputs':{n:hashlib.sha256((self.root/n).read_bytes()).hexdigest() for n in ['mesh.fbx','idle.fbx','walk.fbx']},
            'clips':{
                'idle':{'file':'idle.fbx','frame_count':61,'sample_rate':30,'changed_tracks':2,'loop_closed':True,'root_xy_displacement_cm':0.0},
                'walk':{'file':'walk.fbx','frame_count':25,'sample_rate':30,'changed_tracks':8,'loop_closed':True,'root_xy_displacement_cm':0.0},
            },
            'unreal_import_validated':False,'packaged_runtime_validated':False,
        }
    def test_accepts_actual_files_without_promoting_engine_or_runtime(self):
        actual=validate_receipt(self.r,self.root)
        self.assertFalse(actual['unreal_import_validated'])
        self.assertFalse(actual['packaged_runtime_validated'])
    def test_changed_or_missing_output_is_rejected(self):
        (self.root/'walk.fbx').write_bytes(b'changed')
        with self.assertRaises(NormalizationError): validate_receipt(self.r,self.root)
        (self.root/'walk.fbx').unlink()
        with self.assertRaises(NormalizationError): validate_receipt(self.r,self.root)
    def test_reference_pose_cannot_claim_walk_clip(self):
        self.r['clips']['walk']['changed_tracks']=0
        with self.assertRaises(NormalizationError): validate_receipt(self.r,self.root)
    def test_nonlooping_root_motion_or_wrong_axes_are_rejected(self):
        for field,value in [('root_motion_enabled',True),('target_unit','inch'),('target_forward_axis','Y')]:
            data=json.loads(json.dumps(self.r));data[field]=value
            with self.assertRaises(NormalizationError): validate_receipt(data,self.root)
        self.r['clips']['walk']['root_xy_displacement_cm']=10.0
        with self.assertRaises(NormalizationError): validate_receipt(self.r,self.root)
    def test_material_weight_and_ground_contract_is_bounded(self):
        for field,value in [('material_slots',5),('max_weights_per_vertex',5),('foot_min_cm',-9.0),('height_cm',float('nan'))]:
            data=json.loads(json.dumps(self.r));data[field]=value
            with self.assertRaises(NormalizationError): validate_receipt(data,self.root)
    def test_output_path_escape_is_rejected(self):
        self.r['outputs']['../escape.fbx']='c'*64
        with self.assertRaises(NormalizationError): validate_receipt(self.r,self.root)
    def test_missing_loop_or_wrong_source_hash_is_rejected(self):
        self.r['clips']['walk']['loop_closed']=False
        with self.assertRaises(NormalizationError): validate_receipt(self.r,self.root)
        self.r['clips']['walk']['loop_closed']=True
        self.r['source_archive_sha256']='short'
        with self.assertRaises(NormalizationError): validate_receipt(self.r,self.root)
    def test_malformed_schema_types_fail_with_validation_error(self):
        for field,value in [('schema_version',True),('model_id',None),('source_member_sha256',[]),('clips',[]),('mesh',{})]:
            data=json.loads(json.dumps(self.r));data[field]=value
            with self.subTest(field=field):
                with self.assertRaises(NormalizationError): validate_receipt(data,self.root)
        self.r['outputs']['mesh.fbx']=None
        with self.assertRaises(NormalizationError): validate_receipt(self.r,self.root)
if __name__=='__main__': unittest.main()
