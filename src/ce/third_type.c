#include "global.h"
#include "battle_util.h"
#include "pokemon.h"
#include "ce/third_type.h"

static bool32 IsRealType(u32 type)
{
    return type != TYPE_NONE && type != TYPE_MYSTERY && type != TYPE_STELLAR && type < NUMBER_OF_MON_TYPES;
}

enum Type CeSpecies_GetThirdType(enum Species species)
{
    u32 type;

    species = SanitizeSpeciesId(species);
    type = gCeSpeciesThirdType[species];
    if (!IsRealType(type) || type == GetSpeciesType(species, 0) || type == GetSpeciesType(species, 1))
        return TYPE_MYSTERY;
    return type;
}

bool32 CeSpecies_IsOfType(enum Species species, enum Type type)
{
    return GetSpeciesType(species, 0) == type
        || GetSpeciesType(species, 1) == type
        || (type != TYPE_MYSTERY && CeSpecies_GetThirdType(species) == type);
}

s32 CeThirdType_StealthHazardDamage(u32 hazardType, const enum Type types[3], u32 maxHp)
{
    uq4_12_t modifier = GetTypeModifier(hazardType, types[0]);
    s32 damage;

    if (types[1] != types[0])
        modifier = uq4_12_multiply(modifier, GetTypeModifier(hazardType, types[1]));
    if (types[2] != TYPE_MYSTERY && types[2] != types[1] && types[2] != types[0])
        modifier = uq4_12_multiply(modifier, GetTypeModifier(hazardType, types[2]));
    if (modifier == UQ_4_12(0.0))
        return 0;

    // An eighth of max HP at neutral, scaled by the multiplier (1/64 at 0.125x, all of it at 8x).
    damage = (maxHp * modifier) / (8 * UQ_4_12(1.0));
    if (damage == 0)
        damage = 1;
    return damage;
}
