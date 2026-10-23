#include "global.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "event_data.h"
#include "field_specials.h"
#include "item.h"
#include "malloc.h"
#include "script.h"
#include "shop_criteria.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/maps.h"

bool32 CeProgression_IsReusableTm(enum Item itemId)
{
    // HMs are important in the item data already; the TMs are I_REUSABLE_TMS (FALSE) there.
    return GetItemPocket(itemId) == POCKET_TM_HM && CeQol_IsOn(CE_QOL_REUSABLE_TMS);
}

// The held items that a trade evolution needs, i.e. the EVO_HELD_ITEM_FIELD_FUNC items of
// src/data/items.h without Oval Stone and Razor Claw/Fang, whose evolutions need no trade.
bool32 CeProgression_IsBagEvoItem(enum Item itemId)
{
    switch (itemId)
    {
    case ITEM_KINGS_ROCK:     // Poliwhirl, Slowpoke
    case ITEM_METAL_COAT:     // Onix, Scyther
    case ITEM_DRAGON_SCALE:   // Seadra
    case ITEM_UPGRADE:        // Porygon
    case ITEM_DEEP_SEA_TOOTH: // Clamperl
    case ITEM_DEEP_SEA_SCALE: // Clamperl
    case ITEM_PROTECTOR:      // Rhydon
    case ITEM_ELECTIRIZER:    // Electabuzz
    case ITEM_MAGMARIZER:     // Magmar
    case ITEM_DUBIOUS_DISC:   // Porygon2
    case ITEM_REAPER_CLOTH:   // Dusclops
    case ITEM_PRISM_SCALE:    // Feebas
    case ITEM_WHIPPED_DREAM:  // Swirlix
    case ITEM_SACHET:         // Spritzee
        // Not in the Battle Pyramid: RestorePyramidPlayerParty matches mons by species.
        return CeQol_IsOn(CE_QOL_TRADE_EVOS_WITHOUT_TRADE) && CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE;
    default:
        return FALSE;
    }
}

bool32 CeProgression_InFrontierChallenge(void)
{
    // The Pike, the Pyramid and the Tower multi partner room put Sketched moves back after
    // each battle (ResetSketchedMoves / RestorePyramidPlayerParty), so changing moves there
    // turns the new move into Sketch for good.
    return InBattlePike() || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InMultiPartnerRoom();
}

// The QoL shops' stock, each list ended by ITEM_NONE. A town's mart adds one to its own list
// (sMartExtras), and a seller in the player's room sells it whole (pokemart gCeShop..., in
// data/scripts/ce/room_sellers.inc). Prices are the items' own, which the website edits.
const u16 gCeShopNatureMints[] =
{
    ITEM_LONELY_MINT, ITEM_ADAMANT_MINT, ITEM_NAUGHTY_MINT, ITEM_BRAVE_MINT, ITEM_BOLD_MINT,
    ITEM_IMPISH_MINT, ITEM_LAX_MINT, ITEM_RELAXED_MINT, ITEM_MODEST_MINT, ITEM_MILD_MINT,
    ITEM_RASH_MINT, ITEM_QUIET_MINT, ITEM_CALM_MINT, ITEM_GENTLE_MINT, ITEM_CAREFUL_MINT,
    ITEM_SASSY_MINT, ITEM_TIMID_MINT, ITEM_HASTY_MINT, ITEM_JOLLY_MINT, ITEM_NAIVE_MINT,
    ITEM_SERIOUS_MINT,
    ITEM_NONE,
};

// Everything a Pokémon evolves with: the Linking Cord, the stones, the trade evolutions' held
// items (CeProgression_IsBagEvoItem), the held items of level-up evolutions, and the rest of
// the items used from the bag (ItemUseOutOfBattle_EvolutionItem).
const u16 gCeShopEvolutionItems[] =
{
    ITEM_LINKING_CORD,
    ITEM_FIRE_STONE, ITEM_WATER_STONE, ITEM_THUNDER_STONE, ITEM_LEAF_STONE, ITEM_ICE_STONE,
    ITEM_SUN_STONE, ITEM_MOON_STONE, ITEM_SHINY_STONE, ITEM_DUSK_STONE, ITEM_DAWN_STONE,
    ITEM_KINGS_ROCK, ITEM_METAL_COAT, ITEM_DRAGON_SCALE, ITEM_UPGRADE, ITEM_DUBIOUS_DISC,
    ITEM_PROTECTOR, ITEM_ELECTIRIZER, ITEM_MAGMARIZER, ITEM_REAPER_CLOTH, ITEM_PRISM_SCALE,
    ITEM_DEEP_SEA_TOOTH, ITEM_DEEP_SEA_SCALE, ITEM_WHIPPED_DREAM, ITEM_SACHET,
    ITEM_OVAL_STONE, ITEM_RAZOR_CLAW, ITEM_RAZOR_FANG,
    ITEM_SWEET_APPLE, ITEM_TART_APPLE, ITEM_SYRUPY_APPLE, ITEM_CRACKED_POT, ITEM_CHIPPED_POT,
    ITEM_UNREMARKABLE_TEACUP, ITEM_MASTERPIECE_TEACUP, ITEM_GALARICA_CUFF, ITEM_GALARICA_WREATH,
    ITEM_BLACK_AUGURITE, ITEM_PEAT_BLOCK, ITEM_AUSPICIOUS_ARMOR, ITEM_MALICIOUS_ARMOR,
    ITEM_METAL_ALLOY, ITEM_SCROLL_OF_DARKNESS, ITEM_SCROLL_OF_WATERS,
    ITEM_NONE,
};

