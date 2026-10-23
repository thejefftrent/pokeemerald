#ifndef GUARD_CE_QOL_H
#define GUARD_CE_QOL_H

// Quality-of-life features the Custom Emerald website can switch.
//
// Each feature has a mode in gCeRomConfig.body.qol.mode[] (enum CeQolMode). The two
// PLAYER modes let the player flip the feature from the QoL page of the options menu
// (press R on the options screen); the website picks the starting value. ALWAYS_ON and
// ALWAYS_OFF lock it, which is how a competitive ROM keeps every racer on the same rules.
//
// Every feature id names the non-vanilla behaviour, so an all-zero config (the base ROM)
// plays like vanilla Emerald.
//
// APPEND ONLY: a feature's index is its bit in the save file and its byte in the config.
//
// X(id, upstream): upstream = whether pokeemerald-expansion's own config enables the
// behaviour; TESTING builds start every test with it so upstream's test suite keeps passing
// (CE tests switch features with CeTest_SetQol, ce/test.h).
#define CE_QOL_FEATURES(X)                          \
    X(CE_QOL_LEVEL_CAPS,             FALSE)         \
    X(CE_QOL_RUN_INDOORS,            TRUE)          \
    X(CE_QOL_RUNNING_SHOES,          FALSE)         \
    X(CE_QOL_AUTO_RUN,               FALSE)         \
    X(CE_QOL_BIKE_INDOORS,           FALSE)         \
    X(CE_QOL_FAST_SURF,              FALSE)         \
    X(CE_QOL_INSTANT_TEXT,           FALSE)         \
    X(CE_QOL_HOLD_TO_SPEED_TEXT,     FALSE)         \
    X(CE_QOL_FAST_HP_BAR,            TRUE)          \
    X(CE_QOL_FAST_EXP_BAR,           TRUE)          \
    X(CE_QOL_FAST_BATTLE_INTRO,      TRUE)          \
    X(CE_QOL_SHORT_BATTLE_PAUSES,    FALSE)         \
    X(CE_QOL_QUICK_RUN_CURSOR,       FALSE)         \
    X(CE_QOL_FORFEIT_TRAINER_BATTLES, TRUE)         \
    X(CE_QOL_SKIP_INTRO,             FALSE)         \
    X(CE_QOL_QUICK_SAVE,             FALSE)         \
    X(CE_QOL_REUSABLE_TMS,           FALSE)         \
    X(CE_QOL_ALWAYS_OBEY,            FALSE)         \
    X(CE_QOL_SUMMARY_IV_EV,          FALSE)         \
    X(CE_QOL_SUMMARY_RELEARNER,      TRUE)          \
    X(CE_QOL_EXP_SHARE_ALL,          FALSE)         \
    X(CE_QOL_CATCH_EXP,              TRUE)          \
    X(CE_QOL_TRADE_EVOS_WITHOUT_TRADE, FALSE)       \
    X(CE_QOL_NATURE_MINTS,           FALSE)         \
    X(CE_QOL_NO_WILD_ENCOUNTERS,     FALSE)         \
    X(CE_QOL_NO_TRAINER_SIGHT,       FALSE)         \
    X(CE_QOL_HIDE_REPEAT_MAP_POPUP,  FALSE)         \
    X(CE_QOL_ITEM_DESCRIPTIONS,      FALSE)         \
    X(CE_QOL_CANDY_JAR,              FALSE)         \
    X(CE_QOL_RELEARNER_ITEM,         FALSE)         \
    X(CE_QOL_DELETER_ITEM,           FALSE)         \
    X(CE_QOL_PC_ITEM,                FALSE)         \
    X(CE_QOL_FIELD_MOVES_WITHOUT_HMS, FALSE)        \
    X(CE_QOL_FLY_FROM_ANYWHERE,      FALSE)         \
    X(CE_QOL_INFINITE_REPEL,         FALSE)         \
    X(CE_QOL_FAST_EGG_HATCHING,      FALSE)         \
    X(CE_QOL_SHINY_CHARM,            FALSE)

#define CE_QOL_ENUM_ENTRY(id, upstream) id,
enum CeQolFeature
{
    CE_QOL_FEATURES(CE_QOL_ENUM_ENTRY)
    CE_QOL_COUNT
};
#undef CE_QOL_ENUM_ENTRY

// Room reserved in the config block and the save; CE_QOL_COUNT must stay below it (the save's
// last value bit is the rules warning's, ce/rules_warning.h).
#define CE_QOL_MAX 64

enum CeQolMode
{
    CE_QOL_MODE_OFF,        // locked off (vanilla); also hidden from the in-game page unless showLockedOff
    CE_QOL_MODE_ON,         // locked on
    CE_QOL_MODE_PLAYER_OFF, // player's choice, starts off
    CE_QOL_MODE_PLAYER_ON,  // player's choice, starts on
};

// Vanilla options page rows a competitive ROM can lock (same order as option_menu.c rows).
enum CeVanillaOption
{
    CE_VOPT_TEXT_SPEED,
    CE_VOPT_BATTLE_SCENE,
    CE_VOPT_BATTLE_STYLE,
    CE_VOPT_COUNT,
};

bool32 CeQol_IsOn(enum CeQolFeature feature);
enum CeQolMode CeQol_GetMode(enum CeQolFeature feature);
bool32 CeQol_IsPlayerChoice(enum CeQolFeature feature);
// Player's choice only: record an explicit pick in the save.
void CeQol_SetPlayerValue(enum CeQolFeature feature, bool32 on);

bool32 CeOptions_IsLocked(enum CeVanillaOption option);
u32 CeOptions_Effective(enum CeVanillaOption option, u32 saved);

// Hooks: end of NewGameInitData, and CB2_ContinueSavedGame before the map header loads.
void CeQol_OnNewGame(void);
void CeQol_OnContinue(void);

#endif // GUARD_CE_QOL_H
