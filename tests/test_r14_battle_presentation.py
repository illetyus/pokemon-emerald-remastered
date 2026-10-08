import copy
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from build_r14_battle_package import build, dump
from validate_r14_local_assets import validate, gltf_probe, digest, strict


class SourceAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.audit, cls.header = build()
        cls.vendor = ROOT / 'vendor/vanillaplus'

    def test_full_independent_species_number_oracle(self):
        constants = dict((n,int(v)) for n,v in re.findall(r'#define\s+SPECIES_(\w+)\s+(\d+)\b', (self.vendor/'include/constants/species.h').read_text()))
        national = re.findall(r'\bNATIONAL_DEX_(\w+)\s*,', (self.vendor/'include/constants/pokedex.h').read_text().split('enum {',1)[1].split('};',1)[0])
        raw = (self.vendor/'src/pokemon.c').read_text().split('static const u16 sSpeciesToNationalPokedexNum',1)[1].split('};',1)[0]
        source_pairs = {(constants[n], national.index(n)) for n in re.findall(r'SPECIES_TO_NATIONAL\((\w+)\)', raw) if national.index(n)<=386}
        audit_pairs = {(r['core_species'],r['national_dex']) for r in self.audit['species']}
        self.assertEqual(source_pairs,audit_pairs)
        self.assertEqual(len(audit_pairs),386)
        self.assertEqual({n for _,n in audit_pairs},set(range(1,387)))

    def test_committed_generation_is_exact_and_repeatable(self):
        self.assertEqual((ROOT/'data/r14/species_audit.json').read_text(),dump(self.audit))
        self.assertEqual((ROOT/'unreal/Source/PokemonEmeraldRemastered/RemasterBattleCatalog.inl').read_text(),self.header)
        self.assertEqual(build(),(self.audit,self.header))

    def test_sprite_and_source_hashes_reconcile_raw_bytes(self):
        for path,sha in self.audit['source_hashes'].items():
            self.assertEqual(digest((self.vendor/path).read_bytes()),sha)
        for row in self.audit['species']:
            self.assertEqual(len(row['source_sprite_references']),8 if row['national_dex']==351 else 2)
            for record in row['source_sprite_references']:
                self.assertEqual(digest((self.vendor/record['path']).read_bytes()),record['sha256'])

    def test_all_missing_imports_and_special_channels_are_explicit(self):
        self.assertEqual(self.audit['counts']['missing_owned_3d_models'],386)
        self.assertEqual(self.audit['counts']['verified_unreal_imports'],0)
        for row in self.audit['species']:
            self.assertEqual(row['runtime'],'primitive_fallback')
            self.assertEqual(len(row['animations']),5)
            self.assertTrue(all(v=='missing_explicit_fallback' for v in row['animations'].values()))
            self.assertEqual(row['special_channels'],'unknown_until_source_inspection')
        for key in ['unown','spinda','castform','deoxys','ditto_transform']:
            self.assertIn(key,self.audit['special_cases'])

    def test_all_exact_form_skin_keys_and_source_exceptions(self):
        keys=[key for row in self.audit['species'] for key in row['exact_model_keys']]
        self.assertEqual(len(keys),832);self.assertEqual(len(set(keys)),832)
        self.assertIn('pokemon.201.unown.27.shiny',keys)
        self.assertIn('pokemon.351.snowy.normal',keys)
        self.assertNotIn('pokemon.386.base.normal',keys)
        self.assertIn('pokemon.386.speed.normal',keys)

    def test_event_and_trainer_identity_oracles(self):
        kinds = {int(v) for n,v in re.findall(r'REMASTER_EMERALD_BATTLE_EVENT_(\w+)\s*=\s*(\d+)', (ROOT/'core/include/remaster/emerald_battle.h').read_text().split('REMASTER_EMERALD_BATTLE_EVENT_NONE = 0,',1)[1].split('};',1)[0]) if int(v)}
        self.assertEqual(kinds,set(range(1,26)))
        self.assertEqual(kinds,{e['id'] for e in self.audit['events']})
        raw = (self.vendor/'include/constants/trainers.h').read_text()
        pics = {int(v) for n,v in re.findall(r'#define\s+(TRAINER_PIC_\w+)\s+(\d+)\b',raw)}
        self.assertEqual(pics,{r['trainer_pic'] for r in self.audit['trainers']})

    def test_visual_constants_reconcile_source(self):
        types = (self.vendor/'include/constants/pokemon.h').read_text()
        for name,value in [('TYPE_FIRE',10),('TYPE_WATER',11),('TYPE_ICE',15),('SHINY_ODDS',8)]:
            self.assertRegex(types,rf'#define\s+{name}\s+{value}\b')
        source=(self.vendor/'include/pokemon.h').read_text().split('#define GET_UNOWN_LETTER',1)[1].split('#define GET_SHINY_VALUE',1)[0]
        self.assertIn('0x03000000',source);self.assertIn('0x00000300',source)

    def test_production_authority_and_load_lifecycle_guards(self):
        module=ROOT/'unreal/Source/PokemonEmeraldRemastered'
        files=[module/n for n in ['RemasterBattleRead.h','RemasterBattleStage.cpp','RemasterBattlePresentationSubsystem.cpp']]
        text='\n'.join(p.read_text() for p in files)
        for operation in ['state_init','start','resolve_turn','use_move','switch','replace_fainted','calculate_damage','choose_ai_action','throw_ball','try_run','clear_events','finalize_trainer']:
            self.assertNotRegex(text,rf'remaster_emerald_battle_{operation}\s*\(')
        self.assertNotIn('BlueprintCallable', (module/'RemasterBattlePresentationSubsystem.h').read_text().split('bool BeginCoreBattle')[0].split('public:')[-1])
        stage=files[1].read_text()
        for guard in ['Generation != LoadGeneration[Slot]', 'Key != ModelKeys[Slot]', 'CancelHandle()', 'CreateWeakLambda', 'RequiredSpecialChannels.IsEmpty()', 'bUnrealImportValidated', 'ECollisionEnabled::NoCollision']:
            self.assertIn(guard,stage)
        tick=stage.split('void ARemasterBattleStage::Tick',1)[1].split('void ARemasterBattleStage::ResetPresentation',1)[0]
        self.assertIn('CompletePresentation(',tick);self.assertNotIn('SetBattleHostBusy',tick)


