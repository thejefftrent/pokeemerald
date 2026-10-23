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
