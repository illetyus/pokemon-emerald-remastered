from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "unreal/Source/PokemonEmeraldRemastered"


class R8LifecycleWiring(unittest.TestCase):
    def test_every_owned_application_delegate_is_removed(self):
        text = (MODULE / "RemasterInputSubsystem.cpp").read_text()
        for delegate, handle, callback in (
                ("ApplicationWillEnterBackgroundDelegate", "BackgroundHandle", "Background"),
                ("ApplicationHasEnteredForegroundDelegate", "ForegroundHandle", "Foreground"),
                ("ApplicationWillDeactivateDelegate", "InactiveHandle", "Inactive"),
                ("ApplicationHasReactivatedDelegate", "ReactivatedHandle", "Reactivated")):
            self.assertIn(handle + "=FCoreDelegates::" + delegate + ".AddUObject(this,&URemasterInputSubsystem::" + callback + ")", text)
            self.assertIn("FCoreDelegates::" + delegate + ".Remove(" + handle + ")", text)
        self.assertIn("GetOnInputDeviceConnectionChange().AddUObject", text)
        self.assertIn("GetOnInputDeviceConnectionChange().Remove(DeviceHandle)", text)
        self.assertIn("FTSTicker::GetCoreTicker().RemoveTicker(TickHandle)", text)

    def test_lifecycle_fences_and_pause_observation_do_not_step_gameplay(self):
        text = (MODULE / "RemasterInputSubsystem.cpp").read_text()
        for token in ("FenceSources(Common,Touch,Digital,EnhancedAxis,GamepadAxis)",
                      "PC->FlushPressedKeys()", "bIgnoreAllPressedKeysUntilRelease=true",
                      "EInputMappingRebuildType::RebuildWithFlush", "UGameplayStatics::IsGamePaused(",
                      "Context.suspensionReasons|=Lifecycle.Reasons()"):
            self.assertIn(token, text)
        observer = text.split("bool URemasterInputSubsystem::ObserveLifecycle(", 1)[1].split("\n}", 1)[0]
        self.assertIn("RefreshContext();", observer)
        for forbidden in ("StepPlayer(", "SubmitEvent(", "StoreLegacySave(",
                          "remaster_emerald_script_runtime_complete("):
            self.assertNotIn(forbidden, observer)
        fence = text.split("void URemasterInputSubsystem::LifecycleFence()", 1)[1].split(
            "void URemasterInputSubsystem::SetSuspended(", 1)[0]
        self.assertNotIn("OnDispatch.Execute(", fence)
        self.assertNotIn("SubmitEvent(", fence)

    def test_mapping_context_teardown_removes_only_owned_context(self):
        text = (MODULE / "RemasterPlayerController.cpp").read_text()
        self.assertIn("if (!Subsystem->HasMappingContext(Context))", text)
        self.assertIn("OwnedMappingContext=Context;", text)
        self.assertIn("if (OwnedMappingContext.IsValid())", text)
        self.assertIn("Enhanced->RemoveMappingContext(OwnedMappingContext.Get());", text)
        self.assertNotIn("ClearAllMappings(", text)


if __name__ == "__main__":
    unittest.main()
