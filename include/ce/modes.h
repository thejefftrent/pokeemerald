#ifndef GUARD_CE_MODES_H
#define GUARD_CE_MODES_H

// Game modes: Nuzlocke, hard mode, monotype, single-Pokémon, no-heal, no-evolution and
// no-experience challenges.
//
// The website writes the rules into gCeRomConfig.body.modes. New Game copies them into the
// save (struct CeModesSave, in spare SaveBlock2 space) together with the ROM's config hash,
// and from then on the game reads only the save's copy. Re-patching the ROM therefore never
// changes the rules of a run in progress, and a save started without modes stays vanilla.
//
// Every field is 0 for vanilla behaviour. This header is included from global.h (for the
// save struct), so it only uses basic types.

struct Pokemon;
struct BoxPokemon;
struct ScriptContext;

// The rules. Copied verbatim into the save, so the size is fixed: new rules took the reserved
// bytes (noExp and whiteoutEndsRun used the last two) and keep 0 = off. Field names are the
// website's interface (DWARF).
struct CeModesConfig
{
    // Nuzlocke
    u8 nuzlocke;        // only the first wild encounter in each area can be caught; fainted = dead
    u8 dupesClause;     // a first encounter whose evolution family you already own doesn't count
    u8 shinyClause;     // shiny Pokémon can always be caught and don't use up the area
    u8 releaseDead;     // 0 = dead Pokémon stay (they can't be healed or revived), 1 = released after battle
    u8 forceNicknames;  // the naming screen always opens after a catch
    // Hard mode (trainer levels, IVs and AI are written into gTrainers by the website)
    u8 noBattleItems;   // no bag in trainer battles; only Poké Balls in wild battles
    u8 forceSetStyle;   // battle style locked to Set
    // Monotype
    u8 monotype;        // TYPE_x: only Pokémon of this type can be caught (TYPE_NONE = off)
    // Single-Pokémon
    u16 onlySpecies;    // only this evolution family (its root species) can be caught (0 = any)
    u8 partyLimit;      // 1-5: Pokémon beyond this go to the PC (0 = PARTY_SIZE)
    // No-heal
    u8 noCenterHeal;    // Pokémon Center nurses don't heal
    u8 noRestHeal;      // Mom, beds and rest stops don't heal
    // No evolutions
    u8 noEvolution;     // no Pokémon evolves, by any method (GetEvolutionTargetSpecies)
    // No experience
    u8 noExp;           // enum CeNoExp
    // Nuzlocke (in what was reserved space, so saves from before it read 0 = off)
    u8 whiteoutEndsRun; // whiting out ends the run: the game says so and erases the save
};

// CeModesConfig.noExp: how Pokémon may still gain levels.
enum CeNoExp
{
    CE_NO_EXP_OFF,
    CE_NO_EXP_BATTLE, // no Exp. from any battle (Cmd_getexp); EVs are still gained
    CE_NO_EXP_ALL,    // nor from Rare Candies, Exp. Candies or the Candy Jar, and the Day Care takes no Pokémon
};

// Spare SaveBlock2 space: struct Pokedex's old dex-flag filler (see global.h), next to the
// QoL bits. Saves made before this existed have zeros here, which means "no modes".
#define CE_MODES_SAVE_MAGIC   0xCE
#define CE_MODES_SAVE_VERSION 1
#define CE_MODES_AREA_WORDS   7 // 224 areas: every MAPSEC (checked in modes.c)

struct CeModesSave
{
    u8 magic;           // CE_MODES_SAVE_MAGIC once New Game has copied the rules
    u8 version;         // CE_MODES_SAVE_VERSION
    u16 deaths;         // Nuzlocke: Pokémon that died in this run
    u32 configHash;     // gCeRomConfig.hdr.configHash of the ROM that started the run
    struct CeModesConfig rules;
    u32 areaUsed[CE_MODES_AREA_WORDS]; // Nuzlocke: bit n = area n's encounter is used up
};

// gCeAreaOfMapsec[mapsec]: which Nuzlocke area a map section belongs to.
#define CE_AREA_OWN  0    // its own area (the map section's id)
#define CE_AREA_FREE 0xFF // no first-encounter rule here
// Any other value n: area n - 1 (map sections the website merged into one area).
#define CE_AREA_NONE 0xFFFF

// Special flag (RAM only, never saved): the current wild battle was started by a script.
#define FLAG_CE_SCRIPTED_WILD_BATTLE (SPECIAL_FLAGS_START + 0x7F)

