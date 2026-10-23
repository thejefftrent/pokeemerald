#include "global.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "palette.h"
#include "sprite.h"
#include "ce/selftest.h"
#include "constants/event_objects.h"

// Self-test for custom player sprites (packages/engine/src/sprites/). The website rewrites the
// player's sprite sheets and palettes in place, so for each gender and avatar state this prints
// what the game itself would draw: the graphics info's first frame (address, size, a checksum of
// its pixels) and palette tag, then the palette LoadPlayerObjectEventPalette loads.
// tools/canary/checks/sprites.ts compares them with the sheets and palettes the engine wrote.

#define PROBE_DECORATING (PLAYER_AVATAR_STATE_VSSEEKER + 1) // not an avatar state; its own graphics id

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
}
