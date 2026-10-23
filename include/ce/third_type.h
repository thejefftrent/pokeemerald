#ifndef GUARD_CE_THIRD_TYPE_H
#define GUARD_CE_THIRD_TYPE_H

// Every species can have an innate third type, like the one Forest's Curse and Trick-or-Treat
// add in battle. The website writes gCeSpeciesThirdType; TYPE_MYSTERY means none (vanilla).
//
// In battle the third type lives in gBattleMons[b].types[2], the slot those two moves use, so
// type effectiveness, STAB and IS_BATTLER_OF_TYPE see it without changes. It is set wherever
// the game used to clear that slot (sending out, switching in, Mimicry ending, form changes),
// so anything that replaces the battler's types (Soak, Conversion, Protean, Reflect Type,
// Transform, Terastallization) replaces it too, and Forest's Curse and Trick-or-Treat swap it
// for their own type. See docs/third-type.md on the website side.
//
// Included from pokemon.h right after GetSpeciesType, where enum Species and enum Type exist.

extern const u8 gCeSpeciesThirdType[NUM_SPECIES];

// The species' third type, or TYPE_MYSTERY when it has none. A stored value that isn't a real
// type, or that repeats the species' first or second type, also reads as none.
enum Type CeSpecies_GetThirdType(enum Species species);

// TRUE when `type` is the species' first, second or third type. Use it wherever the game asks
// whether a species is of some type outside battle.
bool32 CeSpecies_IsOfType(enum Species species, enum Type type);

// Stealth Rock and Steelsurge damage against all three of a battler's types (vanilla counted
// only the first two, which missed both innate and Forest's Curse-added third types).
s32 CeThirdType_StealthHazardDamage(u32 hazardType, const enum Type types[3], u32 maxHp);

#endif // GUARD_CE_THIRD_TYPE_H