class LocalProbe(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name)
        def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
        png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1,1,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b'\x00\xff\xff\xff\xff'))+chunk(b'IEND',b'')
        (self.root/'texture.png').write_bytes(png)
        # Original, redistributable test triangle and translation clip; no game payload.
        blob=struct.pack('<9f',-.5,0,0,.5,0,0,0,1,0)+struct.pack('<2f',0,1)+struct.pack('<6f',0,0,0,0,.1,0)
        (self.root/'mesh.bin').write_bytes(blob)
        self.gltf={'asset':{'version':'2.0'},'buffers':[{'uri':'mesh.bin','byteLength':len(blob)}],
            'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':36},{'buffer':0,'byteOffset':36,'byteLength':8},{'buffer':0,'byteOffset':44,'byteLength':24}],
            'accessors':[{'bufferView':0,'componentType':5126,'count':3,'type':'VEC3'},{'bufferView':1,'componentType':5126,'count':2,'type':'SCALAR'},{'bufferView':2,'componentType':5126,'count':2,'type':'VEC3'}],
            'nodes':[{'mesh':0}], 'meshes':[{'primitives':[{'attributes':{'POSITION':0},'material':0}]}],
            'materials':[{'pbrMetallicRoughness':{'baseColorTexture':{'index':0}}}], 'textures':[{'source':0}], 'images':[{'uri':'texture.png'}],
            'animations':[{'name':'Idle','samplers':[{'input':1,'output':2}],'channels':[{'sampler':0,'target':{'node':0,'path':'translation'}}]}]}
        (self.root/'owned.model').write_bytes(b'original synthetic source, not ORAS payload')
        self.receipt={'identity':'pokemon.1.base.normal','source_sha256':digest((self.root/'owned.model').read_bytes()),'user_owned_source':True,'source_family':'oras','required_special_channels':[]}
        self.binding={'identity':self.receipt['identity'],'source':{},'provenance':{},'lods':[], 'intermediate_units':'metres','intermediate_up_axis':'Y',
            'engine_scale':1,'engine_ground_offset_cm':0,'animation_semantics':{'idle':'Idle','entry':None,'attack':None,'hit':None,'faint':None}}
        self.save()

    def save(self):
        def record(name):return {'path':name,'sha256':digest((self.root/name).read_bytes())}
        (self.root/'model.gltf').write_text(json.dumps(self.gltf))
        (self.root/'receipt.json').write_text(json.dumps(self.receipt))
        self.binding.update(source=record('owned.model'),provenance=record('receipt.json'),lods=[record('model.gltf')]*3)
        (self.root/'bindings.json').write_text(json.dumps({'schema':'r14-local-bindings-v1','models':[self.binding]}))

    def rejects(self):
        self.save()
        with self.assertRaises((ValueError,KeyError,TypeError)): validate(self.root/'bindings.json')

    def test_real_geometry_timeline_texture_and_fallback_audit(self):
        report=validate(self.root/'bindings.json');model=report['models'][0]
        self.assertEqual(model['status'],'PREIMPORT_VERIFIED');self.assertFalse(model['unreal_import_validated'])
        self.assertEqual(report['verified_unreal_imports'],0)
        self.assertEqual(model['missing_animations'],['entry','attack','hit','faint'])
        self.assertEqual(model['lods'][0]['triangles'],1)
        self.assertEqual(model['lods'][0]['bounds_metres'],[[-.5,0.,0.],[.5,1.,0.]])
        self.assertEqual(model['lods'][0]['texture_status'],'inspected_external_png')

    def test_source_hash_tampering(self):
        (self.root/'owned.model').write_bytes(b'altered')
        with self.assertRaises(ValueError):validate(self.root/'bindings.json')

    def test_unverified_form_and_spinda_binding_rejected(self):
        for key in ['pokemon.386.base.normal','pokemon.201.base.normal','pokemon.327.spinda.normal']:
            self.binding['identity']=self.receipt['identity']=key;self.rejects()

    def test_owned_receipt_and_identity_required(self):
        for field,value in [('user_owned_source',False),('identity','pokemon.2.base.normal'),('source_family','unknown')]:
            original=self.receipt[field];self.receipt[field]=value;self.rejects();self.receipt[field]=original

    def test_special_channels_never_silently_omitted(self):
        for channel in ['material','visibility','morph','UV','texture_pattern']:
            self.receipt['required_special_channels']=[channel];self.rejects()
        del self.receipt['required_special_channels'];self.rejects()

    def test_unknown_extensions_and_morph_geometry_rejected(self):
        self.gltf['extensionsUsed']=['KHR_materials_unlit'];self.rejects();del self.gltf['extensionsUsed']
        self.gltf['meshes'][0]['primitives'][0]['targets']=[{'POSITION':0}];self.rejects()

    def test_negative_and_out_of_range_references_rejected(self):
        paths=[('accessor',),('view',),('buffer',),('material',),('sampler',),('texture',),('mesh',)]
        for path in paths:
            original=copy.deepcopy(self.gltf)
            kind=path[0]
            if kind=='accessor':self.gltf['meshes'][0]['primitives'][0]['attributes']['POSITION']=-1
            if kind=='view':self.gltf['accessors'][0]['bufferView']=-1
            if kind=='buffer':self.gltf['bufferViews'][0]['buffer']=-1
            if kind=='material':self.gltf['meshes'][0]['primitives'][0]['material']=-1
            if kind=='sampler':self.gltf['animations'][0]['channels'][0]['sampler']=-1
            if kind=='texture':self.gltf['textures'][0]['source']=-1
            if kind=='mesh':self.gltf['nodes'][0]['mesh']=-1
            self.rejects();self.gltf=original

    def test_buffer_bounds_and_nonfinite_geometry(self):
        self.gltf['accessors'][0]['count']=4;self.rejects();self.gltf['accessors'][0]['count']=3
        blob=bytearray((self.root/'mesh.bin').read_bytes());struct.pack_into('<f',blob,0,float('nan'));(self.root/'mesh.bin').write_bytes(blob)
        self.rejects()

    def test_mesh_parent_transform_and_node_cycle(self):
        self.gltf['nodes'].append({'children':[0],'translation':[0,1,0]});self.rejects()
        self.gltf['nodes']=[{'mesh':0,'children':[1]},{'children':[0]}];self.rejects()

    def test_invalid_timeline_and_animation_binding(self):
        blob=bytearray((self.root/'mesh.bin').read_bytes());struct.pack_into('<2f',blob,36,1,1);(self.root/'mesh.bin').write_bytes(blob);self.rejects()
        struct.pack_into('<2f',blob,36,0,1);(self.root/'mesh.bin').write_bytes(blob)
        self.binding['animation_semantics']['attack']='missing';self.rejects()

    def test_ground_units_budget_and_calibration(self):
        self.binding['intermediate_units']='centimetres';self.rejects();self.binding['intermediate_units']='metres'
        self.binding['engine_scale']=float('nan');self.rejects();self.binding['engine_scale']=1
        self.gltf['meshes'][0]['primitives']*=20001;self.rejects()

    def test_path_escape_and_symlink_rejected(self):
        for path in ['../outside.bin','https://example.com/a','%2e%2e/outside.bin']:
            self.gltf['buffers'][0]['uri']=path;self.rejects()
        (self.root/'escape.bin').symlink_to('/etc/hosts');self.gltf['buffers'][0]['uri']='escape.bin';self.rejects()

    def test_duplicate_json_and_identity_rejected(self):
        (self.root/'bad.json').write_text('{"x":1,"x":2}')
        with self.assertRaises(ValueError):strict(self.root/'bad.json')
        doc=json.loads((self.root/'bindings.json').read_text());doc['models']*=2;(self.root/'bindings.json').write_text(json.dumps(doc))
        with self.assertRaises(ValueError):validate(self.root/'bindings.json')

    def test_missing_texture_is_explicit_not_import_success(self):
        self.gltf.pop('images');self.gltf.pop('textures');self.gltf['materials']=[{}];self.save()
        report=validate(self.root/'bindings.json')
        self.assertEqual(report['models'][0]['lods'][0]['texture_status'],'missing_explicit_fallback')
        self.assertEqual(report['verified_unreal_imports'],0)

    def test_invalid_skin_is_not_accepted_as_skeletal(self):
        attrs=self.gltf['meshes'][0]['primitives'][0]['attributes'];attrs['JOINTS_0']=0;self.rejects()
        attrs['WEIGHTS_0']=0;self.rejects()
        self.gltf['skins']=[{'joints':[-1]}];self.rejects()

    def test_oversized_png_and_corrupt_crc_rejected(self):
        raw=bytearray((self.root/'texture.png').read_bytes());raw[20]^=1;(self.root/'texture.png').write_bytes(raw);self.rejects()


if __name__=='__main__':unittest.main()
