#include "global.h"
#include "item.h"
#include "money.h"
#include "ce/rom_config.h"
#include "ce/start.h"
#include "constants/items.h"

static bool32 IsValidEntry(const struct CeStartItem *entry)
{
    return entry->item != ITEM_NONE && entry->item < ITEMS_COUNT && entry->quantity != 0;
}

void CeStart_OnNewGame(void)
{
    const struct CeStartConfig *config = &gCeRomConfig.body.start;
    u32 i;

    if (config->setMoney)
        SetMoney(&gSaveBlock1Ptr->money, min(config->money, MAX_MONEY));

    if (config->replacePcItems)
    {
        CpuFastFill(0, gSaveBlock1Ptr->pcItems, sizeof(gSaveBlock1Ptr->pcItems));
        for (i = 0; i < CE_START_PC_ITEMS && config->pc[i].item != ITEM_NONE; i++)
        {
            if (IsValidEntry(&config->pc[i]))
                AddPCItem(config->pc[i].item, min(config->pc[i].quantity, MAX_PC_ITEM_CAPACITY));
        }
    }

    for (i = 0; i < CE_START_BAG_ITEMS && config->bag[i].item != ITEM_NONE; i++)
    {
        if (IsValidEntry(&config->bag[i]))
            AddBagItem(config->bag[i].item, min(config->bag[i].quantity, MAX_BAG_ITEM_CAPACITY));
    }
}
