#ifndef GUARD_CE_RIVAL_NAMES_H
#define GUARD_CE_RIVAL_NAMES_H

// Rival names (packages/engine/src/sprites/names.ts). The website can rename Brendan and May; the
// player's choice is a cosmetic, like the player sprites. The names live in the patchable
// buffers gText_ExpandedPlaceholder_Brendan/_May (src/strings.c), which already feed {RIVAL},
// the Trainer Fan Club and the PokeNav rival entries.
//
// In battle the game names an opponent from gTrainers, which the website never rewrites (trainer
// data, the Trainers page and the spoiler keep BRENDAN and MAY). This maps the rival trainers'
// names to the buffers instead.

// For a regular trainer opponent: Brendan's or May's buffer when the trainer is a rival (class
// Rival) drawn with Brendan's or May's picture, else `name` unchanged. Partners and every other
// trainer (Wally, Steven, ...) keep `name`.
const u8 *CeRivalNames_OpponentName(u16 trainerId, const u8 *name);

#endif // GUARD_CE_RIVAL_NAMES_H
