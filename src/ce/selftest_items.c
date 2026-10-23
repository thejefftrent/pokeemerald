#include "global.h"
#include "event_data.h"
#include "item_ball.h"
#include "overworld.h"
#include "script.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "ce/items.h"
#include "ce/selftest.h"
#include "../data/map_group_count.h"

// Items self-test: what the game reads for every item ball, hidden item, item gift and
// Poké Mart, through the code paths the overworld and the script commands use. Checked by
// tools/canary/checks/items.ts.

extern const struct MapHeader *const *const gMapGroups[];
extern ScrCmdFunc gScriptCmdTable[];
extern ScrCmdFunc gScriptCmdTableEnd[];

#define CE_ITEMS_SCAN_MAX 0x2000 // bytes searched after an anchor that has no limit
#define CE_ITEMS_MART_MAX 256
#define CE_ITEMS_ROM_START 0x08000000
#define CE_ITEMS_ROM_END   0x0A000000

static bool32 IsGift(const u8 *p)
{
    return p[0] == gCeItemScripts.opSetOrCopyVar && T1_READ_16(p + 1) == VAR_0x8000
        && p[5] == gCeItemScripts.opSetOrCopyVar && T1_READ_16(p + 6) == VAR_0x8001
        && p[10] == gCeItemScripts.opCallStd && p[11] == gCeItemScripts.stdObtainItem;
}

static const u16 *MartList(const u8 *p)
{
    const u16 *list;
    u32 i;

    if (p[0] != gCeItemScripts.opPokemart)
        return NULL;
    list = (const u16 *)T1_READ_PTR(p + 1);
    if ((u32)list < CE_ITEMS_ROM_START || (u32)list >= CE_ITEMS_ROM_END || ((u32)list & 1))
        return NULL;
    for (i = 0; i < CE_ITEMS_MART_MAX; i++)
    {
        if (list[i] == ITEM_NONE)
            return list;
        if (list[i] >= ITEMS_COUNT)
            return NULL;
    }
    return NULL;
}

// The command a site describes: the nth + 1-th match between its anchor and limit.
static const u8 *FindSite(const struct CeItemScriptSite *site)
{
    const u8 *p = site->anchor;
    const u8 *end = site->limit != NULL ? site->limit : site->anchor + CE_ITEMS_SCAN_MAX;
    u32 seen = 0;

    if (site->limit != NULL && site->limit <= site->anchor)
        return NULL; // not assembled (the label is in an .if branch)
    for (; p + 12 <= end; p++)
    {
        if (site->kind == CE_ITEM_SITE_GIFT ? IsGift(p) : MartList(p) != NULL)
        {
            if (seen++ == site->nth)
                return p;
        }
    }
    return NULL;
}

// Item balls: Common_EventScript_FindItem's callnative, for the object the player talked to.
static void ProbeItemBalls(u16 mapId, const struct MapHeader *header)
{
    const struct MapEvents *events = header->events;
    const struct MapEvents *saved = gMapHeader.events;
    u32 i;

    gMapHeader.events = events;
    for (i = 0; i < events->objectEventCount; i++)
    {
        if (events->objectEvents[i].script != gCeItemScripts.findItemScript)
            continue;
        gSpecialVar_LastTalked = events->objectEvents[i].localId;
        gSpecialVar_Result = 0xFFFF;
        gSpecialVar_0x8009 = 0xFFFF;
        GetItemBallIdAndAmountFromTemplate();
        CeSelfTest_LineStart("itemBall");
        CeSelfTest_PutField("map", mapId);
        CeSelfTest_PutField("localId", events->objectEvents[i].localId);
        CeSelfTest_PutField("item", gSpecialVar_Result);
        CeSelfTest_PutField("amount", gSpecialVar_0x8009);
        CeSelfTest_LineEmit();
    }
    gMapHeader.events = saved;
}

