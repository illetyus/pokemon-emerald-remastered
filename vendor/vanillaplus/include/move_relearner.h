#ifndef GUARD_MOVE_RELEARNER_H
#define GUARD_MOVE_RELEARNER_H

void TeachMoveRelearnerMove(void);
void MoveRelearnerShowHideHearts(s32);

enum
{
    VP_RELEARNER_OK,
    VP_RELEARNER_EGG,
    VP_RELEARNER_NO_SCALE,
    VP_RELEARNER_NO_MOVES,
};

u8 VanillaPlusCanUseMoveRelearner(u8 partyIndex);
void VanillaPlusStartMoveRelearner(u8 partyIndex);

#endif //GUARD_MOVE_RELEARNER_H
