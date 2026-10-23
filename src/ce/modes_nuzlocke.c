#include "global.h"
#include "event_data.h"
#include "item.h"
#include "mail.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "ce/modes.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/region_map_sections.h"

// Nuzlocke areas, evolution families and dead Pokémon. Both tables are written by the
// website (src/ce/modes_data.c); all-zero means "each map section and species stands alone".

#define sSave (gSaveBlock2Ptr->pokedex.ceModes)

extern const u8 gCeAreaOfMapsec[];
extern const u16 gCeFamilyRoot[];

u32 CeModes_AreaOfMapsec(u32 mapsec)
{
    u32 value;

    if (mapsec >= MAPSEC_COUNT)
        return CE_AREA_NONE;
    value = gCeAreaOfMapsec[mapsec];
    if (value == CE_AREA_FREE)
        return CE_AREA_NONE;
    if (value == CE_AREA_OWN || value > MAPSEC_COUNT)
        return mapsec; // the website never writes the latter; this guards the save's bitmask
    return value - 1;
}

u32 CeModes_CurrentArea(void)
{
    // The rules start with the Pokédex, which comes with the first Poké Balls.
    if (!FlagGet(FLAG_SYS_POKEDEX_GET))
        return CE_AREA_NONE;
    return CeModes_AreaOfMapsec(gMapHeader.regionMapSectionId);
}

bool32 CeModes_IsAreaUsed(u32 area)
{
    if (area >= 32 * CE_MODES_AREA_WORDS || !CeModes_SaveHasRules())
        return FALSE;
    return (sSave.areaUsed[area / 32] >> (area % 32)) & 1;
}

void CeModes_UseArea(u32 area)
{
    if (area < 32 * CE_MODES_AREA_WORDS && CeModes_SaveHasRules())
        sSave.areaUsed[area / 32] |= 1u << (area % 32);
}

u16 CeModes_FamilyRoot(u16 species)
{
    u16 root;

    if (species >= NUM_SPECIES)
        return species;
    root = gCeFamilyRoot[species];
    return (root == SPECIES_NONE || root >= NUM_SPECIES) ? species : root;
}

// Owned = caught in the Pokédex, which also covers starters, gifts, eggs and trades.
bool32 CeModes_FamilyOwned(u16 species)
{
    u32 i, root = CeModes_FamilyRoot(species);

    for (i = 1; i < NUM_SPECIES; i++)
    {
        u32 dexNum;
        if (CeModes_FamilyRoot(i) != root)
            continue;
        dexNum = SpeciesToNationalPokedexNum(i);
        if (dexNum != NATIONAL_DEX_NONE && GetSetPokedexFlag(dexNum, FLAG_GET_CAUGHT))
            return TRUE;
    }
    return FALSE;
}

bool32 CeModes_IsDeadBox(const struct BoxPokemon *boxMon)
{
    return boxMon->ceDead;
}

bool32 CeModes_IsDead(const struct Pokemon *mon)
{
    return mon != NULL && mon->box.ceDead;
}

u32 CeModes_MarkFaintedDead(u32 slotMask)
{
    u32 i, count = 0;

    if (!CeModes_Rules()->nuzlocke)
        return 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (!(slotMask & (1u << i)) || mon->box.ceDead)
            continue;
        if (!GetMonData(mon, MON_DATA_SANITY_HAS_SPECIES) || GetMonData(mon, MON_DATA_IS_EGG) || GetMonData(mon, MON_DATA_HP) != 0)
            continue;
        mon->box.ceDead = TRUE;
        count++;
    }
    sSave.deaths = min(sSave.deaths + count, 0xFFFF);
    return count;
}

u32 CeModes_ReleaseDead(void)
{
    const struct CeModesConfig *rules = CeModes_Rules();
    u32 i, alive = 0, released = 0;

    if (!rules->nuzlocke || !rules->releaseDead)
        return 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SANITY_HAS_SPECIES) && !GetMonData(mon, MON_DATA_IS_EGG) && !mon->box.ceDead)
            alive++;
    }
    // An empty party would break the game: then the dead stay until a living one joins.
    if (alive == 0)
        return 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u16 item;

        if (!GetMonData(mon, MON_DATA_SANITY_HAS_SPECIES) || !mon->box.ceDead)
            continue;
        item = GetMonData(mon, MON_DATA_HELD_ITEM);
        if (ItemIsMail(item))
            TakeMailFromMon(mon);
        if (item != ITEM_NONE)
            AddBagItem(item, 1);
        ZeroMonData(mon);
        released++;
    }
    if (released != 0)
    {
        CompactPartySlots();
        CalculatePlayerPartyCount();
    }
    return released;
}
