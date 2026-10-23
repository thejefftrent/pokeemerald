#include "global.h"
#include "pokemon_icon.h"
#include "sprite.h"
#include "ce/rom_config.h"
#include "ce/seed_hash.h"
#include "ce/selftest.h"

// The strip under a three-item main menu (the OPTION window's frame ends at y = 127):
// five 32x32 icons, 40 pixels apart, centred on the screen.
#define HASH_MENU_MAX_ITEMS 3
#define HASH_ICON_Y         140 // icons are opaque from row 5: clears the frame, ends at 156
#define HASH_ICON_SPACING   40
#define HASH_ICON_X0        (DISPLAY_WIDTH / 2 - 2 * HASH_ICON_SPACING)
// Low personality byte 0xFF reads as male, so species with a female icon show the usual one.
#define HASH_ICON_PERSONALITY 0xFF

bool32 CeSeedHash_IsPresent(void)
{
    u32 i;
    for (i = 0; i < CE_SEED_HASH_LENGTH; i++)
    {
        u16 species = gCeRomConfig.hdr.seedHash[i];
        if (species == SPECIES_NONE || species >= NUM_SPECIES)
            return FALSE;
    }
    return TRUE;
}

void CeSeedHash_ShowOnMainMenu(u32 menuItemCount)
{
    u32 i;

    if (menuItemCount > HASH_MENU_MAX_ITEMS || !CeSeedHash_IsPresent())
        return;
    for (i = 0; i < CE_SEED_HASH_LENGTH; i++)
    {
        u16 species = gCeRomConfig.hdr.seedHash[i];
        LoadMonIconPalette(species);
        CreateMonIcon(species, SpriteCB_MonIcon, HASH_ICON_X0 + i * HASH_ICON_SPACING, HASH_ICON_Y, 0, HASH_ICON_PERSONALITY);
    }
}

// CE:{"probe":"seedHash","present":1,"s0":...,"s4":...}: the species the main menu would show.
void CeSelfTest_ProbeSeedHash(void)
{
    static const char *const names[CE_SEED_HASH_LENGTH] = { "s0", "s1", "s2", "s3", "s4" };
    u32 i;

    CeSelfTest_LineStart("seedHash");
    CeSelfTest_PutField("present", CeSeedHash_IsPresent());
    for (i = 0; i < CE_SEED_HASH_LENGTH; i++)
        CeSelfTest_PutField(names[i], gCeRomConfig.hdr.seedHash[i]);
    CeSelfTest_LineEmit();
}
