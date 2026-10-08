from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "unreal/Source/PokemonEmeraldRemastered"


class R8CommonRepeatWiring(unittest.TestCase):
    def test_r9_timer_consumes_common_holds_and_drops_old_epoch_time(self):
        text = (MODULE / "RemasterUISubsystem.cpp").read_text()
        timer = text.split("void URemasterUISubsystem::PresentationTick()", 1)[1].split(
            "void URemasterUISubsystem::Refresh()", 1)[0]
        for token in ("Input->RefreshContext();", "Input->GetRouter()",
                      "CommonRepeat.Sync(Router,Model.repeatEnabled)",
                      "CommonRepeat.Tick(Router,Model.repeatEnabled)",
                      "TickAccumulator += bReset ? 0.0", "if (bReset) TickAccumulator=0.0;"):
            self.assertIn(token, timer)
        self.assertNotIn("IsInputKeyDown(", text)
        self.assertNotIn("EKeys::", timer)
        for forbidden in ("StepPlayer(", "SubmitAction(ERemasterUiAction::Confirm",
                          "remaster_emerald_script_runtime_complete("):
            self.assertNotIn(forbidden, timer)

    def test_r9_owns_repeat_and_settings_stay_outside_emerald(self):
        bridge = (MODULE / "RemasterInputRepeat.h").read_text()
        text = (MODULE / "RemasterUISubsystem.cpp").read_text()
        self.assertIn("RemasterUi::Repeat up,down", bridge)
        self.assertIn("repeat.Tick(pressed,enabled)", bridge)
        self.assertIn("focus==Focus::UI||focus==Focus::Dialogue", bridge)
        self.assertIn("CommonRepeat.Reset();TickAccumulator=0.0", text)
        self.assertIn('TEXT("MenuRepeat"), Model.repeatEnabled, GGameUserSettingsIni', text)
        self.assertNotIn("save_block", bridge)


if __name__ == "__main__":
    unittest.main()
