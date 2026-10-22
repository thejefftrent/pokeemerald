#include "global.h"
#include "battle_setup.h"
#include "caps.h"
#include "data.h"
#include "item.h"
#include "ce/level_caps.h"
#include "ce/qol.h"
#include "ce/rom_config.h"
#include "constants/opponents.h"

// Upstream's caps code must stay compiled in but switched by this file, not by config.
#if B_EXP_CAP_TYPE != EXP_CAP_NONE || B_LEVEL_CAP_TYPE != LEVEL_CAP_NONE || B_RARE_CANDY_CAP || B_LEVEL_CAP_EXP_UP
#error "CE: level caps are runtime; keep the level-cap settings in include/config/caps.h at their defaults"
#endif

// Progression order. Trainer flags are set only on a win and never cleared for these
// trainers, so (unlike badges or FLAG_DEFEATED_ELITE_4_*) the cap never drops back.
static const u16 sDefaultBosses[] =
{
    TRAINER_ROXANNE_1,
    TRAINER_BRAWLY_1,
    TRAINER_WATTSON_1,
    TRAINER_FLANNERY_1,
    TRAINER_NORMAN_1,
    TRAINER_WINONA_1,
    TRAINER_TATE_AND_LIZA_1,
    TRAINER_JUAN_1,
    TRAINER_SIDNEY,
    TRAINER_PHOEBE,
    TRAINER_GLACIA,
    TRAINER_DRAKE,
    TRAINER_WALLACE,
};

// Highest level the trainer can field: every party (or pool) entry, after following
// overrideTrainer the way CreateNPCTrainerParty does. 0 if the trainer has no party.
static u32 GetTrainerAceLevel(u16 trainerId)
{
    const struct Trainer *trainer = GetTrainerStructFromId(trainerId);
    const struct TrainerMon *party = trainer->party;
    u32 poolSize = trainer->poolSize;
    u32 partySize = trainer->partySize;
    u32 count, i, ace = 0;

    if (trainer->overrideTrainer)
    {
        const struct Trainer *other = GetTrainerStructFromId(trainer->overrideTrainer);
        party = other->party;
        poolSize = other->poolSize;
        if (partySize == 0)
            partySize = other->partySize;
    }
    count = poolSize ? poolSize : partySize;
    for (i = 0; party != NULL && i < count; i++)
    {
        if (party[i].lvl > ace)
            ace = party[i].lvl;
    }
    return min(ace, MAX_LEVEL);
}

u32 CeLevelCap_GetBosses(const u16 **bosses)
{
    const struct CeLevelCapConfig *config = &gCeRomConfig.body.levelCaps;

    if (config->bossCount != 0)
    {
        *bosses = config->bosses;
        return min(config->bossCount, CE_LEVEL_CAP_MAX_BOSSES);
    }
    *bosses = sDefaultBosses;
    return ARRAY_COUNT(sDefaultBosses);
}

u32 CeLevelCap_Compute(u16 *bossOut)
{
    const struct CeLevelCapConfig *config = &gCeRomConfig.body.levelCaps;
    const u16 *bosses;
    u32 count = CeLevelCap_GetBosses(&bosses);
    u32 i, ace = 0;
    s32 cap;

    for (i = 0; i < count; i++)
    {
        if (bosses[i] == TRAINER_NONE || bosses[i] >= TRAINERS_COUNT)
            continue; // not a trainer; the website rejects these, this guards the ROM
        // Including beaten bosses keeps the cap from dropping if a later boss is weaker.
        ace = max(ace, GetTrainerAceLevel(bosses[i]));
        if (!HasTrainerBeenFought(bosses[i]))
            break;
    }
    if (i == count || ace == 0)
    {
        if (bossOut != NULL)
            *bossOut = TRAINER_NONE;
        return MAX_LEVEL;
    }
    if (bossOut != NULL)
        *bossOut = bosses[i];

    cap = (s32)ace + config->levelOffset;
    return (u32)max(1, min(cap, MAX_LEVEL));
}

u32 CeLevelCap_GetEffective(void)
{
    if (!CeQol_IsOn(CE_QOL_LEVEL_CAPS))
        return MAX_LEVEL;
    return CeLevelCap_Compute(NULL);
}

bool32 CeLevelCap_IsUnlimitedItem(u16 item)
{
    return GetItemPocket(item) == POCKET_KEY_ITEMS;
}

bool32 CeLevelCap_BlocksLevelItem(u16 item, u32 level)
{
    (void)item;
    return level >= GetCurrentLevelCap();
}

u32 CeLevelCap_ClampGainedLevel(u32 before, u32 after)
{
    u32 cap = max(GetCurrentLevelCap(), before);
    return min(after, cap);
}