// The Mega Ring (Mega Evolution needs it in the bag), then every HOLD_EFFECT_MEGA_STONE item.
const u16 gCeShopMegaStones[] =
{
    ITEM_MEGA_RING,
    ITEM_VENUSAURITE, ITEM_CHARIZARDITE_X, ITEM_CHARIZARDITE_Y, ITEM_BLASTOISINITE,
    ITEM_BEEDRILLITE, ITEM_PIDGEOTITE, ITEM_ALAKAZITE, ITEM_SLOWBRONITE,
    ITEM_GENGARITE, ITEM_KANGASKHANITE, ITEM_PINSIRITE, ITEM_GYARADOSITE,
    ITEM_AERODACTYLITE, ITEM_MEWTWONITE_X, ITEM_MEWTWONITE_Y, ITEM_AMPHAROSITE,
    ITEM_STEELIXITE, ITEM_SCIZORITE, ITEM_HERACRONITE, ITEM_HOUNDOOMINITE,
    ITEM_TYRANITARITE, ITEM_SCEPTILITE, ITEM_BLAZIKENITE, ITEM_SWAMPERTITE,
    ITEM_GARDEVOIRITE, ITEM_SABLENITE, ITEM_MAWILITE, ITEM_AGGRONITE,
    ITEM_MEDICHAMITE, ITEM_MANECTITE, ITEM_SHARPEDONITE, ITEM_CAMERUPTITE,
    ITEM_ALTARIANITE, ITEM_BANETTITE, ITEM_ABSOLITE, ITEM_GLALITITE,
    ITEM_SALAMENCITE, ITEM_METAGROSSITE, ITEM_LATIASITE, ITEM_LATIOSITE,
    ITEM_LOPUNNITE, ITEM_GARCHOMPITE, ITEM_LUCARIONITE, ITEM_ABOMASITE,
    ITEM_GALLADITE, ITEM_AUDINITE, ITEM_DIANCITE, ITEM_CLEFABLITE,
    ITEM_VICTREEBELITE, ITEM_STARMINITE, ITEM_DRAGONINITE, ITEM_MEGANIUMITE,
    ITEM_FERALIGITE, ITEM_SKARMORITE, ITEM_FROSLASSITE, ITEM_EMBOARITE,
    ITEM_EXCADRITE, ITEM_SCOLIPITE, ITEM_SCRAFTINITE, ITEM_EELEKTROSSITE,
    ITEM_CHANDELURITE, ITEM_CHESNAUGHTITE, ITEM_DELPHOXITE, ITEM_GRENINJITE,
    ITEM_PYROARITE, ITEM_FLOETTITE, ITEM_MALAMARITE, ITEM_BARBARACITE,
    ITEM_DRAGALGITE, ITEM_HAWLUCHANITE, ITEM_ZYGARDITE, ITEM_DRAMPANITE,
    ITEM_FALINKSITE, ITEM_HEATRANITE, ITEM_DARKRANITE, ITEM_ZERAORITE,
    ITEM_RAICHUNITE_X, ITEM_RAICHUNITE_Y, ITEM_CHIMECHITE, ITEM_ABSOLITE_Z,
    ITEM_STARAPTITE, ITEM_GARCHOMPITE_Z, ITEM_LUCARIONITE_Z, ITEM_GOLURKITE,
    ITEM_MEOWSTICITE, ITEM_CRABOMINITE, ITEM_GOLISOPITE, ITEM_MAGEARNITE,
    ITEM_SCOVILLAINITE, ITEM_BAXCALIBRITE, ITEM_TATSUGIRINITE, ITEM_GLIMMORANITE,
    ITEM_NONE,
};

