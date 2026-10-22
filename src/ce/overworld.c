#include "global.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "field_player_avatar.h"
#include "task.h"
#include "trainer_hill.h"
#include "window.h"
#include "ce/overworld.h"
#include "ce/qol.h"
#include "constants/layouts.h"
#include "constants/map_types.h"
#include "constants/region_map_sections.h"

bool32 CeOverworld_IsRunButtonHeld(u16 heldKeys)
{
    return ((heldKeys & B_BUTTON) != 0) != (CeQol_IsOn(CE_QOL_AUTO_RUN) != FALSE);
}

// PlayerWalkFaster is the Mach Bike's top speed. The surf blob copies the player's sprite
// position every frame, so it keeps up at any speed.
void CeOverworld_SurfStep(enum Direction direction, u16 heldKeys)
{
    if (CeQol_IsOn(CE_QOL_FAST_SURF) && CeOverworld_IsRunButtonHeld(heldKeys))
        PlayerWalkFaster(direction);
    else
        PlayerWalkFast(direction);
}

// Indoor maps that keep the vanilla ban on bikes. Arriving on one of them by warp puts the
// player back on foot (GetAdjustedInitialTransitionFlags), so these also guard the maps that
// can only be reached through them.
static const u16 sNoBikeLayouts[] =
{
    LAYOUT_POKEMON_CENTER_2F,                       // attendants walk the player into the link rooms
    LAYOUT_BATTLE_TENT_LOBBY,                       // Battle Tents: scripted facility challenges
    LAYOUT_BATTLE_TENT_CORRIDOR,
    LAYOUT_BATTLE_TENT_BATTLE_ROOM,
    LAYOUT_VERDANTURF_TOWN_BATTLE_TENT_BATTLE_ROOM,
    LAYOUT_MAUVILLE_CITY_BIKE_SHOP,                 // swapping bikes while riding one
};

// An ordinary indoor map: houses, shops, Pokémon Centers' ground floors, gyms, the League.
// Puzzles that need the player on foot (rotating gates, Strength boulders) only need the
// player to get off, which CeOverworld_CanGetOffBikeIndoors always allows here.
static bool32 IsBikeIndoorsMap(void)
{
    u32 i;

    // Not secret bases (decorations), caves or underwater maps.
    if (gMapHeader.mapType != MAP_TYPE_INDOOR)
        return FALSE;

    // MAPSEC_DYNAMIC: link rooms, contest halls, the S.S. Tidal and the Battle Pyramid squares.
    // MAPSEC_BATTLE_FRONTIER: facilities; the Battle Pyramid swaps in its own Bag, which
    // holds no bike to get off with.
    if (gMapHeader.regionMapSectionId == MAPSEC_DYNAMIC
     || gMapHeader.regionMapSectionId == MAPSEC_BATTLE_FRONTIER
     || gMapHeader.regionMapSectionId == MAPSEC_TRAINER_HILL)
        return FALSE;

    for (i = 0; i < ARRAY_COUNT(sNoBikeLayouts); i++)
    {
        if (gMapHeader.mapLayoutId == sNoBikeLayouts[i])
            return FALSE;
    }
    return TRUE;
}

bool32 CeOverworld_IsBikeIndoorsAllowed(void)
{
    return CeQol_IsOn(CE_QOL_BIKE_INDOORS) && IsBikeIndoorsMap();
}

// Vanilla never has the player riding where the map bans cycling, so this only changes
// what happens after CE_QOL_BIKE_INDOORS put them there.
bool32 CeOverworld_CanGetOffBikeIndoors(void)
{
    return TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_BIKE) && IsBikeIndoorsMap();
}

bool32 CeOverworld_NoWildEncounters(void)
{
    return CeQol_IsOn(CE_QOL_NO_WILD_ENCOUNTERS) && !InBattlePike() && !InBattlePyramid();
}

bool32 CeOverworld_NoTrainerSight(void)
{
    return CeQol_IsOn(CE_QOL_NO_TRAINER_SIGHT) && !InBattlePyramid() && !InTrainerHill();
}

// ScriptShowItemDescription's box: a BG 0 window of 28x3 tiles from tile 8, the same tiles
// the money box (DrawMoneyBox) starts at. Its icon also shares the money label's sprite tag.
#define DESCRIPTION_BOX_FIRST_TILE 8
#define DESCRIPTION_BOX_TILE_COUNT (28 * 3)

static bool32 IsDescriptionBoxTileRangeFree(void)
{
    u32 i;

    for (i = 0; i < WINDOWS_MAX; i++)
    {
        const struct WindowTemplate *window = &gWindows[i].window;
        u32 first = window->baseBlock;
        u32 end = first + window->width * window->height;

        if (window->bg != 0) // removed windows have bg 0xFF
            continue;
        if (first < DESCRIPTION_BOX_FIRST_TILE + DESCRIPTION_BOX_TILE_COUNT && end > DESCRIPTION_BOX_FIRST_TILE)
            return FALSE;
    }
    return TRUE;
}

// Marks a description box as up between the show and hide script commands. A task, so no
// RAM is added; field scripts never reset tasks between the two.
static void Task_CeItemDescriptionUp(u8 taskId)
{
}

bool32 CeOverworld_ShowItemDescription(void)
{
    if (!CeQol_IsOn(CE_QOL_ITEM_DESCRIPTIONS) || !IsDescriptionBoxTileRangeFree())
        return FALSE;
    CreateTask(Task_CeItemDescriptionUp, 0xFF);
    return FuncIsActiveTask(Task_CeItemDescriptionUp); // FALSE if every task slot was taken
}

bool32 CeOverworld_HideItemDescription(void)
{
    u8 taskId = FindTaskIdByFunc(Task_CeItemDescriptionUp);

    if (taskId == TASK_NONE)
        return FALSE;
    DestroyTask(taskId);
    return TRUE;
}
