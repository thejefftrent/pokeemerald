#!/usr/bin/env python3
"""Fail the build if the Pokémon Center 2F sellers drift (docs/qol-shops.md in the outer repo).

The sellers are eight object events appended by hand to the 17 map.json files on
LAYOUT_POKEMON_CENTER_2F, shown or hidden by CeProgression_UpdatePcSellers (src/ce/progression.c)
through FLAG_TEMP_5..FLAG_TEMP_C. This checks that:
  - all 17 maps exist, each ends with exactly the eight agreed objects (tile, sprite, flag,
    script, facing), no other object stands on their tiles or on the row they're talked to from,
    and the map has at most 13 object templates (13 + player + follower = 15 of the 16 objects);
  - each map's OnTransition is CableClub_OnTransition, whose first line updates the sellers;
  - sPcSellers in src/ce/progression.c lists the same flags in the same order;
  - both bedrooms keep the five old sellers as hidden placeholders (same flags, PcSeller scripts)
    and still call the update, so saves from 1.17.1-ce2/ce3 made there stay valid.
Runs on every build (ce_rules.mk).
"""
import json
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))

# (x, sprite, script, hide flag), all on row 8, facing up, talked to from row 7.
SELLERS = [
    (1, 'OBJ_EVENT_GFX_WOMAN_2', 'CE_EventScript_PcSellerTraining', 'FLAG_TEMP_5'),
    (2, 'OBJ_EVENT_GFX_SCIENTIST_1', 'CE_EventScript_PcSellerEvolutionItems', 'FLAG_TEMP_6'),
    (3, 'OBJ_EVENT_GFX_GENTLEMAN', 'CE_EventScript_PcSellerMegaStones', 'FLAG_TEMP_7'),
    (4, 'OBJ_EVENT_GFX_BLACK_BELT', 'CE_EventScript_PcSellerZCrystals', 'FLAG_TEMP_8'),
    (7, 'OBJ_EVENT_GFX_HEX_MANIAC', 'CE_EventScript_PcSellerGemsPlatesMemories', 'FLAG_TEMP_9'),
    (11, 'OBJ_EVENT_GFX_EXPERT_M', 'CE_EventScript_PcSellerBattleItems', 'FLAG_TEMP_A'),
    (12, 'OBJ_EVENT_GFX_OLD_WOMAN', 'CE_EventScript_PcSellerRelearner', 'FLAG_TEMP_B'),
    (13, 'OBJ_EVENT_GFX_POKEFAN_F', 'CE_EventScript_PcSellerWonderTrade', 'FLAG_TEMP_C'),
]
SELLER_ROW = 8
TALK_ROW = 7
EXPECTED_MAPS = 17
MAX_TEMPLATES = 13
HOOK = 'callnative CeProgression_ScriptUpdatePcSellers, requests_effects=1'
BEDROOMS = ['LittlerootTown_BrendansHouse_2F', 'LittlerootTown_MaysHouse_2F']

errors = []


def fail(msg):
    errors.append(msg)


def seller_fields(o):
    return (o.get('graphics_id'), o.get('script'), o.get('flag'))