// The Z-Power Ring (Z-Moves need it in the bag), then every Z-Crystal. No Dynamax Band: the
// player can't Dynamax in this base (B_FLAG_DYNAMAX_BATTLE is 0).
const u16 gCeShopZCrystals[] =
{
    ITEM_Z_POWER_RING,
    ITEM_NORMALIUM_Z, ITEM_FIRIUM_Z, ITEM_WATERIUM_Z, ITEM_ELECTRIUM_Z,
    ITEM_GRASSIUM_Z, ITEM_ICIUM_Z, ITEM_FIGHTINIUM_Z, ITEM_POISONIUM_Z,
    ITEM_GROUNDIUM_Z, ITEM_FLYINIUM_Z, ITEM_PSYCHIUM_Z, ITEM_BUGINIUM_Z,
    ITEM_ROCKIUM_Z, ITEM_GHOSTIUM_Z, ITEM_DRAGONIUM_Z, ITEM_DARKINIUM_Z,
    ITEM_STEELIUM_Z, ITEM_FAIRIUM_Z, ITEM_PIKANIUM_Z, ITEM_EEVIUM_Z,
    ITEM_SNORLIUM_Z, ITEM_MEWNIUM_Z, ITEM_DECIDIUM_Z, ITEM_INCINIUM_Z,
    ITEM_PRIMARIUM_Z, ITEM_LYCANIUM_Z, ITEM_MIMIKIUM_Z, ITEM_KOMMONIUM_Z,
    ITEM_TAPUNIUM_Z, ITEM_SOLGANIUM_Z, ITEM_LUNALIUM_Z, ITEM_MARSHADIUM_Z,
    ITEM_ALORAICHIUM_Z, ITEM_PIKASHUNIUM_Z, ITEM_ULTRANECROZIUM_Z,
    ITEM_NONE,
};

// The type items: Gems (used up for a stronger move), Plates (Arceus, Judgment) and Memories
// (Silvally). No Tera Shards: nothing in this base uses them.
const u16 gCeShopGemsPlatesMemories[] =
{
    ITEM_NORMAL_GEM, ITEM_FIRE_GEM, ITEM_WATER_GEM, ITEM_ELECTRIC_GEM,
    ITEM_GRASS_GEM, ITEM_ICE_GEM, ITEM_FIGHTING_GEM, ITEM_POISON_GEM,
    ITEM_GROUND_GEM, ITEM_FLYING_GEM, ITEM_PSYCHIC_GEM, ITEM_BUG_GEM,
    ITEM_ROCK_GEM, ITEM_GHOST_GEM, ITEM_DRAGON_GEM, ITEM_DARK_GEM,
    ITEM_STEEL_GEM, ITEM_FAIRY_GEM,
    ITEM_FLAME_PLATE, ITEM_SPLASH_PLATE, ITEM_ZAP_PLATE, ITEM_MEADOW_PLATE,
    ITEM_ICICLE_PLATE, ITEM_FIST_PLATE, ITEM_TOXIC_PLATE, ITEM_EARTH_PLATE,
    ITEM_SKY_PLATE, ITEM_MIND_PLATE, ITEM_INSECT_PLATE, ITEM_STONE_PLATE,
    ITEM_SPOOKY_PLATE, ITEM_DRACO_PLATE, ITEM_DREAD_PLATE, ITEM_IRON_PLATE,
    ITEM_PIXIE_PLATE,
    ITEM_FIRE_MEMORY, ITEM_WATER_MEMORY, ITEM_ELECTRIC_MEMORY, ITEM_GRASS_MEMORY,
    ITEM_ICE_MEMORY, ITEM_FIGHTING_MEMORY, ITEM_POISON_MEMORY, ITEM_GROUND_MEMORY,
    ITEM_FLYING_MEMORY, ITEM_PSYCHIC_MEMORY, ITEM_BUG_MEMORY, ITEM_ROCK_MEMORY,
    ITEM_GHOST_MEMORY, ITEM_DRAGON_MEMORY, ITEM_DARK_MEMORY, ITEM_STEEL_MEMORY,
    ITEM_FAIRY_MEMORY,
    ITEM_NONE,
};

// Kadabra, Machoke, Graveler and Haunter (and later trade evolutions) evolve with it.
static const u16 sLinkingCord[] = { ITEM_LINKING_CORD, ITEM_NONE };

struct CeMartExtras
{
    u16 map;          // MAP_*: the mart's map; each map listed here has one clerk
    u8 feature;       // enum CeQolFeature
    const u16 *items; // ended by ITEM_NONE
};

