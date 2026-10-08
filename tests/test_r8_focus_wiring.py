from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "unreal/Source/PokemonEmeraldRemastered"


class R8NativeFocusWiring(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.controller = (MODULE / "RemasterPlayerController.cpp").read_text()
        cls.endpoint = (MODULE / "RemasterInputSubsystem.cpp").read_text()
        cls.ui = (MODULE / "RemasterUISubsystem.cpp").read_text()

    def test_native_delivery_uses_one_compiled_owner_selector(self):
        dispatch = self.controller.split("bool ARemasterPlayerController::DispatchInput(", 1)[1].split(
            "void ARemasterPlayerController::SetupInputComponent()", 1)[0]
        self.assertIn("return Deliver(Action,Target,UI,World,Field,Battle);", dispatch)
        self.assertIn("AttachRouting(this,", self.controller)
        self.assertIn("Input->DetachRouting(this);", self.controller)
        for event in ("BP_OnInteract();", "BP_OnCancel();", "BP_OnMenu();", "BP_OnMap();",
                      "BP_OnQuest();", "BP_OnQuickItem();"):
            self.assertNotIn(event, self.controller)
        self.assertIn("FieldOwner.IsBound() && FieldOwner.Execute(Action)", self.endpoint)
        self.assertIn("BattleOwner.IsBound() && BattleOwner.Execute(Action)", self.endpoint)
        self.assertIn("Status::Unsupported", self.endpoint)

    def test_context_precedes_every_source_capture_and_event_delivery(self):
        methods = {"SubmitPhysical": "Digital.Process(", "SubmitAxis": "Adapter.Process(",
                   "TouchPressed": "Touch.Press(", "TouchMoved": "Touch.Move(",
                   "TouchReleased": "Touch.Release(", "SubmitEvent": "Common.Submit("}
        for method, capture in methods.items():
            body = self.endpoint.split("URemasterInputSubsystem::" + method + "(", 1)[1]
            body = body.split("\n}", 1)[0]
            self.assertLess(body.index("RefreshContext();"), body.index(capture), method)
        self.assertIn("OnReadContext.Execute()", self.endpoint)
        self.assertIn("Context.fieldAttached=FieldOwner.IsBound()", self.endpoint)
        self.assertIn("Context.battleAttached=BattleOwner.IsBound()", self.endpoint)

    def test_r9_host_facts_and_world_api_are_real(self):
        header = (MODULE / "RemasterWorldGameplaySubsystem.h").read_text()
        calls = set(re.findall(r"Gameplay->(\w+)\(", self.ui))
        for method in calls:
            self.assertRegex(header, r"\b" + method + r"\s*\(")
        for fact in ("Out.ioBusy=bIoBusy", "Out.scriptBusy=bScriptBusy", "Out.battleBusy=bBattleBusy",
                     "Out.uiModal=Model.Modal()", "remaster_emerald_script_runtime_pending_request(",
                     "Request!=LastInputRequest", "Screen!=LastInputScreen", "MapKey!=LastInputMap",
                     "InputHostGeneration==MAX_uint64"):
            self.assertIn(fact, self.ui)
        self.assertIn("bIoBusy = true;\n        AdvanceInputBoundary();", self.ui)
        self.assertIn("bIoBusy = false;\n        AdvanceInputBoundary();", self.ui)

    def test_source_guards_do_not_claim_unreal_compilation(self):
        for path in MODULE.glob("*.[ch]pp"):
            self.assertNotRegex(path.read_text(), r"(?m)^\s*\+\s*$", str(path))


if __name__ == "__main__":
    unittest.main()
