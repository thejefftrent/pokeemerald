#include "global.h"
#include "data.h"
#include "string_util.h"
#include "strings.h"
#include "test/test.h"
#include "ce/rival_names.h"
#include "constants/characters.h"
#include "constants/trainers.h"

// Rival names (ce/rival_names.h). The website writes Brendan's and May's names into
// gText_ExpandedPlaceholder_Brendan/_May; {RIVAL} and the rival trainers in battle read them.
// Test builds have expansion's test trainers in gTrainers (test/battle/trainer_control.party),
// none drawn as Brendan or May, so the renaming itself is proven in the ROM by the self-test
// (src/ce/selftest_sprites.c, tools/canary/checks/sprites.ts); these check the rest.
#define RIVAL_RED   1 // Class Rival, Red's picture
#define TRAINER_TEST1 3 // Class Pkmn Trainer 1, Red's picture

ASSUMPTIONS
{
    ASSUME(GetTrainerClassFromId(RIVAL_RED) == TRAINER_CLASS_RIVAL);
    ASSUME(GetTrainerPicFromId(RIVAL_RED) == TRAINER_PIC_RED);
    ASSUME(GetTrainerClassFromId(TRAINER_TEST1) != TRAINER_CLASS_RIVAL);
}

TEST("CE: The name buffers hold the game's names, then the terminator, then zeros")
{
    u32 i;

    EXPECT_EQ(StringLength(gText_ExpandedPlaceholder_Brendan), 7);
    EXPECT_EQ(gText_ExpandedPlaceholder_Brendan[7], EOS);
    EXPECT_EQ(StringLength(gText_ExpandedPlaceholder_May), 3);
    EXPECT_EQ(gText_ExpandedPlaceholder_May[3], EOS);
    for (i = 4; i <= PLAYER_NAME_LENGTH; i++)
        EXPECT_EQ(gText_ExpandedPlaceholder_May[i], 0);
}

TEST("CE: {RIVAL} is May's buffer for a boy and Brendan's for a girl")
{
    u8 gender = gSaveBlock2Ptr->playerGender;

    gSaveBlock2Ptr->playerGender = MALE;
    EXPECT(GetExpandedPlaceholder(PLACEHOLDER_ID_RIVAL) == gText_ExpandedPlaceholder_May);
    gSaveBlock2Ptr->playerGender = FEMALE;
    EXPECT(GetExpandedPlaceholder(PLACEHOLDER_ID_RIVAL) == gText_ExpandedPlaceholder_Brendan);
    gSaveBlock2Ptr->playerGender = gender;
}

TEST("CE: Rival names leave every trainer not drawn as Brendan or May its own name")
{
    const u8 *red = GetTrainerNameFromId(RIVAL_RED);
    const u8 *test1 = GetTrainerNameFromId(TRAINER_TEST1);

    EXPECT(CeRivalNames_OpponentName(RIVAL_RED, red) == red);
    EXPECT(CeRivalNames_OpponentName(TRAINER_TEST1, test1) == test1);
}
