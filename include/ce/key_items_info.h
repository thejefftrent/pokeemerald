// gItemsInfo entries of the QoL key items (ce/key_items.h). Not a normal header: it is
// #included inside the gItemsInfo initializer at the end of src/data/items.h, so it may
// only hold designated initializers and uses that file's ITEM_NAME.
//
// Names and descriptions fit the bag (test/text.c): names at most 53px in FONT_NARROWER,
// description lines at most 97px in FONT_NORMAL.

    // Must stay exactly ItemUseOutOfBattle_RareCandy with holdEffectParam 0: party_menu.c
    // compares this pointer to keep the party menu open between uses, and pokemon.c reads a
    // zero param as "one level". Being a key item is what stops ItemUseCB_RareCandy from
    // using it up (CeLevelCap_IsUnlimitedItem); the level cap blocks it like a Rare Candy.
    [ITEM_CE_CANDY_JAR] =
    {
        .name = ITEM_NAME("Candy Jar"),
        .price = 0,
        .description = COMPOUND_STRING(
            "Raises a Pokémon's\n"
            "level by one. It\n"
            "never runs out."),
        .importance = 1,
        .pocket = POCKET_KEY_ITEMS,
        .type = ITEM_USE_PARTY_MENU,
        .fieldUseFunc = ItemUseOutOfBattle_RareCandy,
        .effect = gItemEffect_RareCandy,
        .iconPic = gItemIcon_RareCandy,
        .iconPalette = gItemIconPalette_RareCandy,
    },

    [ITEM_CE_MOVE_RELEARNER] =
    {
        .name = ITEM_NAME("Move Relearner"),
        .price = 0,
        .description = COMPOUND_STRING(
            "Lets a Pokémon\n"
            "relearn moves it\n"
            "learns by leveling."),
        .importance = 1,
        .pocket = POCKET_KEY_ITEMS,
        .type = ITEM_USE_FIELD,
        .fieldUseFunc = CeItemUse_MoveRelearner,
        .iconPic = gItemIcon_HeartScale,
        .iconPalette = gItemIconPalette_HeartScale,
    },

    [ITEM_CE_MOVE_DELETER] =
    {
        .name = ITEM_NAME("Move Deleter"),
        .price = 0,
        .description = COMPOUND_STRING(
            "Makes a Pokémon\n"
            "forget one of its\n"
            "moves."),
        .importance = 1,
        .pocket = POCKET_KEY_ITEMS,
        .type = ITEM_USE_FIELD,
        .fieldUseFunc = CeItemUse_MoveDeleter,
        .iconPic = gItemIcon_TMCase,
        .iconPalette = gItemIconPalette_TMCase,
    },