// How the current battle relates to the rules.
enum CeBattleKind
{
    CE_BATTLE_WILD,     // a random wild encounter: every rule applies
    CE_BATTLE_SCRIPTED, // a static, legendary or roaming Pokémon: no Nuzlocke area rule
    CE_BATTLE_TRAINER,  // deaths count, nothing to catch
    CE_BATTLE_EXEMPT,   // Wally's tutorial, Birch's rescue, Safari Zone, Battle Frontier,
                        // Trainer Hill, secret bases, link and recorded battles: no rules
};

// Why a Poké Ball can't be thrown.
enum CeCatchBlock
{
    CE_CATCH_OK,
    CE_CATCH_AREA_USED, // Nuzlocke: not the first encounter in this area
    CE_CATCH_DUPE,      // Nuzlocke dupes clause: this family is already owned
    CE_CATCH_MONOTYPE,  // not of the run's type
    CE_CATCH_SPECIES,   // not the single-Pokémon family
    CE_CATCH_STATIC_TEAM, // static team: nothing new can join (ce/team.h)
};

// Heals the no-heal mode can refuse.
enum CeHealSource
{
    CE_HEAL_CENTER,
    CE_HEAL_REST,
};

// The rules in force: the save's copy, or all-zero if this run has none.
const struct CeModesConfig *CeModes_Rules(void);
bool32 CeModes_SaveHasRules(void);

// Hooks: NewGameInitData (copy the rules), HandleEndTurn_FinishBattle (deaths and areas),
// ReturnFromBattleToOverworld (release the dead), CB2_WhiteOut (the run is lost).
void CeModes_OnNewGame(void);
void CeModes_OnBattleEnd(void);
void CeModes_AfterBattle(void);

// Catching (GetBallThrowableState and CannotUseItemsInBattle).
enum CeBattleKind CeModes_BattleKind(void);
enum CeCatchBlock CeModes_CatchRule(u16 species, bool32 shiny, u32 area, enum CeBattleKind kind);
enum CeCatchBlock CeModes_CurrentCatchBlock(void);
const u8 *CeModes_CatchBlockedText(void);
bool32 CeModes_ForcesNickname(void);

// Nuzlocke areas, families and the dead.
u32 CeModes_AreaOfMapsec(u32 mapsec);
u32 CeModes_CurrentArea(void);
bool32 CeModes_IsAreaUsed(u32 area);
void CeModes_UseArea(u32 area);
u16 CeModes_FamilyRoot(u16 species);
bool32 CeModes_FamilyOwned(u16 species);
bool32 CeModes_CountsAsEncounter(u16 species, bool32 shiny);
bool32 CeModes_IsDeadBox(const struct BoxPokemon *boxMon);
bool32 CeModes_IsDead(const struct Pokemon *mon);
// Nuzlocke only: mark the party Pokémon in slotMask that have fainted as dead (returns how many).
u32 CeModes_MarkFaintedDead(u32 slotMask);
// Nuzlocke with releaseDead only: release dead party Pokémon (their held items go to the bag),
// unless no living Pokémon would remain. Returns how many were released.
u32 CeModes_ReleaseDead(void);
// Nuzlocke with whiteoutEndsRun only (src/ce/run_lost.c). CB2_WhiteOut asks first: then the
// run-lost screen erases the save and resets to the title screen instead of the whiteout warp.
bool32 CeModes_WhiteoutEndsRun(void);
bool32 CeModes_TryEndRunOnWhiteout(void);
void CB2_CeRunLost(void);

// Hard mode.
bool32 CeModes_NoBagInTrainerBattles(void);
bool32 CeModes_BlocksBattleItem(u16 item);
const u8 *CeModes_NoItemsText(void);
bool32 CeModes_ForcesSetStyle(void);

// No evolutions.
bool32 CeModes_NoEvolution(void);

// No experience: Cmd_getexp, ItemUseOutOfBattle_RareCandy and the Day Care lady's script.
bool32 CeModes_NoBattleExp(void);
bool32 CeModes_NoLevelItems(void);
const u8 *CeModes_NoLevelItemsText(void);
bool32 CeModes_DaycareClosed(void);
void CeModes_ScriptDaycareGate(struct ScriptContext *ctx);

// Single-Pokémon.
u32 CeModes_PartyLimit(void);
void CeModes_EnforcePartyLimit(void);

// No-heal: script natives that jump to a "can't heal" script when the rules refuse.
bool32 CeModes_CanHeal(enum CeHealSource source);
void CeModes_ScriptCenterHealGate(struct ScriptContext *ctx);
void CeModes_ScriptRestHealGate(struct ScriptContext *ctx);

#endif // GUARD_CE_MODES_H
