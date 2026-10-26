#include "global.h"
#include "battle.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "palette.h"
#include "sprite.h"
#include "string_util.h"
#include "ce/selftest.h"
#include "constants/characters.h"
#include "constants/event_objects.h"
#include "constants/opponents.h"

// Self-test for custom player sprites (packages/engine/src/sprites/). The website rewrites the
// player's sprite sheets and palettes in place, so for each gender and avatar state this prints
// what the game itself would draw: the graphics info's first frame (address, size, a checksum of
// its pixels) and palette tag, then the palette LoadPlayerObjectEventPalette loads.
// tools/canary/checks/sprites.ts compares them with the sheets and palettes the engine wrote.
//
// It also proves the rival names (src/ce/rival_names.c): the {RIVAL} placeholder for each
// gender, and the name the battle text gives two rival trainers and a control (Wally).

#define PROBE_DECORATING (PLAYER_AVATAR_STATE_VSSEEKER + 1) // not an avatar state; its own graphics id
#define PROBE_BATTLE_NAME_SIZE 12 // the bytes of a battle-text name the probe prints (3 words)

static const u8 sText_Trainer1Name[] = _("{B_TRAINER1_NAME}");
static const u16 sProbeRivalTrainers[] = {TRAINER_BRENDAN_ROUTE_103_MUDKIP, TRAINER_MAY_ROUTE_103_MUDKIP, TRAINER_WALLY_MAUVILLE};

static u32 Checksum(const u8 *bytes, u32 size)
{
    u32 i, sum = 0;
    for (i = 0; i < size; i++)
        sum = sum * 31 + bytes[i];
    return sum;
}

static void ProbeSheet(enum Gender gender, u32 state)
{
    u16 gfxId;
    const struct ObjectEventGraphicsInfo *info;

    if (state == PROBE_DECORATING)
        gfxId = gender == MALE ? OBJ_EVENT_GFX_BRENDAN_DECORATING : OBJ_EVENT_GFX_MAY_DECORATING;
    else
        gfxId = GetPlayerAvatarGraphicsIdByStateIdAndGender(state, gender);
    info = GetObjectEventGraphicsInfo(gfxId);

    CeSelfTest_LineStart("playerSprite");
    CeSelfTest_PutField("gender", gender);
    CeSelfTest_PutField("state", state);
    CeSelfTest_PutField("addr", (u32)info->images[0].data);
    CeSelfTest_PutField("size", info->images[0].size);
    CeSelfTest_PutField("sum0", Checksum(info->images[0].data, info->images[0].size));
    CeSelfTest_PutField("tag", info->paletteTag);
    CeSelfTest_LineEmit();
}

static void ProbePalette(enum Gender gender)
{
    static const char *const names[16] = {
        "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7",
        "c8", "c9", "c10", "c11", "c12", "c13", "c14", "c15",
    };
    u32 i, slot;

    FreeAllSpritePalettes();
    slot = LoadPlayerObjectEventPalette(gender);
    CeSelfTest_LineStart("playerPalette");
    CeSelfTest_PutField("gender", gender);
    CeSelfTest_PutField("slot", slot);
    for (i = 0; i < 16 && slot < 16; i++)
        CeSelfTest_PutField(names[i], gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + i]);
    CeSelfTest_LineEmit();
    FreeAllSpritePalettes();
}

static u32 Word(const u8 *bytes, u32 i)
{
    return bytes[i * 4] | (bytes[i * 4 + 1] << 8) | (bytes[i * 4 + 2] << 16) | ((u32)bytes[i * 4 + 3] << 24);
}

// The rival's name as {RIVAL} gives it to each gender (its address and its 8 raw bytes), then the
// battle text's {B_TRAINER1_NAME} for a Brendan and a May rival and for Wally (12 raw bytes).
static void ProbeNames(void)
{
    u8 buf[PROBE_BATTLE_NAME_SIZE];
    u32 gender, i;
    u8 savedGender = gSaveBlock2Ptr->playerGender;
    u32 savedFlags = gBattleTypeFlags;
    u16 savedOpponent = TRAINER_BATTLE_PARAM.opponentA;

    for (gender = MALE; gender <= FEMALE; gender++)
    {
        const u8 *s;

        gSaveBlock2Ptr->playerGender = gender;
        s = GetExpandedPlaceholder(PLACEHOLDER_ID_RIVAL);
        CeSelfTest_LineStart("rivalName");
        CeSelfTest_PutField("gender", gender);
        CeSelfTest_PutField("addr", (u32)s);
        CeSelfTest_PutField("w0", Word(s, 0));
        CeSelfTest_PutField("w1", Word(s, 1));
        CeSelfTest_LineEmit();
    }
    gSaveBlock2Ptr->playerGender = savedGender;

    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    for (i = 0; i < ARRAY_COUNT(sProbeRivalTrainers); i++)
    {
        TRAINER_BATTLE_PARAM.opponentA = sProbeRivalTrainers[i];
        BattleStringExpandPlaceholders(sText_Trainer1Name, buf, sizeof(buf)); // fills buf with EOS first
        CeSelfTest_LineStart("rivalBattle");
        CeSelfTest_PutField("trainer", sProbeRivalTrainers[i]);
        CeSelfTest_PutField("w0", Word(buf, 0));
        CeSelfTest_PutField("w1", Word(buf, 1));
        CeSelfTest_PutField("w2", Word(buf, 2));
        CeSelfTest_LineEmit();
    }
    gBattleTypeFlags = savedFlags;
    TRAINER_BATTLE_PARAM.opponentA = savedOpponent;
}

void CeSelfTest_ProbeSprites(void)
{
    u32 gender, state;

    // For tools/canary/sprites.ts, which plays a new game as each character.
    CeSelfTest_LineStart("spritesScenario");
    CeSelfTest_PutField("playerGender", offsetof(struct SaveBlock2, playerGender));
    CeSelfTest_LineEmit();

    for (gender = MALE; gender <= FEMALE; gender++)
    {
        for (state = PLAYER_AVATAR_STATE_NORMAL; state <= PLAYER_AVATAR_STATE_WATERING; state++)
            ProbeSheet(gender, state);
        ProbeSheet(gender, PROBE_DECORATING);
        ProbePalette(gender);
    }
    ProbeNames();
}
