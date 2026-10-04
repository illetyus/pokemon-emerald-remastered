#!/usr/bin/env python3
"""Apply Vanilla+ Phase 5 source changes with assertive, idempotent replacements."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, text):
    (ROOT / path).write_text(text, encoding="utf-8")


def insert_after(path, anchor, addition, sentinel):
    text = read(path)
    if sentinel in text:
        return
    if anchor not in text:
        raise RuntimeError(f"{path}: anchor not found for {sentinel!r}")
    text = text.replace(anchor, anchor + addition, 1)
    write(path, text)


def insert_before(path, anchor, addition, sentinel):
    text = read(path)
    if sentinel in text:
        return
    if anchor not in text:
        raise RuntimeError(f"{path}: anchor not found for {sentinel!r}")
    text = text.replace(anchor, addition + anchor, 1)
    write(path, text)


def replace_once(path, old, new, sentinel=None):
    text = read(path)
    if sentinel is not None and sentinel in text:
        return
    if old not in text:
        if new in text:
            return
        raise RuntimeError(f"{path}: replacement anchor not found")
    text = text.replace(old, new, 1)
    write(path, text)


# ---------------------------------------------------------------------------
# Bag sorting, auto-sort, and quick registered items.
# ---------------------------------------------------------------------------
insert_after(
    "src/item_menu.c",
    '#include "window.h"\n',
    '#include "script_menu.h"\n#include "vanillaplus_items.h"\n',
    '#include "vanillaplus_items.h"',
)

insert_after(
    "src/item_menu.c",
    "static void CancelSell(u8);\n",
    """static bool8 CanOpenVanillaPlusBagSortMenu(void);\nstatic void OpenVanillaPlusBagSortMenu(u8);\nstatic void Task_VanillaPlusBagSortMenuInput(u8);\nstatic void VanillaPlusBagSortByName(u8);\nstatic void VanillaPlusBagSortByType(u8);\nstatic void VanillaPlusBagSortByQuantity(u8);\nstatic void VanillaPlusBagSortByValue(u8);\nstatic void VanillaPlusBagToggleAutoSort(u8);\nstatic void VanillaPlusBagSortCancel(u8);\nstatic void ApplyVanillaPlusBagSort(u8, enum VanillaPlusItemSortMode);\nstatic void RefreshVanillaPlusBagAfterSort(u8, u16);\nstatic void Task_VanillaPlusQuickItemMenu(u8);\nstatic void UseVanillaPlusQuickItem(u8, u16);\n""",
    "static void OpenVanillaPlusBagSortMenu(u8);",
)

insert_after(
    "src/item_menu.c",
    "static EWRAM_DATA struct TempWallyBag *sTempWallyBag = 0;\n",
    "static EWRAM_DATA u8 sVanillaPlusQuickWindowId = WINDOW_NONE;\n",
    "sVanillaPlusQuickWindowId",
)

insert_before(
    "src/item_menu.c",
    "// these are all 2D arrays with a width of 2 but are represented as 1D arrays\n",
    """enum {\n    VP_SORT_MENU_NAME,\n    VP_SORT_MENU_TYPE,\n    VP_SORT_MENU_QUANTITY,\n    VP_SORT_MENU_VALUE,\n    VP_SORT_MENU_AUTO,\n    VP_SORT_MENU_CANCEL,\n};\n\nstatic const struct MenuAction sVanillaPlusSortMenuActions[] =\n{\n    [VP_SORT_MENU_NAME]     = {gText_SortByName,     VanillaPlusBagSortByName},\n    [VP_SORT_MENU_TYPE]     = {gText_SortByType,     VanillaPlusBagSortByType},\n    [VP_SORT_MENU_QUANTITY] = {gText_SortByQuantity, VanillaPlusBagSortByQuantity},\n    [VP_SORT_MENU_VALUE]    = {gText_SortByValue,    VanillaPlusBagSortByValue},\n    [VP_SORT_MENU_AUTO]     = {gText_AutoSort,       VanillaPlusBagToggleAutoSort},\n    [VP_SORT_MENU_CANCEL]   = {gText_Cancel2,        VanillaPlusBagSortCancel},\n};\n\nstatic const u8 sVanillaPlusSortMenuIds[] =\n{\n    VP_SORT_MENU_NAME, VP_SORT_MENU_TYPE,\n    VP_SORT_MENU_QUANTITY, VP_SORT_MENU_VALUE,\n    VP_SORT_MENU_AUTO, VP_SORT_MENU_CANCEL,\n};\n\n""",
    "sVanillaPlusSortMenuActions",
)

replace_once(
    "src/item_menu.c",
    """    default:\n        CompactItemsInBagPocket(pocket);\n        break;\n""",
    """    default:\n        VanillaPlusMaybeAutoSortBagPocket(pocketId);\n        CompactItemsInBagPocket(pocket);\n        break;\n""",
    "VanillaPlusMaybeAutoSortBagPocket(pocketId);",
)

replace_once(
    "src/item_menu.c",
    """            if (gSaveBlock1Ptr->registeredItem != ITEM_NONE && gSaveBlock1Ptr->registeredItem == itemId)\n                BlitBitmapToWindow(windowId, sRegisteredSelect_Gfx, 96, y - 1, 24, 16);\n""",
    """            if (VanillaPlusIsQuickItemRegistered(itemId))\n                BlitBitmapToWindow(windowId, sRegisteredSelect_Gfx, 96, y - 1, 24, 16);\n""",
    "if (VanillaPlusIsQuickItemRegistered(itemId))",
)

replace_once(
    "src/item_menu.c",
    """                if (gSaveBlock1Ptr->registeredItem == gSpecialVar_ItemId)\n                    gBagMenu->contextMenuItemsBuffer[1] = ACTION_DESELECT;\n""",
    """                if (VanillaPlusIsQuickItemRegistered(gSpecialVar_ItemId))\n                    gBagMenu->contextMenuItemsBuffer[1] = ACTION_DESELECT;\n""",
    "VanillaPlusIsQuickItemRegistered(gSpecialVar_ItemId)",
)

old_register = """static void ItemMenu_Register(u8 taskId)\n{\n    s16 *data = gTasks[taskId].data;\n    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];\n    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];\n\n    if (gSaveBlock1Ptr->registeredItem == gSpecialVar_ItemId)\n        gSaveBlock1Ptr->registeredItem = ITEM_NONE;\n    else\n        gSaveBlock1Ptr->registeredItem = gSpecialVar_ItemId;\n    DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);\n    LoadBagItemListBuffers(gBagPosition.pocket);\n    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);\n    ScheduleBgCopyTilemapToVram(0);\n    ItemMenu_Cancel(taskId);\n}\n"""
new_register = """static void ItemMenu_Register(u8 taskId)\n{\n    s16 *data = gTasks[taskId].data;\n    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];\n    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];\n    bool8 success;\n\n    if (VanillaPlusIsQuickItemRegistered(gSpecialVar_ItemId))\n        success = VanillaPlusUnregisterQuickItem(gSpecialVar_ItemId);\n    else if (ItemId_GetFieldFunc(gSpecialVar_ItemId) != NULL)\n        success = VanillaPlusRegisterQuickItem(gSpecialVar_ItemId);\n    else\n        success = FALSE;\n\n    if (!success)\n    {\n        PlaySE(SE_FAILURE);\n        RemoveContextWindow();\n        DisplayItemMessage(taskId, FONT_NORMAL, gText_QuickItemsFull, CloseItemMessage);\n        return;\n    }\n\n    DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);\n    LoadBagItemListBuffers(gBagPosition.pocket);\n    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);\n    ScheduleBgCopyTilemapToVram(0);\n    ItemMenu_Cancel(taskId);\n}\n"""
replace_once("src/item_menu.c", old_register, new_register, "bool8 success;\n\n    if (VanillaPlusIsQuickItemRegistered")

replace_once(
    "src/item_menu.c",
    """        RemoveBagItem(gSpecialVar_ItemId, tItemCount);\n        DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);\n""",
    """        RemoveBagItem(gSpecialVar_ItemId, tItemCount);\n        VanillaPlusPruneQuickItems();\n        DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);\n""",
    "RemoveBagItem(gSpecialVar_ItemId, tItemCount);\n        VanillaPlusPruneQuickItems();",
)

# Add START as a sorting accelerator without disturbing SELECT manual movement.
replace_once(
    "src/item_menu.c",
    """        default:\n            if (JOY_NEW(SELECT_BUTTON))\n            {\n""",
    """        default:\n            if (JOY_NEW(START_BUTTON) && CanOpenVanillaPlusBagSortMenu())\n            {\n                OpenVanillaPlusBagSortMenu(taskId);\n                return;\n            }\n            if (JOY_NEW(SELECT_BUTTON))\n            {\n""",
    "JOY_NEW(START_BUTTON) && CanOpenVanillaPlusBagSortMenu()",
)

insert_before(
    "src/item_menu.c",
    "static bool8 CanSwapItems(void)\n",
    r'''static bool8 CanOpenVanillaPlusBagSortMenu(void)
{
    if (gBagPosition.pocket == TMHM_POCKET || gBagPosition.pocket == BERRIES_POCKET)
        return FALSE;
    switch (gBagPosition.location)
    {
    case ITEMMENULOCATION_FIELD:
    case ITEMMENULOCATION_BATTLE:
    case ITEMMENULOCATION_SHOP:
    case ITEMMENULOCATION_ITEMPC:
        return TRUE;
    default:
        return FALSE;
    }
}

static void OpenVanillaPlusBagSortMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 pos;
    u8 windowId;

    ListMenuGetScrollAndRow(tListTaskId, &gBagPosition.scrollPosition[gBagPosition.pocket], &gBagPosition.cursorPosition[gBagPosition.pocket]);
    pos = gBagPosition.scrollPosition[gBagPosition.pocket] + gBagPosition.cursorPosition[gBagPosition.pocket];
    if (pos < gBagMenu->numItemStacks[gBagPosition.pocket] - 1)
        tListPosition = BagGetItemIdByPocketPosition(gBagPosition.pocket + 1, pos);
    else
        tListPosition = ITEM_NONE;

    BagDestroyPocketScrollArrowPair();
    windowId = BagMenu_AddWindow(ITEMWIN_2x3);
    PrintMenuActionGrid(windowId, FONT_NARROW, 8, 1, 56, 2, 3, sVanillaPlusSortMenuActions, sVanillaPlusSortMenuIds);
    InitMenuActionGrid(windowId, 56, 2, 3, 0);
    gTasks[taskId].func = Task_VanillaPlusBagSortMenuInput;
}

static void Task_VanillaPlusBagSortMenuInput(u8 taskId)
{
    s8 cursorPos = Menu_GetCursorPos();

    if (JOY_NEW(DPAD_UP))
    {
        if (cursorPos >= 2)
        {
            PlaySE(SE_SELECT);
            ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_UP);
        }
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (cursorPos < 4)
        {
            PlaySE(SE_SELECT);
            ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_DOWN);
        }
    }
    else if (JOY_NEW(DPAD_LEFT) || GetLRKeysPressed() == MENU_L_PRESSED)
    {
        if (cursorPos & 1)
        {
            PlaySE(SE_SELECT);
            ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_LEFT, MENU_CURSOR_DELTA_NONE);
        }
    }
    else if (JOY_NEW(DPAD_RIGHT) || GetLRKeysPressed() == MENU_R_PRESSED)
    {
        if (!(cursorPos & 1))
        {
            PlaySE(SE_SELECT);
            ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_RIGHT, MENU_CURSOR_DELTA_NONE);
        }
    }
    else if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        sVanillaPlusSortMenuActions[Menu_GetCursorPos()].func.void_u8(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        VanillaPlusBagSortCancel(taskId);
    }
}

static void RefreshVanillaPlusBagAfterSort(u8 taskId, u16 selectedItem)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];
    struct BagPocket *pocket = &gBagPockets[gBagPosition.pocket];
    u16 pos = 0;
    u16 maxShown;

    DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);
    UpdatePocketItemList(gBagPosition.pocket);

    if (selectedItem != ITEM_NONE)
    {
        while (pos < pocket->capacity && pocket->itemSlots[pos].itemId != selectedItem)
            pos++;
    }
    if (pos >= pocket->capacity || pocket->itemSlots[pos].itemId == ITEM_NONE)
        pos = 0;

    maxShown = gBagMenu->numShownItems[gBagPosition.pocket];
    if (maxShown == 0 || pos < maxShown)
    {
        *scrollPos = 0;
        *cursorPos = pos;
    }
    else
    {
        *cursorPos = maxShown - 1;
        *scrollPos = pos - *cursorPos;
    }

    LoadBagItemListBuffers(gBagPosition.pocket);
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
    ScheduleBgCopyTilemapToVram(0);
    CreatePocketScrollArrowPair();
    CreatePocketSwitchArrowPair();
    PrintItemDescription(pos);
    gTasks[taskId].func = Task_BagMenu_HandleInput;
}

static void ApplyVanillaPlusBagSort(u8 taskId, enum VanillaPlusItemSortMode mode)
{
    u16 selectedItem = gTasks[taskId].tListPosition;

    BagMenu_RemoveWindow(ITEMWIN_2x3);
    VanillaPlusSortBagPocket(gBagPosition.pocket, mode);
    RefreshVanillaPlusBagAfterSort(taskId, selectedItem);
}

static void VanillaPlusBagSortByName(u8 taskId)
{
    ApplyVanillaPlusBagSort(taskId, VP_ITEM_SORT_NAME);
}

static void VanillaPlusBagSortByType(u8 taskId)
{
    ApplyVanillaPlusBagSort(taskId, VP_ITEM_SORT_TYPE);
}

static void VanillaPlusBagSortByQuantity(u8 taskId)
{
    ApplyVanillaPlusBagSort(taskId, VP_ITEM_SORT_QUANTITY);
}

static void VanillaPlusBagSortByValue(u8 taskId)
{
    ApplyVanillaPlusBagSort(taskId, VP_ITEM_SORT_VALUE);
}

static void VanillaPlusBagToggleAutoSort(u8 taskId)
{
    bool8 enabled = !VanillaPlusIsAutoSortEnabled(gBagPosition.pocket);
    enum VanillaPlusItemSortMode mode = VanillaPlusGetBagSortMode(gBagPosition.pocket);

    VanillaPlusSetAutoSortEnabled(gBagPosition.pocket, enabled);
    if (enabled)
    {
        if (mode == VP_ITEM_SORT_NONE)
            mode = VP_ITEM_SORT_NAME;
        ApplyVanillaPlusBagSort(taskId, mode);
    }
    else
    {
        VanillaPlusBagSortCancel(taskId);
    }
}

static void VanillaPlusBagSortCancel(u8 taskId)
{
    BagMenu_RemoveWindow(ITEMWIN_2x3);
    CreatePocketScrollArrowPair();
    CreatePocketSwitchArrowPair();
    gTasks[taskId].func = Task_BagMenu_HandleInput;
}

''',
    "static bool8 CanOpenVanillaPlusBagSortMenu(void)\n{",
)

# Replace vanilla one-item SELECT field use with a four-entry compatibility layer.
old_use_registered = r'''#define tUsingRegisteredKeyItem data[3] // See usage in item_use.c

bool8 UseRegisteredKeyItemOnField(void)
{
    u8 taskId;

    if (InUnionRoom() == TRUE || InBattlePyramid() || InBattlePike() || InMultiPartnerRoom() == TRUE)
        return FALSE;
    HideMapNamePopUpWindow();
    ChangeBgY_ScreenOff(0, 0, BG_COORD_SET);
    if (gSaveBlock1Ptr->registeredItem != ITEM_NONE)
    {
        if (CheckBagHasItem(gSaveBlock1Ptr->registeredItem, 1) == TRUE)
        {
            LockPlayerFieldControls();
            FreezeObjectEvents();
            PlayerFreeze();
            StopPlayerAvatar();
            gSpecialVar_ItemId = gSaveBlock1Ptr->registeredItem;
            taskId = CreateTask(ItemId_GetFieldFunc(gSaveBlock1Ptr->registeredItem), 8);
            gTasks[taskId].tUsingRegisteredKeyItem = TRUE;
            return TRUE;
        }
        else
        {
            gSaveBlock1Ptr->registeredItem = ITEM_NONE;
        }
    }
    ScriptContext_SetupScript(EventScript_SelectWithoutRegisteredItem);
    return TRUE;
}

#undef tUsingRegisteredKeyItem
'''
new_use_registered = r'''#define tUsingRegisteredKeyItem data[3] // See usage in item_use.c

static void UseVanillaPlusQuickItem(u8 chooserTaskId, u16 itemId)
{
    u8 itemTaskId;
    ItemUseFunc func = ItemId_GetFieldFunc(itemId);

    if (chooserTaskId != TASK_NONE)
    {
        if (sVanillaPlusQuickWindowId != WINDOW_NONE)
        {
            ClearToTransparentAndRemoveWindow(sVanillaPlusQuickWindowId);
            sVanillaPlusQuickWindowId = WINDOW_NONE;
        }
        DestroyTask(chooserTaskId);
    }

    if (func == NULL)
    {
        VanillaPlusUnregisterQuickItem(itemId);
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
        return;
    }

    gSpecialVar_ItemId = itemId;
    itemTaskId = CreateTask(func, 8);
    gTasks[itemTaskId].tUsingRegisteredKeyItem = TRUE;
}

static void Task_VanillaPlusQuickItemMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 i;
    u8 count = VanillaPlusGetQuickItemCount();
    s8 input;

    if (data[0] == 0)
    {
        sVanillaPlusQuickWindowId = CreateWindowFromRect(14, 1, 15, count * 2);
        SetStandardWindowBorderStyle(sVanillaPlusQuickWindowId, FALSE);
        for (i = 0; i < count; i++)
        {
            CopyItemName(VanillaPlusGetQuickItem(i), gStringVar1);
            AddTextPrinterParameterized(sVanillaPlusQuickWindowId, FONT_NORMAL, gStringVar1, 8, i * 16 + 1, TEXT_SKIP_DRAW, NULL);
        }
        InitMenuInUpperLeftCornerNormal(sVanillaPlusQuickWindowId, count, 0);
        CopyWindowToVram(sVanillaPlusQuickWindowId, COPYWIN_FULL);
        data[0] = 1;
        return;
    }

    input = Menu_ProcessInputNoWrap();
    if (input == MENU_NOTHING_CHOSEN)
        return;
    if (input == MENU_B_PRESSED)
    {
        PlaySE(SE_SELECT);
        ClearToTransparentAndRemoveWindow(sVanillaPlusQuickWindowId);
        sVanillaPlusQuickWindowId = WINDOW_NONE;
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
        DestroyTask(taskId);
        return;
    }

    PlaySE(SE_SELECT);
    UseVanillaPlusQuickItem(taskId, VanillaPlusGetQuickItem(input));
}

bool8 UseRegisteredKeyItemOnField(void)
{
    u8 taskId;
    u8 count;
    u16 itemId;

    if (InUnionRoom() == TRUE || InBattlePyramid() || InBattlePike() || InMultiPartnerRoom() == TRUE)
        return FALSE;
    HideMapNamePopUpWindow();
    ChangeBgY_ScreenOff(0, 0, BG_COORD_SET);

    VanillaPlusPruneQuickItems();
    count = VanillaPlusGetQuickItemCount();
    if (count == 0)
    {
        ScriptContext_SetupScript(EventScript_SelectWithoutRegisteredItem);
        return TRUE;
    }

    LockPlayerFieldControls();
    FreezeObjectEvents();
    PlayerFreeze();
    StopPlayerAvatar();

    if (count == 1)
    {
        itemId = VanillaPlusGetQuickItem(0);
        UseVanillaPlusQuickItem(TASK_NONE, itemId);
        return TRUE;
    }

    taskId = CreateTask(Task_VanillaPlusQuickItemMenu, 8);
    gTasks[taskId].data[0] = 0;
    return TRUE;
}

#undef tUsingRegisteredKeyItem
'''
replace_once("src/item_menu.c", old_use_registered, new_use_registered, "static void Task_VanillaPlusQuickItemMenu(u8 taskId)\n{")

# ---------------------------------------------------------------------------
# Player PC sorting.
# ---------------------------------------------------------------------------
insert_after(
    "src/player_pc.c",
    '#include "window.h"\n',
    '#include "script_menu.h"\n#include "vanillaplus_items.h"\n',
    '#include "vanillaplus_items.h"',
)
insert_after(
    "src/player_pc.c",
    "static void ItemStorage_ProcessInput(u8);\n",
    """static void OpenVanillaPlusPcSortMenu(u8);\nstatic void Task_VanillaPlusPcSortMenuInput(u8);\nstatic void ApplyVanillaPlusPcSort(u8, enum VanillaPlusItemSortMode);\nstatic void VanillaPlusPcSortByName(u8);\nstatic void VanillaPlusPcSortByType(u8);\nstatic void VanillaPlusPcSortByQuantity(u8);\nstatic void VanillaPlusPcSortByValue(u8);\nstatic void ItemStorage_CompactList(void);\nstatic void ItemStorage_CompactCursor(void);\n""",
    "static void OpenVanillaPlusPcSortMenu(u8);",
)
insert_after(
    "src/player_pc.c",
    "static EWRAM_DATA struct ItemStorageMenu *sItemStorageMenu = NULL;\n",
    "static EWRAM_DATA u8 sVanillaPlusPcSortWindowId = WINDOW_NONE;\n",
    "sVanillaPlusPcSortWindowId",
)
insert_after(
    "src/player_pc.c",
    "#define tListTaskId data[5]\n",
    "#define tSortItem   data[6]\n",
    "#define tSortItem",
)

insert_before(
    "src/player_pc.c",
    "static const struct ItemSlot sNewGamePCItems[] =\n",
    r'''static const struct MenuAction sVanillaPlusPcSortActions[] =
{
    {gText_SortByName, VanillaPlusPcSortByName},
    {gText_SortByType, VanillaPlusPcSortByType},
    {gText_SortByQuantity, VanillaPlusPcSortByQuantity},
    {gText_SortByValue, VanillaPlusPcSortByValue},
};

''',
    "sVanillaPlusPcSortActions",
)

replace_once(
    "src/player_pc.c",
    """    if (JOY_NEW(SELECT_BUTTON))\n    {\n        // 'Select' starts input for swapping items if not on Cancel\n""",
    """    if (JOY_NEW(START_BUTTON))\n    {\n        OpenVanillaPlusPcSortMenu(taskId);\n    }\n    else if (JOY_NEW(SELECT_BUTTON))\n    {\n        // 'Select' starts input for swapping items if not on Cancel\n""",
    "if (JOY_NEW(START_BUTTON))\n    {\n        OpenVanillaPlusPcSortMenu(taskId);",
)

insert_before(
    "src/player_pc.c",
    "static void ItemStorage_ReturnToMenuSelect(u8 taskId)\n",
    r'''static void OpenVanillaPlusPcSortMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 pos;

    ListMenuGetScrollAndRow(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);
    pos = gPlayerPCItemPageInfo.itemsAbove + gPlayerPCItemPageInfo.cursorPos;
    if (pos < gPlayerPCItemPageInfo.count - 1)
        tSortItem = gSaveBlock1Ptr->pcItems[pos].itemId;
    else
        tSortItem = ITEM_NONE;

    ItemStorage_RemoveScrollIndicator();
    sVanillaPlusPcSortWindowId = CreateWindowFromRect(17, 2, 12, 8);
    SetStandardWindowBorderStyle(sVanillaPlusPcSortWindowId, FALSE);
    PrintMenuTable(sVanillaPlusPcSortWindowId, ARRAY_COUNT(sVanillaPlusPcSortActions), sVanillaPlusPcSortActions);
    InitMenuInUpperLeftCornerNormal(sVanillaPlusPcSortWindowId, ARRAY_COUNT(sVanillaPlusPcSortActions), 0);
    CopyWindowToVram(sVanillaPlusPcSortWindowId, COPYWIN_FULL);
    gTasks[taskId].func = Task_VanillaPlusPcSortMenuInput;
}

static void Task_VanillaPlusPcSortMenuInput(u8 taskId)
{
    s8 input = Menu_ProcessInputNoWrap();

    if (input == MENU_NOTHING_CHOSEN)
        return;
    if (input == MENU_B_PRESSED)
    {
        PlaySE(SE_SELECT);
        ClearToTransparentAndRemoveWindow(sVanillaPlusPcSortWindowId);
        sVanillaPlusPcSortWindowId = WINDOW_NONE;
        ItemStorage_AddScrollIndicator();
        gTasks[taskId].func = ItemStorage_ProcessInput;
        return;
    }

    PlaySE(SE_SELECT);
    sVanillaPlusPcSortActions[input].func.void_u8(taskId);
}

static void ApplyVanillaPlusPcSort(u8 taskId, enum VanillaPlusItemSortMode mode)
{
    s16 *data = gTasks[taskId].data;
    u16 selectedItem = tSortItem;
    u16 pos = 0;

    ClearToTransparentAndRemoveWindow(sVanillaPlusPcSortWindowId);
    sVanillaPlusPcSortWindowId = WINDOW_NONE;
    DestroyListMenuTask(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);
    VanillaPlusSortPCItems(mode);
    ItemStorage_CompactList();

    if (selectedItem != ITEM_NONE)
    {
        while (pos < PC_ITEMS_COUNT && gSaveBlock1Ptr->pcItems[pos].itemId != selectedItem)
            pos++;
    }
    if (pos >= PC_ITEMS_COUNT || gSaveBlock1Ptr->pcItems[pos].itemId == ITEM_NONE)
        pos = 0;

    if (pos < gPlayerPCItemPageInfo.pageItems)
    {
        gPlayerPCItemPageInfo.itemsAbove = 0;
        gPlayerPCItemPageInfo.cursorPos = pos;
    }
    else
    {
        gPlayerPCItemPageInfo.cursorPos = gPlayerPCItemPageInfo.pageItems - 1;
        gPlayerPCItemPageInfo.itemsAbove = pos - gPlayerPCItemPageInfo.cursorPos;
    }

    ItemStorage_RefreshListMenu();
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, gPlayerPCItemPageInfo.itemsAbove, gPlayerPCItemPageInfo.cursorPos);
    ItemStorage_AddScrollIndicator();
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = ItemStorage_ProcessInput;
}

static void VanillaPlusPcSortByName(u8 taskId) { ApplyVanillaPlusPcSort(taskId, VP_ITEM_SORT_NAME); }
static void VanillaPlusPcSortByType(u8 taskId) { ApplyVanillaPlusPcSort(taskId, VP_ITEM_SORT_TYPE); }
static void VanillaPlusPcSortByQuantity(u8 taskId) { ApplyVanillaPlusPcSort(taskId, VP_ITEM_SORT_QUANTITY); }
static void VanillaPlusPcSortByValue(u8 taskId) { ApplyVanillaPlusPcSort(taskId, VP_ITEM_SORT_VALUE); }

''',
    "static void OpenVanillaPlusPcSortMenu(u8 taskId)\n{",
)

# ---------------------------------------------------------------------------
# Party held-item MOVE/SWAP.
# ---------------------------------------------------------------------------
replace_once(
    "src/party_menu.c",
    """    MENU_GIVE,\n    MENU_TAKE_ITEM,\n    MENU_MAIL,\n""",
    """    MENU_GIVE,\n    MENU_TAKE_ITEM,\n    MENU_MOVE_ITEM,\n    MENU_MAIL,\n""",
    "MENU_MOVE_ITEM",
)
insert_after(
    "src/party_menu.c",
    "static void CursorCb_TakeItem(u8);\n",
    "static void CursorCb_MoveItem(u8);\nstatic void Task_HandleHeldItemMoveInput(u8);\nstatic void Task_FinishHeldItemMove(u8);\n",
    "static void CursorCb_MoveItem(u8);",
)

replace_once(
    "src/data/party_menu.h",
    """    [MENU_TAKE_ITEM] = {gText_Take, CursorCb_TakeItem},\n    [MENU_MAIL] = {gText_Mail, CursorCb_Mail},\n""",
    """    [MENU_TAKE_ITEM] = {gText_Take, CursorCb_TakeItem},\n    [MENU_MOVE_ITEM] = {gText_MoveItem, CursorCb_MoveItem},\n    [MENU_MAIL] = {gText_Mail, CursorCb_Mail},\n""",
    "[MENU_MOVE_ITEM] = {gText_MoveItem, CursorCb_MoveItem}",
)
replace_once(
    "src/data/party_menu.h",
    "static const u8 sPartyMenuAction_GiveTakeItemCancel[] = {MENU_GIVE, MENU_TAKE_ITEM, MENU_CANCEL2};\n",
    "static const u8 sPartyMenuAction_GiveTakeItemCancel[] = {MENU_GIVE, MENU_TAKE_ITEM, MENU_MOVE_ITEM, MENU_CANCEL2};\n",
    "MENU_GIVE, MENU_TAKE_ITEM, MENU_MOVE_ITEM, MENU_CANCEL2",
)
replace_once(
    "src/data/party_menu.h",
    """    .tilemapLeft = 23,\n    .tilemapTop = 13,\n    .width = 6,\n    .height = 6,\n""",
    """    .tilemapLeft = 23,\n    .tilemapTop = 11,\n    .width = 6,\n    .height = 8,\n""",
    ".tilemapTop = 11,\n    .width = 6,\n    .height = 8,",
)

insert_before(
    "src/party_menu.c",
    "static void CursorCb_TakeItem(u8 taskId)\n",
    r'''static void CursorCb_MoveItem(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);

    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);

    if (item == ITEM_NONE)
    {
        GetMonNickname(mon, gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_PkmnNotHolding);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        return;
    }
    if (GetMonData(mon, MON_DATA_IS_EGG) || ItemIsMail(item))
    {
        DisplayPartyMenuMessage(gText_CantMoveMail, TRUE);
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        return;
    }

    gPartyMenu.slotId2 = gPartyMenu.slotId;
    DisplayPartyMenuMessage(gText_MoveItemWhere, TRUE);
    gTasks[taskId].func = Task_HandleHeldItemMoveInput;
}

static void Task_HandleHeldItemMoveInput(u8 taskId)
{
    s8 *targetSlot = (s8 *)&gPartyMenu.slotId2;
    u16 input;

    if (IsPartyMenuTextPrinterActive())
        return;

    input = PartyMenuButtonHandler(targetSlot);
    if (input == B_BUTTON)
    {
        PlaySE(SE_SELECT);
        AnimatePartySlot(gPartyMenu.slotId2, 0);
        gPartyMenu.slotId2 = gPartyMenu.slotId;
        AnimatePartySlot(gPartyMenu.slotId, 1);
        DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
        gTasks[taskId].func = Task_HandleChooseMonInput;
        return;
    }
    if (input == A_BUTTON)
    {
        struct Pokemon *sourceMon;
        struct Pokemon *targetMon;
        u16 sourceItem;
        u16 targetItem;

        if (gPartyMenu.slotId2 >= gPlayerPartyCount || gPartyMenu.slotId2 == gPartyMenu.slotId)
        {
            PlaySE(SE_FAILURE);
            return;
        }
        sourceMon = &gPlayerParty[gPartyMenu.slotId];
        targetMon = &gPlayerParty[gPartyMenu.slotId2];
        if (GetMonData(targetMon, MON_DATA_IS_EGG))
        {
            PlaySE(SE_FAILURE);
            return;
        }

        sourceItem = GetMonData(sourceMon, MON_DATA_HELD_ITEM);
        targetItem = GetMonData(targetMon, MON_DATA_HELD_ITEM);
        if (ItemIsMail(sourceItem) || ItemIsMail(targetItem))
        {
            PlaySE(SE_FAILURE);
            DisplayPartyMenuMessage(gText_CantMoveMail, TRUE);
            gTasks[taskId].func = Task_FinishHeldItemMove;
            return;
        }

        PlaySE(SE_SELECT);
        SetMonData(sourceMon, MON_DATA_HELD_ITEM, &targetItem);
        SetMonData(targetMon, MON_DATA_HELD_ITEM, &sourceItem);
        UpdatePartyMonHeldItemSprite(sourceMon, &sPartyMenuBoxes[gPartyMenu.slotId]);
        UpdatePartyMonHeldItemSprite(targetMon, &sPartyMenuBoxes[gPartyMenu.slotId2]);
        DisplayPartyMenuMessage(gText_HeldItemMoved, TRUE);
        gTasks[taskId].func = Task_FinishHeldItemMove;
    }
}

static void Task_FinishHeldItemMove(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        AnimatePartySlot(gPartyMenu.slotId2, 0);
        gPartyMenu.slotId2 = gPartyMenu.slotId;
        AnimatePartySlot(gPartyMenu.slotId, 1);
        DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
        gTasks[taskId].func = Task_HandleChooseMonInput;
    }
}

''',
    "static void CursorCb_MoveItem(u8 taskId)\n{",
)

# ---------------------------------------------------------------------------
# Registered bike synchronization.
# ---------------------------------------------------------------------------
insert_after(
    "src/item.c",
    '#include "battle_pyramid_bag.h"\n',
    '#include "vanillaplus_items.h"\n',
    '#include "vanillaplus_items.h"',
)
replace_once(
    "src/item.c",
    r'''void SwapRegisteredBike(void)
{
    switch (gSaveBlock1Ptr->registeredItem)
    {
    case ITEM_MACH_BIKE:
        gSaveBlock1Ptr->registeredItem = ITEM_ACRO_BIKE;
        break;
    case ITEM_ACRO_BIKE:
        gSaveBlock1Ptr->registeredItem = ITEM_MACH_BIKE;
        break;
    }
}
''',
    r'''void SwapRegisteredBike(void)
{
    u16 oldItem = gSaveBlock1Ptr->registeredItem;

    switch (oldItem)
    {
    case ITEM_MACH_BIKE:
        gSaveBlock1Ptr->registeredItem = ITEM_ACRO_BIKE;
        VanillaPlusReplaceQuickItem(ITEM_MACH_BIKE, ITEM_ACRO_BIKE);
        break;
    case ITEM_ACRO_BIKE:
        gSaveBlock1Ptr->registeredItem = ITEM_MACH_BIKE;
        VanillaPlusReplaceQuickItem(ITEM_ACRO_BIKE, ITEM_MACH_BIKE);
        break;
    }
}
''',
    "VanillaPlusReplaceQuickItem(ITEM_MACH_BIKE",
)

# ---------------------------------------------------------------------------
# Strings and build identity.
# ---------------------------------------------------------------------------
insert_before(
    "include/strings.h",
    "#endif // GUARD_STRINGS_H\n",
    """extern const u8 gText_SortItems[];\nextern const u8 gText_SortByName[];\nextern const u8 gText_SortByType[];\nextern const u8 gText_SortByQuantity[];\nextern const u8 gText_SortByValue[];\nextern const u8 gText_AutoSort[];\nextern const u8 gText_MoveItem[];\nextern const u8 gText_MoveItemWhere[];\nextern const u8 gText_CantMoveMail[];\nextern const u8 gText_HeldItemMoved[];\nextern const u8 gText_QuickItemsFull[];\n\n""",
    "extern const u8 gText_SortItems[];",
)
insert_before(
    "src/strings.c",
    "const u8 gText_ContinueMenuPlayer[] = _(\"OYUNCU V+006\");\n",
    """const u8 gText_SortItems[] = _(\"SIRALA\");\nconst u8 gText_SortByName[] = _(\"ADA GÖRE\");\nconst u8 gText_SortByType[] = _(\"TÜRE GÖRE\");\nconst u8 gText_SortByQuantity[] = _(\"MİKTARA\");\nconst u8 gText_SortByValue[] = _(\"DEĞERE\");\nconst u8 gText_AutoSort[] = _(\"OTOMATİK\");\nconst u8 gText_MoveItem[] = _(\"TAŞI\");\nconst u8 gText_MoveItemWhere[] = _(\"Eşya hangi POKéMON'a taşınsın?\");\nconst u8 gText_CantMoveMail[] = _(\"POSTA bu şekilde taşınamaz.\");\nconst u8 gText_HeldItemMoved[] = _(\"Eşya taşındı.\");\nconst u8 gText_QuickItemsFull[] = _(\"Hızlı eşya listesi dolu.\");\n\n""",
    "const u8 gText_SortItems[]",
)
replace_once("src/strings.c", 'const u8 gText_ContinueMenuPlayer[] = _(\"OYUNCU V+006\");', 'const u8 gText_ContinueMenuPlayer[] = _(\"OYUNCU V+007\");')
replace_once("Makefile", "TITLE       := ZUMRUT T006", "TITLE       := ZUMRUT T007")
replace_once("Makefile", "TITLE       := ZUMRUT VP006", "TITLE       := ZUMRUT VP007")

# ---------------------------------------------------------------------------
# Version ownership and permanent CI.
# ---------------------------------------------------------------------------
replace_once(
    "tools/vanillaplus_phase08_verify.py",
    r'''for needle in ["ZUMRUT VP006", "ZUMRUT T006"]:
    if needle not in makefile:
        fail(f"Makefile: Phase 8 build marker missing {needle!r}")
if 'gText_ContinueMenuPlayer[] = _("OYUNCU V+006")' not in strings:
    fail("src/strings.c: Phase 8 Continue marker must be OYUNCU V+006")
''',
    r'''# Exact build-marker ownership moved to Phase 5. Phase 8 remains version-agnostic
# but still requires release/test/Continue markers to stay synchronized.
release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
continue_match = re.search(r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)', strings)
if not release_match or not test_match or not continue_match:
    fail("Phase 8: synchronized Vanilla+ build marker structure is missing")
elif len({release_match.group(1), test_match.group(1), continue_match.group(1)}) != 1:
    fail("Phase 8: release/test/Continue build markers must stay synchronized")
''',
    "Exact build-marker ownership moved to Phase 5",
)

# Ensure Phase 8 permanent workflow contract knows about Phase 5 as a later permanent verifier.
replace_once(
    "tools/vanillaplus_phase08_verify.py",
    '    "tools/vanillaplus_phase04_verify.py",\n    "tools/vanillaplus_phase08_verify.py",\n',
    '    "tools/vanillaplus_phase04_verify.py",\n    "tools/vanillaplus_phase05_verify.py",\n    "tools/vanillaplus_phase08_verify.py",\n',
    '"tools/vanillaplus_phase05_verify.py",',
)

text = read("tools/phase_version_contract_test.py")
if "phase05 =" not in text:
    text = text.replace(
        'phase04 = (ROOT / "tools/vanillaplus_phase04_verify.py").read_text(encoding="utf-8")\n',
        'phase04 = (ROOT / "tools/vanillaplus_phase04_verify.py").read_text(encoding="utf-8")\nphase05 = (ROOT / "tools/vanillaplus_phase05_verify.py").read_text(encoding="utf-8")\n',
        1,
    )
old_owner = '''assert '\"ZUMRUT VP006\"' in phase08\nassert '\"ZUMRUT T006\"' in phase08\nassert 'OYUNCU V+006' in phase08\n'''
new_owner = '''assert '\"ZUMRUT VP006\"' not in phase08\nassert '\"ZUMRUT T006\"' not in phase08\nassert 'OYUNCU V+006' not in phase08\nassert "release_match" in phase08\nassert "test_match" in phase08\nassert "continue_match" in phase08\n\nassert 'ZUMRUT VP007' in phase05\nassert 'ZUMRUT T007' in phase05\nassert 'OYUNCU V+007' in phase05\n'''
if "assert 'ZUMRUT VP007' in phase05" not in text:
    if old_owner not in text:
        raise RuntimeError("phase_version_contract_test.py: ownership block missing")
    text = text.replace(old_owner, new_owner, 1)
write("tools/phase_version_contract_test.py", text)

replace_once(
    ".github/workflows/build.yml",
    """      - name: Verify Vanilla+ Phase 4 invariants\n        run: python3 tools/vanillaplus_phase04_verify.py\n\n      - name: Verify Vanilla+ Phase 8 RTC invariants\n""",
    """      - name: Verify Vanilla+ Phase 4 invariants\n        run: python3 tools/vanillaplus_phase04_verify.py\n\n      - name: Verify Vanilla+ Phase 5 item management invariants\n        run: python3 tools/vanillaplus_phase05_verify.py\n\n      - name: Verify Vanilla+ Phase 8 RTC invariants\n""",
    "Verify Vanilla+ Phase 5 item management invariants",
)

print("Phase 5 source transformation applied")
