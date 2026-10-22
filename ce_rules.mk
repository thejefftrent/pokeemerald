# Custom Emerald build rules.

CE_BUILD_INFO := $(INCLUDE_DIRS)/ce/build_info.h
AUTO_GEN_TARGETS += $(CE_BUILD_INFO)

# Always run; the script only rewrites the header when CE_BASE_ID or the commit changes.
$(CE_BUILD_INFO): ce-force
	@python3 tools/ce/build_info.py $@

.PHONY: ce-force
ce-force:
