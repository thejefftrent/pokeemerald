#include "global.h"
#include "text.h"
#include "test/test.h"
#include "ce/modes.h"
#include "ce/rules_warning.h"
#include "ce/test.h"
#include "constants/characters.h"

// The main menu's one-time warning when a save loads on a ROM with other challenge rules
// (ce/rules_warning.h).

static const struct CeModesConfig sNone = {0};
static const struct CeModesConfig sNuzlocke = { .nuzlocke = TRUE, .dupesClause = TRUE };
static const struct CeModesConfig sMonotype = { .monotype = TYPE_WATER };

// The save starts on a ROM with `started` rules, then loads on a ROM with `rom` rules.
static void LoadOn(const struct CeModesConfig *started, const struct CeModesConfig *rom)
{
    memset(gSaveBlock2Ptr->pokedex.ceQolValue, 0, sizeof(gSaveBlock2Ptr->pokedex.ceQolValue));
    CeTest_StartRun(started);
    CE_TEST_CONFIG->body.modes = *rom;
}

TEST("CE: Rules warning: none when the ROM's rules match the save's")
{
    const struct CeModesConfig *rules = NULL;

    PARAMETRIZE { rules = &sNone; }
    PARAMETRIZE { rules = &sNuzlocke; }

    LoadOn(rules, rules);
    EXPECT(!CeRulesWarning_RulesDiffer());
    EXPECT(!CeRulesWarning_ShouldShow());
}

TEST("CE: Rules warning: shown once when the rules differ")
{
    const struct CeModesConfig *started = NULL, *rom = NULL;

    PARAMETRIZE { started = &sNone;     rom = &sNuzlocke; } // a save without modes, a ROM with them
    PARAMETRIZE { started = &sNuzlocke; rom = &sNone; }
    PARAMETRIZE { started = &sNuzlocke; rom = &sMonotype; }

    LoadOn(started, rom);
    EXPECT(CeRulesWarning_RulesDiffer());
    EXPECT(CeRulesWarning_ShouldShow());
    EXPECT(!CeRulesWarning_ShouldShow());
    // The save keeps its original rules.
    EXPECT_EQ(CeModes_Rules()->nuzlocke, started->nuzlocke);
    EXPECT_EQ(CeModes_Rules()->monotype, started->monotype);
}

TEST("CE: Rules warning: the config hash alone doesn't warn")
{
    // Re-patching with other QoL or randomizer settings changes the hash, not the rules.
    CE_TEST_CONFIG->hdr.configHash = 0x11111111;
    LoadOn(&sNuzlocke, &sNuzlocke);
    CE_TEST_CONFIG->hdr.configHash = 0x22222222;
    EXPECT(!CeRulesWarning_ShouldShow());
}

TEST("CE: Rules warning: back on a matching ROM, a later change warns again")
{
    LoadOn(&sNuzlocke, &sMonotype);
    EXPECT(CeRulesWarning_ShouldShow());
    CE_TEST_CONFIG->body.modes = sNuzlocke;
    EXPECT(!CeRulesWarning_ShouldShow());
    CE_TEST_CONFIG->body.modes = sNone;
    EXPECT(CeRulesWarning_ShouldShow());
}

TEST("CE: Rules warning: the shown bit is no QoL feature's")
{
    LoadOn(&sNone, &sNuzlocke);
    EXPECT(CeRulesWarning_ShouldShow());
    EXPECT_EQ(gSaveBlock2Ptr->pokedex.ceQolChosen[0] | gSaveBlock2Ptr->pokedex.ceQolChosen[1], 0);
    EXPECT_EQ(gSaveBlock2Ptr->pokedex.ceQolValue[0], 0);
    EXPECT_EQ(gSaveBlock2Ptr->pokedex.ceQolValue[1], 1u << 31);
    EXPECT_LT(CE_QOL_COUNT, CE_RULES_WARNED_BIT + 1);
}

TEST("CE: Rules warning: every line fits the main menu's message window")
{
    const u8 *text = CeRulesWarning_Text();
    u8 line[64];
    u32 i, n = 0, lines = 0;

    for (i = 0; ; i++)
    {
        if (text[i] == CHAR_NEWLINE || text[i] == CHAR_PROMPT_CLEAR || text[i] == EOS)
        {
            line[n] = EOS;
            EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), 26 * 8); // MENU_WIDTH_ERROR tiles
            lines++;
            n = 0;
            if (text[i] == EOS)
                break;
        }
        else
        {
            EXPECT_LT(n, ARRAY_COUNT(line) - 1);
            line[n++] = text[i];
        }
    }
    EXPECT_EQ(lines, 3);
}
