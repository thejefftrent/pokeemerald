#include "global.h"
#include "braille_puzzles.h"
#include "event_data.h"
#include "field_move.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "fldeff.h"
#include "follower_npc.h"
#include "item_use.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "pokemon.h"
#include "ce/field_moves.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "constants/event_objects.h"
#include "constants/follower_npc.h"
#include "constants/map_types.h"

// The HM field moves, in the order the party menu lists them.
static const u8 sHmFieldMoves[] =
{
    FIELD_MOVE_CUT,
    FIELD_MOVE_FLASH,
    FIELD_MOVE_ROCK_SMASH,
    FIELD_MOVE_STRENGTH,
    FIELD_MOVE_SURF,
    FIELD_MOVE_FLY,
    FIELD_MOVE_DIVE,
    FIELD_MOVE_WATERFALL,
};

// Party menu entries after the field moves: Switch, Item (or Mail) and Cancel.
#define PARTY_MENU_TRAILING_ACTIONS 3

static bool32 IsHmFieldMove(enum FieldMove fieldMove)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sHmFieldMoves); i++)
    {
        if (sHmFieldMoves[i] == fieldMove)
            return TRUE;
    }
    return FALSE;
}

static bool32 IsUsableWithoutMove(enum FieldMove fieldMove)
{
    return CeQol_IsOn(CE_QOL_FIELD_MOVES_WITHOUT_HMS) && IsHmFieldMove(fieldMove) && IsFieldMoveUnlocked(fieldMove);
}

static s32 FirstPartyMonNotEgg(void)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            break;
        if (!GetMonData(mon, MON_DATA_IS_EGG))
            return i;
    }
    return -1;
}

void CeFieldMoves_CheckFieldMove(enum FieldMove fieldMove)
{
    s32 slot;

    if (gSpecialVar_Result != PARTY_SIZE || !IsUsableWithoutMove(fieldMove))
        return;
    slot = FirstPartyMonNotEgg();
    if (slot < 0)
        return;
    gSpecialVar_Result = slot;
    gSpecialVar_0x8004 = GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_SPECIES);
}

bool32 CeFieldMoves_CanSurfWithoutMove(void)
{
    return !TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING) && IsUsableWithoutMove(FIELD_MOVE_SURF) && FirstPartyMonNotEgg() >= 0;
}

static u8 MetatileBehaviorInFront(void)
{
    s16 x, y;
    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    return MapGridGetMetatileBehaviorAt(x, y);
}

static u8 MetatileBehaviorUnderPlayer(void)
{
    s16 x, y;
    PlayerGetDestCoords(&x, &y);
    return MapGridGetMetatileBehaviorAt(x, y);
}

// The conditions vanilla's SetUpFieldMove_* functions test, without their side effects (they
// set the post-menu callbacks). Cut lists only trees, not the grass vanilla also cuts.
static bool32 CanUseHere(enum FieldMove fieldMove)
{
    bool32 surfing = TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING);

    switch (fieldMove)
    {
    case FIELD_MOVE_CUT:
        return CheckObjectGraphicsInFrontOfPlayer(OBJ_EVENT_GFX_CUTTABLE_TREE)
            || CheckObjectGraphicsInFrontOfPlayer(OBJ_EVENT_GFX_CUTTABLE_TREE_FRLG);
    case FIELD_MOVE_FLASH:
        return ShouldDoBrailleRegisteelEffect() || (gMapHeader.cave == TRUE && !FlagGet(FLAG_SYS_USE_FLASH));
    case FIELD_MOVE_ROCK_SMASH:
        return ShouldDoBrailleRegirockEffect() || CheckObjectGraphicsInFrontOfPlayer(OBJ_EVENT_GFX_BREAKABLE_ROCK);
    case FIELD_MOVE_STRENGTH:
        return CheckObjectGraphicsInFrontOfPlayer(OBJ_EVENT_GFX_PUSHABLE_BOULDER)
            || CheckObjectGraphicsInFrontOfPlayer(OBJ_EVENT_GFX_PUSHABLE_BOULDER_FRLG);
    case FIELD_MOVE_SURF:
        return !surfing && CheckFollowerNPCFlag(FOLLOWER_NPC_FLAG_CAN_SURF) && IsPlayerFacingSurfableFishableWater();
    case FIELD_MOVE_FLY:
        return CheckFollowerNPCFlag(FOLLOWER_NPC_FLAG_CAN_LEAVE_ROUTE) && CeFly_AllowedHere();
    case FIELD_MOVE_DIVE:
        if (!CheckFollowerNPCFlag(FOLLOWER_NPC_FLAG_CAN_DIVE))
            return FALSE;
        if (gMapHeader.mapType == MAP_TYPE_UNDERWATER)
            return !MetatileBehavior_IsUnableToEmerge(MetatileBehaviorUnderPlayer());
        return surfing && MetatileBehavior_IsDiveable(MetatileBehaviorUnderPlayer());
    case FIELD_MOVE_WATERFALL:
        return CheckFollowerNPCFlag(FOLLOWER_NPC_FLAG_CAN_WATERFALL) && IsPlayerSurfingNorth()
            && MetatileBehavior_IsWaterfall(MetatileBehaviorInFront());
    default:
        return FALSE;
    }
}

static bool32 IsInList(const u8 *actions, u32 numActions, u32 action)
{
    u32 i;
    for (i = 0; i < numActions; i++)
    {
        if (actions[i] == action)
            return TRUE;
    }
    return FALSE;
}

void CeFieldMoves_AddPartyMenuActions(u8 *actions, u8 *numActions, u32 maxActions, u32 fieldMoveAction)
{
    u32 i;

    if (CeProgression_InFrontierChallenge())
        return;
    for (i = 0; i < ARRAY_COUNT(sHmFieldMoves); i++)
    {
        enum FieldMove fieldMove = sHmFieldMoves[i];
        u32 action = fieldMoveAction + fieldMove;

        if (*numActions + PARTY_MENU_TRAILING_ACTIONS >= maxActions)
            return;
        if (IsUsableWithoutMove(fieldMove) && !IsInList(actions, *numActions, action) && CanUseHere(fieldMove))
            actions[(*numActions)++] = action;
    }
}

bool32 CeFly_AllowedHere(void)
{
    if (Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
        return TRUE;
    return CeQol_IsOn(CE_QOL_FLY_FROM_ANYWHERE) && CanUseDigOrEscapeRopeOnCurMap();
}

bool32 CeFly_CanFlyFromMap(void)
{
    if (FlagGet(OW_FLAG_POKE_RIDER) && Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
        return TRUE;
    return CeQol_IsOn(CE_QOL_FLY_FROM_ANYWHERE) && IsFieldMoveUnlocked(FIELD_MOVE_FLY)
        && CheckFollowerNPCFlag(FOLLOWER_NPC_FLAG_CAN_LEAVE_ROUTE) && CeFly_AllowedHere();
}
