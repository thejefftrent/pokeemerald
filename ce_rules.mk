# Custom Emerald build rules.

CE_BUILD_INFO := $(INCLUDE_DIRS)/ce/build_info.h
AUTO_GEN_TARGETS += $(CE_BUILD_INFO)

# Always run; the script only rewrites the header when CE_BASE_ID or the commit changes.
$(CE_BUILD_INFO): ce-force
	@python3 tools/ce/build_info.py $@

.PHONY: ce-force
ce-force:

# Gift/static/starter encounter table: ids for scripts and C, and the gCeEncounters initializer.
# Always runs: the generator only rewrites changed files, and the check fails the build if a
# script still gives or battles a literal species.
CE_ENCOUNTERS_JSON := ce/encounters.json
CE_ENCOUNTERS_OUT := $(INCLUDE_DIRS)/constants/ce_encounters.h $(DATA_SRC_SUBDIR)/ce_encounters.h
AUTO_GEN_TARGETS += $(CE_ENCOUNTERS_OUT)

$(CE_ENCOUNTERS_OUT) &: $(CE_ENCOUNTERS_JSON) tools/ce/gen_encounters.py ce-force
	@python3 tools/ce/gen_encounters.py $(CE_ENCOUNTERS_JSON) $(CE_ENCOUNTERS_OUT)
	@tools/ce/check_encounter_scripts.sh

# Item gifts and Poké Marts in the map scripts (include/ce/items.h), for the website. Always
# runs: the generator only rewrites the header when the scripts change.
CE_FIELD_ITEMS_OUT := $(DATA_SRC_SUBDIR)/ce_field_items.h
AUTO_GEN_TARGETS += $(CE_FIELD_ITEMS_OUT)

$(CE_FIELD_ITEMS_OUT): tools/ce/gen_field_items.py ce-force
	@python3 tools/ce/gen_field_items.py $@

# Pokémon Center 2F sellers: the 17 maps on LAYOUT_POKEMON_CENTER_2F end with the same eight
# seller objects and the bedrooms keep their hidden placeholders. Always runs with the other
# generated targets; the check fails the build when a map drifts.
.PHONY: ce-check-pc-sellers
AUTO_GEN_TARGETS += ce-check-pc-sellers

ce-check-pc-sellers:
	@python3 tools/ce/check_pc_sellers.py
