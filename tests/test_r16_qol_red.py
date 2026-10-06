import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class R16QolClosureRedTests(unittest.TestCase):
    """Intentional RED gates for closure work that is not implemented in slice 1."""

    def test_fishing_preserves_required_round_rng(self):
        vendor = read("vendor/vanillaplus/src/field_player_avatar.c")
        header = read("core/include/remaster/emerald_qol.h")
        source = read("core/src/emerald_qol.c")

        # Pinned Vanilla+ keeps rod-dependent mandatory rounds even though it
        # removes the optional follow-up round probabilities.
        self.assertIn("const s16 minRounds1[]", vendor)
        self.assertIn("const s16 minRounds2[]", vendor)
        self.assertIn("Random() % minRounds2[task->tFishingRod]", vendor)
        self.assertIn("[GOOD_ROD]  = {0, 0}", vendor)
        self.assertIn("[SUPER_ROD] = {0, 0}", vendor)

        # Slice 2 must replace the current oversimplified zero-round contract
        # with an explicit deterministic required-round helper that accepts the
        # already-consumed Emerald RNG value rather than owning RNG itself.
        self.assertIn("remaster_emerald_qol_fishing_required_rounds", header)
        self.assertIn("remaster_emerald_qol_fishing_required_rounds", source)
        self.assertNotIn(
            "return rod <= 2 ? 0u : 0u;",
            source,
            "current R16 incorrectly collapses the required fishing rounds",
        )

    def test_running_policy_only_removes_shoes_gate(self):
        vendor = read("vendor/vanillaplus/src/field_player_avatar.c")
        header = read("core/include/remaster/emerald_qol.h")
        source = read("core/src/emerald_qol.c")

        self.assertNotIn("FlagGet(FLAG_SYS_B_DASH)", vendor)
        self.assertIn("PLAYER_AVATAR_FLAG_UNDERWATER", vendor)
        self.assertIn("IsRunningDisallowed", vendor)

        # The portable policy must describe the changed gate rather than claim
        # that running is universally allowed.
        self.assertIn("remaster_emerald_qol_running_requires_shoes", header)
        self.assertIn("remaster_emerald_qol_running_requires_shoes", source)
        self.assertNotIn("int remaster_emerald_qol_running_allowed(void)", header)

    def test_bike_toggle_exposes_source_guards(self):
        vendor = read("vendor/vanillaplus/src/bike.c")
        header = read("core/include/remaster/emerald_qol.h")

        for token in [
            "FLAG_SYS_CYCLING_ROAD",
            "runningState != NOT_MOVING",
            "bikeSpeed != PLAYER_SPEED_STANDING",
            "acroBikeState != ACRO_STATE_NORMAL",
        ]:
            self.assertIn(token, vendor)

        # Slice 2 must expose enough state to regression-pin those guards. The
        # current one-boolean stationary helper is intentionally insufficient.
        for token in [
            "cycling_road",
            "player_moving",
            "mach_speed_standing",
            "acro_state_normal",
        ]:
            self.assertIn(token, header)

    def test_missing_accepted_management_surfaces_are_explicitly_red(self):
        header = read("core/include/remaster/emerald_qol.h")

        # Accepted QOL-020/021/022/026/047 are deliberately absent on main.
        # These assertions keep the closure branch RED until their owning
        # implementation slices land.
        for api in [
            "remaster_emerald_qol_bag_sort",
            "remaster_emerald_qol_bag_auto_sort",
            "remaster_emerald_qol_pc_items_sort",
            "remaster_emerald_qol_move_relearner_status",
            "remaster_emerald_qol_pc_give_held_item",
            "remaster_emerald_qol_pc_take_held_item",
        ]:
            with self.subTest(api=api):
                self.assertIn(api, header)


if __name__ == "__main__":
    unittest.main()
