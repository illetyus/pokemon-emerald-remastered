import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / 'unreal/Source/PokemonEmeraldRemastered'
sys.path.insert(0, str(ROOT / 'tools'))
from build_r9_ui_catalog import build


class R9UI(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.contract = json.loads((ROOT / 'data/r9/ui_contract.json').read_text())
        cls.model = (MODULE / 'RemasterUiModel.h').read_text() + (MODULE / 'RemasterUiModel.cpp').read_text()
        cls.host = (MODULE / 'RemasterUISubsystem.cpp').read_text()
        cls.controller = (MODULE / 'RemasterPlayerController.cpp').read_text()
        cls.widget = (MODULE / 'RemasterUiWidget.cpp').read_text()

    def test_generated_source_names_are_current(self):
        self.assertEqual((MODULE / 'RemasterUiCatalog.inl').read_text(), build())

    def test_nickname_pages_do_not_alias_turkish_japanese(self):
        text = (MODULE / 'RemasterUiCatalog.inl').read_text()
        latin, japanese = text.split('inline const char* JapaneseNicknameGlyph', 1)
        latin = latin.split('inline const char* NicknameGlyph', 1)[1]
        self.assertIn('case 1: return "Ğ";', latin)
        self.assertIn('case 1: return "あ";', japanese)
        self.assertIn('mon.language == 1', self.model)

    def test_required_screen_and_input_contract(self):
        for screen in self.contract['required_screens']:
            self.assertIn('Screen::' + screen, self.model)
        header = (MODULE / 'RemasterUISubsystem.h').read_text()
        for action in self.contract['actions']:
            self.assertRegex(header, r'\b' + action + r'\b')
        self.assertEqual(self.contract['presentation']['region_grid'], [28, 15])
        source = (ROOT / 'vendor/vanillaplus/src/region_map.c').read_text()
        self.assertRegex(source, r'#define MAP_WIDTH\s+28\b')
        self.assertRegex(source, r'#define MAP_HEIGHT\s+15\b')

    def test_ui_never_owns_gameplay_mutators_or_vm_execution(self):
        presentation = self.model + self.host + self.widget
        for forbidden in ['remaster_emerald_flag_set(', 'remaster_emerald_var_set(',
                          'remaster_emerald_party_set(', 'remaster_emerald_bag_remove(',
                          'remaster_emerald_calculate_stats(', 'remaster_emerald_script_runtime_run(']:
            self.assertNotIn(forbidden, presentation)
        self.assertIn('remaster_emerald_script_runtime_complete(', self.model)
        self.assertIn('remaster_emerald_quest_active(save)', self.model)

    def test_world_movement_is_routed_after_ui_consumer(self):
        step = self.controller.split('bool ARemasterPlayerController::StepDirection', 1)[1].split('void ARemasterPlayerController::HandleMove', 1)[0]
        self.assertLess(step.index('RouteUI(UiDirection)'), step.index('Gameplay->StepPlayer'))
        self.assertIn('NotifyCoreStep', step)
        self.assertIn('for (unsigned i=0;i<static_cast<unsigned>(RemasterControls::Action::Count);++i)', self.controller)
        self.assertIn('BindNativeAction(static_cast<RemasterControls::Action>(i))', self.controller)
        self.assertIn('NativeCoverage.Covered(Binding)', self.controller)

    def test_save_rtc_use_existing_wrappers(self):
        for call in ['StoreLegacySave()', 'LoadLegacySave()', 'RealignRtcNow(Time)', 'LoadCurrentMapFromSave(true)']:
            self.assertIn(call, self.host)
        for status in ['Corrupt', 'Unsupported', 'Empty', 'IoError']:
            self.assertIn('ERemasterLegacySaveStatus::' + status, self.host)
        self.assertIn('bIoBusy = true', self.host)
        self.assertIn('bIoBusy = false', self.host)

    def test_widget_constructs_before_slate_and_has_safe_touch_route(self):
        initialization = self.widget.split('void URemasterUiWidget::NativeOnInitialized()', 1)[1].split('void URemasterUiWidget::NativeConstruct()', 1)[0]
        self.assertIn('WidgetTree->RootWidget = Safe', initialization)
        self.assertIn('ConstructWidget<USafeZone>', initialization)
        self.assertIn('SetMinDesiredHeight(64.0f)', self.widget)
        self.assertIn('UI->ActivateRow(Index, Revision)', self.widget)
        self.assertIn('ScrollWidgetIntoView', self.widget)

    def test_public_workflow_is_scoped_and_has_all_regressions(self):
        workflow = (ROOT / '.github/workflows/r9-ui-presentation.yml').read_text()
        self.assertIn('r9-ui-presentation', workflow)
        self.assertNotIn('schedule:', workflow)
        self.assertIn("-p 'test_*.py'", workflow)
        self.assertIn('ctest --test-dir build', workflow)
        self.assertIn('build_r9_ui_catalog.py --check', workflow)

    def test_portable_probe_and_fixture_are_registered(self):
        cmake = (ROOT / 'CMakeLists.txt').read_text()
        self.assertIn('r9_ui_authority_and_actions', cmake)
        self.assertIn('r9_ui_fixture_matrix', cmake)
        recipe = json.loads((ROOT / 'tests/fixtures/r9/ui_expectations.json').read_text())
        self.assertEqual(len(recipe['frames']), 10)
        self.assertEqual(recipe['source_pin'], self.contract['source_pin'])

    def test_decision_ledger_stays_core_scoped(self):
        data = json.loads((ROOT / 'data/r16/qol_decisions.json').read_text())
        rows = data['decisions']
        decisions = {row['id']: row for row in rows}
        self.assertEqual(decisions['QOL-028']['status'], 'REJECTED')
        self.assertEqual(decisions['QOL-025']['status'], 'ACCEPTED')
        self.assertIn('outside portable R16', self.contract['qol']['ledger_policy'])


if __name__ == '__main__':
    unittest.main()