def check_pc_map(name, data):
    objs = data['object_events']
    if len(objs) > MAX_TEMPLATES:
        fail(f'{name}: {len(objs)} object templates, at most {MAX_TEMPLATES} fit beside the player and a follower')
    if len(objs) < len(SELLERS):
        fail(f'{name}: fewer objects than the {len(SELLERS)} sellers')
        return
    tail = objs[-len(SELLERS):]
    for (x, gfx, script, flag), o in zip(SELLERS, tail):
        want = {
            'graphics_id': gfx, 'x': x, 'y': SELLER_ROW, 'elevation': 3,
            'movement_type': 'MOVEMENT_TYPE_FACE_UP', 'movement_range_x': 0, 'movement_range_y': 0,
            'trainer_type': 'TRAINER_TYPE_NONE', 'script': script, 'flag': flag,
        }
        for k, v in want.items():
            if o.get(k) != v:
                fail(f'{name}: seller {script} has {k} {o.get(k)!r}, expected {v!r}')
        if 'local_id' in o:
            fail(f'{name}: seller {script} has a local_id name; local ids differ between the maps')
    seller_tiles = {(x, SELLER_ROW) for x, _, _, _ in SELLERS}
    talk_tiles = {(x, TALK_ROW) for x, _, _, _ in SELLERS}
    for o in objs[:-len(SELLERS)]:
        pos = (o.get('x'), o.get('y'))
        if pos in seller_tiles:
            fail(f'{name}: {o.get("graphics_id")} stands on a seller tile {pos}')
        if o.get('movement_range_x', 0) == 0 and o.get('movement_range_y', 0) == 0 and pos in talk_tiles:
            fail(f'{name}: {o.get("graphics_id")} blocks the tile a seller is talked to from {pos}')
        if str(o.get('script', '')).startswith('CE_EventScript_PcSeller'):
            fail(f'{name}: a seller script on object {o.get("graphics_id")} outside the eight sellers')

    scripts = os.path.join(ROOT, 'data', 'maps', name, 'scripts.inc')
    with open(scripts, encoding='utf-8') as f:
        text = f.read()
    if not re.search(r'map_script\s+MAP_SCRIPT_ON_TRANSITION\s*,\s*CableClub_OnTransition\b', text):
        fail(f'{name}: OnTransition is not CableClub_OnTransition, so the sellers never update')


def check_cable_club():
    path = os.path.join(ROOT, 'data', 'scripts', 'cable_club.inc')
    with open(path, encoding='utf-8') as f:
        lines = f.read().split('\n')
    try:
        i = lines.index('CableClub_OnTransition:')
    except ValueError:
        fail('data/scripts/cable_club.inc: no CableClub_OnTransition label')
        return
    if HOOK not in lines[i + 1]:
        fail('data/scripts/cable_club.inc: the first line of CableClub_OnTransition must update the PC sellers')


def check_seller_table():
    path = os.path.join(ROOT, 'src', 'ce', 'progression.c')
    with open(path, encoding='utf-8') as f:
        text = f.read()
    m = re.search(r'sPcSellers\[\]\s*=\s*\{(.*?)\n\};', text, re.S)
    if not m:
        fail('src/ce/progression.c: no sPcSellers table')
        return
    flags = re.findall(r'\{\s*(FLAG_TEMP_[0-9A-F]+)\s*,', m.group(1))
    want = [flag for _, _, _, flag in SELLERS]
    if flags != want:
        fail(f'src/ce/progression.c: sPcSellers flags {flags}, expected the map order {want}')


def check_bedroom(name):
    with open(os.path.join(ROOT, 'data', 'maps', name, 'map.json'), encoding='utf-8') as f:
        objs = json.load(f)['object_events']
    placeholders = [o for o in objs if str(o.get('script', '')).startswith('CE_EventScript_')]
    want = [(gfx, script, flag) for _, gfx, script, flag in SELLERS[:5]]
    got = [seller_fields(o) for o in placeholders]
    if got != want or objs[-5:] != placeholders:
        fail(f'{name}: the last five objects must be the old sellers kept as hidden placeholders {want}, found {got}')
    with open(os.path.join(ROOT, 'data', 'maps', name, 'scripts.inc'), encoding='utf-8') as f:
        if HOOK not in f.read():
            fail(f'{name}: OnTransition must still call CeProgression_ScriptUpdatePcSellers to hide the placeholders')


def main():
    maps_dir = os.path.join(ROOT, 'data', 'maps')
    count = 0
    for name in sorted(os.listdir(maps_dir)):
        path = os.path.join(maps_dir, name, 'map.json')
        if not os.path.isfile(path):
            continue
        with open(path, encoding='utf-8') as f:
            data = json.load(f)
        if data.get('layout') != 'LAYOUT_POKEMON_CENTER_2F':
            continue
        count += 1
        check_pc_map(name, data)
    if count != EXPECTED_MAPS:
        fail(f'{count} maps on LAYOUT_POKEMON_CENTER_2F, expected {EXPECTED_MAPS} (update docs/qol-shops.md and this check)')
    check_cable_club()
    check_seller_table()
    for name in BEDROOMS:
        check_bedroom(name)
    if errors:
        print('error: Pokémon Center 2F sellers (tools/ce/check_pc_sellers.py):', file=sys.stderr)
        for e in errors:
            print('  ' + e, file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
