import hashlib,importlib.util,json,re,subprocess
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1];M=ROOT/'unreal/Source/PokemonEmeraldRemastered'
spec=importlib.util.spec_from_file_location('truck',ROOT/'tools/build_ap04_truck_oracle.py');truck=importlib.util.module_from_spec(spec);spec.loader.exec_module(truck)
class SessionSourceTests(unittest.TestCase):
    def test_all_original_truck_functions_remain_unchanged(self):
        rendered,receipt=truck.generate(ROOT)
        original=subprocess.check_output(['git','-C',str(ROOT),'show',truck.init.prepare.SOURCE_TREE+':src/field_special_scene.c']).decode()
        for name in truck.FUNCTIONS:
            body=truck.init.prepare.function(original,name);self.assertIn(body,rendered)
            self.assertEqual(hashlib.sha256(body.encode()).hexdigest(),receipt['OriginalFunctions'][name])
        self.assertEqual(len(receipt['OriginalFunctions']),8)
        self.assertFalse(receipt['GPUVerified']);self.assertFalse(receipt['SourceABIVerified'])
    def test_saved_selection_is_bounded_explicit_and_separate_from_raw_import(self):
        code=(M/'RemasterVanillaPlusSaveSubsystem.cpp').read_text(encoding='utf-8')
        self.assertIn('Bytes[256]',code);self.assertIn('ValidSessionSlot',code)
        self.assertIn('static_cast<RemasterEmeraldSaveFormat>(ActiveFormat)',code)
        self.assertIn('RemasterSaveImport::CopyFirst(',code);self.assertIn('quarantine-import-',code)
        self.assertIn('RemasterSaveImport::WriteExclusive(Path(WorkingCopy)',code)
        self.assertLess(code.index('PersistSessionSelection(Slot,Format)'),code.index('ActiveSlot=Slot;ActiveFormat=Format;'))
    def test_world_and_frontend_require_real_session_readiness(self):
        ui=(M/'RemasterUISubsystem.cpp').read_text(encoding='utf-8')
        world=(M/'RemasterWorldGameplaySubsystem.cpp').read_text(encoding='utf-8')
        session=(M/'RemasterGameSessionSubsystem.cpp').read_text(encoding='utf-8')
        self.assertIn('Context.mapReady=Context.mapReady && Session->IsPlaying()',ui)
        step=world.split('bool URemasterWorldGameplaySubsystem::StepPlayer(',1)[1].split('\n}',1)[0]
        self.assertIn('if(!Session->IsPlaying())return false;',step)
        fieldmap=(ROOT/'vendor/vanillaplus/include/fieldmap.h').read_text()
        self.assertIn('#define NUM_TILES_IN_PRIMARY 512',fieldmap);self.assertIn('#define NUM_PALS_IN_PRIMARY 6',fieldmap)
        source_map=json.loads((ROOT/'vendor/vanillaplus/data/maps/InsideOfTruck/map.json').read_text())
        self.assertEqual(source_map['id'],'MAP_INSIDE_OF_TRUCK')
        self.assertIn('RemasterSession::IsTruckMap(',session);self.assertIn('RemasterSession::IsTruckMap(',world)
        self.assertIn('Session.FinishIntro(',session);self.assertIn('Session.CompleteIntro(',session)
        widget=(M/'RemasterUiWidget.cpp').read_text(encoding='utf-8')
        self.assertIn('Frame.bModal && Frame.ScreenId!=1004',widget)
        self.assertIn('FLinearColor::Transparent',widget)

        self.assertLess(session.index('SetDoor(true) || !Session.CompleteIntro'),session.index('AP04_TRUCK_INTRO_COMPLETE'))
    def test_same_widget_publication_fence_protects_both_owners_and_editor_focus(self):
        ui=(M/'RemasterUISubsystem.cpp').read_text(encoding='utf-8')
        self.assertIn('!FrameFence.Matches(ExpectedRevision)',ui)
        self.assertIn('PresentedFrame.Revision = static_cast<int64>(FrameFence.revision)',ui)
        self.assertIn('PresentedFrame.Revision=static_cast<int64>(FrameFence.revision)',ui)
        self.assertIn('Widget->HasSessionTextFocus()',ui);self.assertIn('LastTextCommitFrame==GFrameCounter',ui)
    def test_hardware_clock_and_source_projection_boundaries_remain_explicit(self):
        code=(M/'RemasterVanillaPlusSaveSubsystem.cpp').read_text(encoding='utf-8')
        self.assertIn('remaster_emerald_rtc_realign_now(Candidate.Get()',code)
        self.assertIn('remaster_emerald_save_set_last_berry_update(Candidate.Get(),BerryTime)',code)
        self.assertNotIn('SetSystemTime(',code);self.assertNotIn('SetLocalTime(',code)
    def test_runtime_test_is_development_only_and_labels_input_scope(self):
        code=(M/'RemasterSessionSmoke.cpp').read_text(encoding='utf-8')
        self.assertIn('#if !UE_BUILD_SHIPPING',code);self.assertIn('PhysicalInputVerified"),false',code)
        self.assertIn('Button->OnClicked.Broadcast()',code)
        self.assertIn('FPaths::ProjectSavedDir()',code)
        self.assertIn('REMASTER_AP04_INPUT_SHA',code)
        # Exercise the same localized targets the real UMG smoke driver uses.
        # Encoding drift makes it miss visible controls while ASCII tests pass.
        production='\n'.join((M/name).read_text(encoding='utf-8') for name in
            ('RemasterUiWidget.cpp','RemasterSessionMenu.h','RemasterUiModel.h','RemasterUiModel.cpp'))
        for label in ('Adın','Yolculuğa Başla','Kayıt / Yükle',
                      'Başlangıç menüsü','Kayıt Aktar','Kopyayı Aktar'):
            self.assertIn(label,production)
            self.assertIn('TEXT("'+label+'")',code)
        for name in ('RemasterSessionSmoke.cpp','RemasterGameSessionSubsystem.cpp',
                     'RemasterVanillaPlusSaveSubsystem.cpp'):
            strings=re.findall(r'TEXT\("([^"\n]*)"\)',(M/name).read_text(encoding='utf-8'))
            for literal in strings:
                self.assertNotRegex(literal,r'[ÃÄÅâ][^\x00-\x7f]',name)

    def test_authoritative_surfaces_reject_unconfigured_mandatory_shader(self):
        code=(M/'RemasterWorldActor.cpp').read_text(encoding='utf-8')
        component=code.split('ARemasterWorldActor::ComponentForMetatile(',1)[1].split('bool ARemasterWorldActor::BuildRenderChunks()',1)[0]
        self.assertNotIn('UMaterial::GetDefaultMaterial',component)
    def test_door_publication_requires_checked_final_renderer_refresh(self):
        code=(M/'RemasterGameSessionSubsystem.cpp').read_text(encoding='utf-8')
        door=code.split('bool URemasterGameSessionSubsystem::SetDoor(',1)[1].split('void URemasterGameSessionSubsystem::PlayTruckSound(',1)[0]
        self.assertIn('RefreshPresentation()',door)
    def test_failed_new_game_does_not_publish_a_probe_selection(self):
        code=(M/'RemasterVanillaPlusSaveSubsystem.cpp').read_text(encoding='utf-8')
        create=code.split('bool URemasterVanillaPlusSaveSubsystem::CreateNewGame(',1)[1].split('bool URemasterVanillaPlusSaveSubsystem::ImportCopy(',1)[0]
        before_store=create.split('remaster_emerald_save_store_platform(',1)[0]
        self.assertNotIn('ActiveSlot=',before_store);self.assertNotIn('ActiveFormat=',before_store)
        self.assertNotIn('LoadLegacySave()',before_store)
    def test_truck_host_uses_camera_basis_for_source_sprite_offsets(self):
        code=(M/'RemasterGameSessionSubsystem.cpp').read_text(encoding='utf-8')
        camera=code.split('FVector URemasterGameSessionSubsystem::GetTruckCameraOffset()',1)[1].split('FVector URemasterGameSessionSubsystem::GetTruckBoxOffset(',1)[0]
        box=code.split('FVector URemasterGameSessionSubsystem::GetTruckBoxOffset(',1)[1]
        self.assertIn('SourcePixelLocal(',camera);self.assertIn('SourcePixelLocal(',box);self.assertIn('RotateVector(',box)
if __name__=='__main__':unittest.main()
