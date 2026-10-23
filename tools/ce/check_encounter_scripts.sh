#!/usr/bin/env bash
# Fail if a script still gives, hatches or battles a literal SPECIES_ constant. Those encounters
# must come from gCeEncounters (ce/encounters.json) through the macros in
# asm/macros/ce_encounters.inc (ce_givemon, ce_giveegg, setwildbattle_ce, seteventmon_ce,
# ce_loadenc), or the website can't change them. Runs on every build (ce_rules.mk).
#
# Allowlist: files that aren't fixed encounters of the Emerald game.
#   data/maps/*_Frlg/        FireRed/LeafGreen maps; Emerald builds leave them out (MAP_VERSION)
#   data/scripts/debug.inc   the debug menu, not in release builds
#   data/scripts/gift_*.inc  Mystery Event scripts (data/mystery_gift.s), copied to RAM from event data
set -euo pipefail
cd "$(dirname "$0")/../.."

pattern='^[[:space:]]*((givemon|giveegg|setwildbattle|seteventmon)[[:space:]]+SPECIES_|createmon[[:space:]]+[^,]+,[[:space:]]*[^,]+,[[:space:]]*SPECIES_)'
hits="$(grep -rnE "$pattern" data --include='*.inc' --include='*.s' --include='*.pory' \
  | grep -vE '^data/maps/[^/]*_Frlg/|^data/scripts/debug\.inc:|^data/scripts/gift_[a-z_]*\.inc:' || true)"

if [ -n "$hits" ]; then
  echo "error: scripts give or battle a literal species; add it to ce/encounters.json and use the CE_ENC_ macros:" >&2
  echo "$hits" >&2
  exit 1
fi
