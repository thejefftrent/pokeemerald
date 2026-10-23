#include "global.h"
#include "item.h"
#include "pokemon.h"
#include "ce/teachable.h"
#include "constants/moves.h"

STATIC_ASSERT(sizeof(((struct SpeciesInfo *)0)->teachableLearnset) == CE_TEACHABLE_BYTES, CeTeachableFieldSize);
STATIC_ASSERT(CE_TEACHABLE_TUTOR_START + CE_TUTOR_SLOTS <= CE_TEACHABLE_SLOTS, CeTeachableSlotsFit);

#define BIT_SET(bits, slot) (((bits)[(slot) / 8] >> ((slot) % 8)) & 1)

static const u8 *GetBits(u32 species)
{
    return gSpeciesInfo[SanitizeSpeciesId(species)].teachableLearnset;
}

u32 CeTeachable_SlotMove(u32 slot)
{
    if (slot < CE_TEACHABLE_TUTOR_START)
        return GetTMHMMoveId(slot + 1);
    if (slot < CE_TEACHABLE_TUTOR_START + CE_TUTOR_SLOTS)
        return gCeTutorMoves[slot - CE_TEACHABLE_TUTOR_START];
    return MOVE_NONE;
}

bool32 CeTeachable_HasSlot(u32 species, u32 slot)
{
    return slot < CE_TEACHABLE_SLOTS && BIT_SET(GetBits(species), slot);
}

bool32 CeTeachable_CanLearn(u32 species, u32 move)
{
    const u8 *bits;
    u32 slot;

    if (species == SPECIES_EGG || move == MOVE_NONE)
        return FALSE;
    bits = GetBits(species);
    for (slot = 0; slot < CE_TEACHABLE_SLOTS; slot++)
    {
        if (BIT_SET(bits, slot) && CeTeachable_SlotMove(slot) == move)
            return TRUE;
    }
    return FALSE;
}

u32 CeTeachable_GetMoves(u32 species, u16 *moves)
{
    const u8 *bits;
    u32 slot, i, count = 0;

    if (species == SPECIES_EGG)
        return 0;
    bits = GetBits(species);
    for (slot = 0; slot < CE_TEACHABLE_SLOTS; slot++)
    {
        u32 move;
        if (!BIT_SET(bits, slot))
            continue;
        move = CeTeachable_SlotMove(slot);
        if (move == MOVE_NONE)
            continue;
        for (i = 0; i < count && moves[i] != move; i++)
            ;
        if (i == count)
            moves[count++] = move;
    }
    return count;
}