// Hidden items: the values GetInteractedBackgroundEventScript loads into the special vars.
static void ProbeHiddenItems(u16 mapId, const struct MapHeader *header)
{
    const struct MapEvents *events = header->events;
    u32 i;

    for (i = 0; i < events->bgEventCount; i++)
    {
        const struct BgEvent *bg = &events->bgEvents[i];
        if (bg->kind != gCeItemScripts.bgEventHiddenItem)
            continue;
        CeSelfTest_LineStart("hiddenItem");
        CeSelfTest_PutField("map", mapId);
        CeSelfTest_PutField("index", i);
        CeSelfTest_PutField("flag", bg->bgUnion.hiddenItem.hiddenItemId + FLAG_HIDDEN_ITEMS_START);
        CeSelfTest_PutField("item", bg->bgUnion.hiddenItem.item);
        CeSelfTest_PutField("quantity", bg->bgUnion.hiddenItem.quantity);
        CeSelfTest_LineEmit();
    }
}

// Item gifts: run giveitem's two setorcopyvar commands through the script command table.
static void ProbeGift(u32 id, const u8 *p)
{
    struct ScriptContext ctx;
    u32 i;

    gSpecialVar_0x8000 = 0xFFFF;
    gSpecialVar_0x8001 = 0xFFFF;
    if (p != NULL)
    {
        InitScriptContext(&ctx, gScriptCmdTable, gScriptCmdTableEnd);
        SetupBytecodeScript(&ctx, p);
        for (i = 0; i < 2; i++)
            gScriptCmdTable[ScriptReadByte((&ctx))](&ctx);
    }
    CeSelfTest_LineStart("itemGift");
    CeSelfTest_PutField("site", id);
    CeSelfTest_PutField("found", p != NULL);
    CeSelfTest_PutField("item", gSpecialVar_0x8000);
    CeSelfTest_PutField("amount", gSpecialVar_0x8001);
    CeSelfTest_LineEmit();
}

// Poké Marts: the list pointer as ScrCmd_pokemart reads it, then the items up to ITEM_NONE
// (what the shop menu lists).
static void ProbeMart(u32 id, const u8 *p)
{
    struct ScriptContext ctx;
    const u16 *list = NULL;
    u32 i, count = 0;

    if (p != NULL)
    {
        InitScriptContext(&ctx, gScriptCmdTable, gScriptCmdTableEnd);
        SetupBytecodeScript(&ctx, p + 1);
        list = (const u16 *)ScriptReadWord(&ctx);
        while (list[count] != ITEM_NONE && count < CE_ITEMS_MART_MAX)
            count++;
    }
    CeSelfTest_LineStart("itemMart");
    CeSelfTest_PutField("site", id);
    CeSelfTest_PutField("found", p != NULL);
    CeSelfTest_PutField("count", count);
    CeSelfTest_LineEmit();
    for (i = 0; i < count; i++)
    {
        CeSelfTest_LineStart("itemMartItem");
        CeSelfTest_PutField("site", id);
        CeSelfTest_PutField("i", i);
        CeSelfTest_PutField("item", list[i]);
        CeSelfTest_LineEmit();
    }
}

void CeSelfTest_ProbeItems(void)
{
    u32 group, num, i;

    for (group = 0; group < MAP_GROUPS_COUNT; group++)
    {
        if (gMapGroups[group] == NULL)
            continue;
        for (num = 0; num < MAP_GROUP_COUNT[group]; num++)
        {
            const struct MapHeader *header = Overworld_GetMapHeaderByGroupAndId(group, num);
            if (header == NULL || header->events == NULL)
                continue;
            ProbeItemBalls((group << 8) | num, header);
            ProbeHiddenItems((group << 8) | num, header);
        }
    }

    for (i = 0; i < gCeItemScripts.siteCount; i++)
    {
        const struct CeItemScriptSite *site = &gCeItemScripts.sites[i];
        if (site->kind == CE_ITEM_SITE_GIFT)
            ProbeGift(i, FindSite(site));
        else
            ProbeMart(i, FindSite(site));
    }
}
