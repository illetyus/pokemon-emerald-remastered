from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
MODULE=ROOT/'unreal/Source/PokemonEmeraldRemastered'


class R8EnhancedFallbackCoverage(unittest.TestCase):
    def test_controller_fallback_is_per_key_and_stick_coverage(self):
        text=(MODULE/'RemasterPlayerController.cpp').read_text()
        self.assertIn('NativeCoverage.Add(Keys,Semantic,bMove)',text)
        self.assertIn('BindNativeAction(static_cast<RemasterControls::Action>(i))',text)
        self.assertIn('Binding.action != Action || NativeCoverage.Covered(Binding)',text)
        self.assertIn('bNativeGamepadAxis = !NativeCoverage.StickCovered()',text)
        self.assertNotIn('if (!MoveBound)',text)

    def test_asset_keys_are_checked_before_binding(self):
        text=(MODULE/'RemasterPlayerController.cpp').read_text()
        self.assertIn('if (!Entry.Key.IsValid()) return false;',text)
        self.assertIn('RegisterMappings(Action,RemasterControls::Action::Up,true)',text)
        self.assertIn('BoundActions.Contains(Action) || !RegisterMappings(Action,Semantic)',text)
        self.assertIn('Action->ValueType != EInputActionValueType::Boolean',text)


if __name__=='__main__':unittest.main()
