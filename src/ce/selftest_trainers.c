#include "global.h"
#include "battle_setup.h"
#include "data.h"
#include "item.h"
#include "pokemon.h"
#include "constants/battle.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/opponents.h"
#include "ce/rom_config.h"
#include "ce/selftest.h"

// Builds the probe trainers' parties with the game's own party code, the way a battle
// does, and prints what came out: the canary compares it with what the website wrote.
// Uses the opponent party buffer, which holds nothing at this point (no new RAM).

// Same as the static CreateNPCTrainerParty in battle_setup.c: a trainer with
// overrideTrainer fields that trainer's party (and its partySize when its own is 0).
static void CreateParty(struct Pokemon *party, u16 trainerId)
{
    const struct Trainer *trainer = GetTrainerStructFromId(trainerId);
    struct Trainer copy;

    if (trainer->overrideTrainer)
    {
        const struct Trainer *other = GetTrainerStructFromId(trainer->overrideTrainer);
        copy = *trainer;
        copy.party = other->party;
        copy.poolSize = other->poolSize;
        if (copy.partySize == 0)
            copy.partySize = other->partySize;
        trainer = &copy;
    }
    CreateNPCTrainerPartyFromTrainer(party, trainer);
}

static void ProbeTrainer(u16 trainerId)
{
    struct Pokemon *party = gParties[B_TRAINER_OPPONENT_A];
    const enum Item *items = GetTrainerItemsFromId(trainerId);
    u64 aiFlags = GetTrainerAIFlagsFromId(trainerId);
    u32 i, count = 0;

    CreateParty(party, trainerId);
    for (i = 0; i < PARTY_SIZE && GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE; i++)
        count++;

    CeSelfTest_LineStart("trainer");
    CeSelfTest_PutField("trainer", trainerId);
    CeSelfTest_PutField("count", count);
    CeSelfTest_PutField("class", GetTrainerClassFromId(trainerId));
    CeSelfTest_PutField("aiLo", (u32)aiFlags);
    CeSelfTest_PutField("aiHi", (u32)(aiFlags >> 32));
    for (i = 0; i < MAX_TRAINER_ITEMS; i++)
    {
        static const char *const names[] = { "i0", "i1", "i2", "i3" };
        CeSelfTest_PutField(names[i], items[i]);
    }
    CeSelfTest_LineEmit();

    for (i = 0; i < count; i++)
    {
        struct Pokemon *mon = &party[i];
        u8 nickname[POKEMON_NAME_LENGTH + 1];
        u32 j, nick = 0;

        // The nickname's first four characters (0 after the end), so a written nickname
        // shows up without printing text.
        GetMonData(mon, MON_DATA_NICKNAME, nickname);
        for (j = 0; j < 4 && nickname[j] != EOS; j++)
            nick |= nickname[j] << (j * 8);

        CeSelfTest_LineStart("tmon");
        CeSelfTest_PutField("trainer", trainerId);
        CeSelfTest_PutField("slot", i);
        CeSelfTest_PutField("species", GetMonData(mon, MON_DATA_SPECIES));
        CeSelfTest_PutField("level", GetMonData(mon, MON_DATA_LEVEL));
        CeSelfTest_PutField("m0", GetMonData(mon, MON_DATA_MOVE1));
        CeSelfTest_PutField("m1", GetMonData(mon, MON_DATA_MOVE2));
        CeSelfTest_PutField("m2", GetMonData(mon, MON_DATA_MOVE3));
        CeSelfTest_PutField("m3", GetMonData(mon, MON_DATA_MOVE4));
        CeSelfTest_PutField("item", GetMonData(mon, MON_DATA_HELD_ITEM));
        CeSelfTest_PutField("ability", GetMonAbility(mon));
        CeSelfTest_PutField("nature", GetNature(mon));
        CeSelfTest_PutField("ball", GetMonData(mon, MON_DATA_POKEBALL));
        CeSelfTest_PutField("shiny", GetMonData(mon, MON_DATA_IS_SHINY));
        CeSelfTest_PutField("ivs", GetMonData(mon, MON_DATA_IVS));
        CeSelfTest_PutField("nick", nick);
        CeSelfTest_LineEmit();

        CeSelfTest_LineStart("tmonEvs");
        CeSelfTest_PutField("trainer", trainerId);
        CeSelfTest_PutField("slot", i);
        CeSelfTest_PutField("hp", GetMonData(mon, MON_DATA_HP_EV));
        CeSelfTest_PutField("atk", GetMonData(mon, MON_DATA_ATK_EV));
        CeSelfTest_PutField("def", GetMonData(mon, MON_DATA_DEF_EV));
        CeSelfTest_PutField("spe", GetMonData(mon, MON_DATA_SPEED_EV));
        CeSelfTest_PutField("spa", GetMonData(mon, MON_DATA_SPATK_EV));
        CeSelfTest_PutField("spd", GetMonData(mon, MON_DATA_SPDEF_EV));
        CeSelfTest_LineEmit();
    }
}

void CeSelfTest_ProbeTrainers(void)
{
    const struct CeTrainerConfig *config = &gCeRomConfig.body.trainers;
    u32 i;

    for (i = 0; i < CE_TRAINER_PROBES && config->probeTrainers[i] != TRAINER_NONE; i++)
    {
        if (config->probeTrainers[i] < TRAINERS_COUNT)
            ProbeTrainer(config->probeTrainers[i]);
    }
    for (i = 0; i < CE_PRICE_PROBES && config->probeItems[i] != ITEM_NONE; i++)
    {
        CeSelfTest_LineStart("itemPrice");
        CeSelfTest_PutField("item", config->probeItems[i]);
        CeSelfTest_PutField("price", GetItemPrice(config->probeItems[i]));
        CeSelfTest_LineEmit();
    }
}