static const struct CeMartExtras sMartExtras[] =
{
    { MAP_LAVARIDGE_TOWN_HERB_SHOP, CE_QOL_NATURE_MINTS,             gCeShopNatureMints },
    { MAP_MAUVILLE_CITY_MART,       CE_QOL_TRADE_EVOS_WITHOUT_TRADE, sLinkingCord },
    { MAP_SLATEPORT_CITY_MART,      CE_QOL_GEMS_PLATES_MEMORIES,     gCeShopGemsPlatesMemories },
    { MAP_FORTREE_CITY_MART,        CE_QOL_Z_CRYSTALS,               gCeShopZCrystals },
    { MAP_MOSSDEEP_CITY_MART,       CE_QOL_MEGA_STONES,              gCeShopMegaStones },
};

static bool32 IsMartExtrasHere(const struct CeMartExtras *extras)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(extras->map)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(extras->map)
        && CeQol_IsOn(extras->feature);
}

static u32 CountItems(const u16 *items)
{
    u32 count;

    for (count = 0; items[count] != ITEM_NONE; count++)
        ;
    return count;
}

static bool32 IsInList(const u16 *list, u32 count, u16 item)
{
    u32 i;

    for (i = 0; i < count; i++)
    {
        if (list[i] == item)
            return TRUE;
    }
    return FALSE;
}

void CeProgression_BuildShopItemList(const u16 **itemList, u16 *itemCount)
{
    u32 i, j, count, extraCount = 0;
    u16 *list;

    // This runs on every BUY of one clerk conversation. Leaving the buy menu restores the
    // script's list but not its count, which the extras below enlarged, so recount it the
    // way SetShopItemsForSale does before upstream reads that many entries.
    *itemCount = CountItems(*itemList);
    TryBuildDynamicShopItemList(itemList, itemCount);

    for (i = 0; i < ARRAY_COUNT(sMartExtras); i++)
    {
        if (IsMartExtrasHere(&sMartExtras[i]))
            extraCount += CountItems(sMartExtras[i].items);
    }
    if (extraCount == 0)
        return;

    // Same shape as upstream's dynamic list (heap, ITEM_NONE-terminated), so
    // TryFreeDynamicShopItemList frees this one and restores the script's list.
    list = AllocZeroed((*itemCount + extraCount + 1) * sizeof(u16));
    if (list == NULL)
        return;

    count = *itemCount;
    for (i = 0; i < count; i++)
        list[i] = (*itemList)[i];

    for (i = 0; i < ARRAY_COUNT(sMartExtras); i++)
    {
        if (!IsMartExtrasHere(&sMartExtras[i]))
            continue;
        for (j = 0; sMartExtras[i].items[j] != ITEM_NONE; j++)
        {
            u16 item = sMartExtras[i].items[j];

            if (!IsInList(list, count, item) && IsItemShopCriteriaFulfilled(item))
                list[count++] = item;
        }
    }
    list[count] = ITEM_NONE;

    Free((u16 *)*itemList);
    *itemList = list;
    *itemCount = count;
}

// The sellers in the player's room, in the order of their object events in both bedrooms'
// map.json (LittlerootTown_BrendansHouse_2F, LittlerootTown_MaysHouse_2F), with the temp flag
// that hides each one and the feature whose stock it sells.
static const struct CeRoomSeller
{
    u16 flag;
    u8 feature; // enum CeQolFeature
} sRoomSellers[] =
{
    { FLAG_TEMP_5, CE_QOL_NATURE_MINTS },
    { FLAG_TEMP_6, CE_QOL_TRADE_EVOS_WITHOUT_TRADE },
    { FLAG_TEMP_7, CE_QOL_MEGA_STONES },
    { FLAG_TEMP_8, CE_QOL_Z_CRYSTALS },
    { FLAG_TEMP_9, CE_QOL_GEMS_PLATES_MEMORIES },
};

// Each house's 2F is its own child's bedroom, so the other one is the rival's.
static bool32 InPlayersBedroom(void)
{
    u16 map = gSaveBlock2Ptr->playerGender == MALE ? MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F : MAP_LITTLEROOT_TOWN_MAYS_HOUSE_2F;

    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(map)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(map);
}

void CeProgression_UpdateRoomSellers(void)
{
    u32 i;
    bool32 open = CeQol_IsOn(CE_QOL_ROOM_SELLERS) && InPlayersBedroom();

    for (i = 0; i < ARRAY_COUNT(sRoomSellers); i++)
    {
        if (open && CeQol_IsOn(sRoomSellers[i].feature))
            FlagClear(sRoomSellers[i].flag);
        else
            FlagSet(sRoomSellers[i].flag);
    }
}

void CeProgression_ScriptUpdateRoomSellers(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
    CeProgression_UpdateRoomSellers();
}

#define CE_FAST_EGG_CYCLE_DIVISOR 4

u32 CeProgression_EggCycleLength(u32 steps)
{
    if (CeQol_IsOn(CE_QOL_FAST_EGG_HATCHING))
        return steps / CE_FAST_EGG_CYCLE_DIVISOR;
    return steps;
}
