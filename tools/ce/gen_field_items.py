#!/usr/bin/env python3
"""Find every item gift (giveitem) and Poké Mart (pokemart) in the map scripts.

    python3 tools/ce/gen_field_items.py src/data/ce_field_items.h

Reads data/event_scripts.s and the script files it includes, data/maps/map_groups.json and
each map's map.json, and writes the gCeItemScriptSites / gCeItemScriptOwners initializers
(include/ce/items.h has the structs). The website uses them to find, edit and verify the
script commands; the game never reads them (only the self-test probe does).

For each command it records:
- anchor: the nearest global (::) label above the command, which C can name, and limit: the
  next global label below it. The engine finds the command's bytes between the two.
- nth: how many giveitem (or pokemart) commands lie between the anchor and this one, so the
  n-th match of the command's fixed bytes is this one.
- name: the nearest label of any kind above it, plus "#n" for the n-th command under one
  label. This is the stable key settings use.
- owners: the map events (object, sign, trigger, map script) whose scripts reach it.

Reachability is worked out on the source: every argument that names a script label is
followed, and a script falls through into the next label until a terminating command.
giveitem with an item from a variable (giveitem VAR_0x8004) is skipped. The file is only
rewritten when its content changes.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

TERMINATORS = {"end", "return", "goto", "gotostd", "endram", "returnram", "gotonative"}
# Macro files that hold event script commands (the rest are battle, contest and sound scripts).
SCRIPT_MACRO_FILES = ["asm/macros/event.inc", "asm/macros/map.inc", "asm/macros/ce_encounters.inc",
                      "asm/macros/trainer_hill.inc", "asm/macros/trainer_tower.inc", "asm/macros/battle_tent.inc"]
SCRIPT_MACRO_GLOBS = ["asm/macros/battle_frontier/*.inc"]
OWNER_KINDS = ["CE_ITEM_OWNER_OBJECT", "CE_ITEM_OWNER_BG", "CE_ITEM_OWNER_COORD", "CE_ITEM_OWNER_MAP_SCRIPT"]
LABEL = re.compile(r"^([A-Za-z_][\w]*)(::?)")


def strip_comment(line: str) -> str:
    out, quoted = [], False
    for i, ch in enumerate(line):
        if ch == '"' and (i == 0 or line[i - 1] != "\\"):
            quoted = not quoted
        if ch == "@" and not quoted:
            break
        out.append(ch)
    return "".join(out)


def split_args(text: str) -> list[str]:
    args, depth, cur = [], 0, []
    for ch in text:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            args.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if "".join(cur).strip():
        args.append("".join(cur).strip())
    return args


class Stmt:
    __slots__ = ("file", "line", "name", "args", "labels", "cond")

    def __init__(self, file, line, name, args, cond):
        self.file, self.line, self.name, self.args, self.cond = file, line, name, args, cond
        self.labels: list[tuple[str, bool]] = []  # labels right before this statement (name, global)


def parse_file(path: Path, rel: str, stmts: list[Stmt]) -> None:
    pending: list[tuple[str, bool]] = []
    cond = 0
    for n, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = strip_comment(raw).strip()
        while line:
            m = LABEL.match(line)
            if m and line[m.end():m.end() + 1] != "=" and not line[m.end():].lstrip().startswith("="):
                pending.append((m.group(1), m.group(2) == "::"))
                line = line[m.end():].strip()
                continue
            break
        for part in (p.strip() for p in line.split(";")):
            if not part:
                continue
            name, _, rest = part.partition(" ") if " " in part else part.partition("\t")
            name = name.strip()
            if name in (".if", ".ifdef", ".ifndef", ".ifeq", ".ifne", ".ifgt", ".iflt", ".ifge", ".ifle"):
                cond += 1
                continue
            if name == ".endif":
                cond = max(0, cond - 1)
                continue
            if name in (".else", ".elseif"):
                continue
            s = Stmt(rel, n, name, split_args(rest.strip()), cond)
            s.labels, pending = pending, []
            stmts.append(s)
    if pending:
        s = Stmt(rel, 0, ".eof", [], 0)
        s.labels = pending
        stmts.append(s)


def script_macros(root: Path) -> set[str]:
    names = set()
    files = [root / f for f in SCRIPT_MACRO_FILES]
    for g in SCRIPT_MACRO_GLOBS:
        files += sorted(root.glob(g))
    for f in files:
        if f.exists():
            names |= set(re.findall(r"^\s*\.macro\s+(\w+)", f.read_text(encoding="utf-8"), re.M))
    return names


def included_scripts(root: Path) -> list[str]:
    text = (root / "data/event_scripts.s").read_text(encoding="utf-8")
    return [m for m in re.findall(r'^\s*\.include\s+"([^"]+)"', text, re.M)
            if m.startswith("data/") and m.endswith(".inc") and "/text/" not in m and m != "data/script_cmd_table.inc"]


def opcodes(root: Path) -> dict[str, int]:
    ops = {}
    for line in (root / "data/script_cmd_table.inc").read_text(encoding="utf-8").splitlines():
        m = re.match(r"\s*script_cmd_table_entry\s+(SCR_OP_\w+)", line)
        if m:
            ops[m.group(1)] = len(ops)
    return ops


def std_value(root: Path, name: str) -> int:
    m = re.search(rf"^\s*{name}\s*=\s*(\w+)", (root / "asm/macros/event.inc").read_text(encoding="utf-8"), re.M)
    if not m:
        sys.exit(f"gen_field_items: {name} not found in asm/macros/event.inc")
    return int(m.group(1), 0)


def hoenn_maps(root: Path) -> list[dict]:
    groups = json.loads((root / "data/maps/map_groups.json").read_text(encoding="utf-8"))
    maps = []
    for group in groups["group_order"]:
        for name in groups[group]:
            data = json.loads((root / f"data/maps/{name}/map.json").read_text(encoding="utf-8"))
            region = data.get("region", "REGION_KANTO" if name.endswith("_Frlg") else "REGION_HOENN")
            if region == "REGION_HOENN":
                maps.append(data)
    return maps


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out_path = Path(sys.argv[1])
    root = Path(".")
    macros = script_macros(root)
    ops = opcodes(root)

    stmts: list[Stmt] = []
    for rel in included_scripts(root):
        parse_file(root / rel, rel, stmts)
    label_at: dict[str, int] = {}
    is_global: dict[str, bool] = {}
    for i, s in enumerate(stmts):
        for name, glob in s.labels:
            label_at.setdefault(name, i)
            is_global[name] = glob

    # Walk one entry label: the statements it runs (through fall-through), and the labels it names.
    walk_cache: dict[str, tuple[list[int], set[str]]] = {}

    def walk(label: str) -> tuple[list[int], set[str]]:
        if label in walk_cache:
            return walk_cache[label]
        sites, refs = [], set()
        i = label_at[label]
        while i < len(stmts):
            s = stmts[i]
            if s.name not in macros:
                break
            if s.name in ("giveitem", "pokemart"):
                sites.append(i)
            refs |= {a for a in s.args if a in label_at}
            if s.name in TERMINATORS and s.cond == 0:
                break
            i += 1
        walk_cache[label] = (sites, refs)
        return sites, refs

    def reach(entry: str) -> set[int]:
        found, seen, todo = set(), set(), [entry]
        while todo:
            label = todo.pop()
            if label in seen or label not in label_at:
                continue
            seen.add(label)
            sites, refs = walk(label)
            found.update(sites)
            todo.extend(refs - seen)
        return found

    owners_of: dict[int, list[tuple[str, int, int]]] = {}
    for m in hoenn_maps(root):
        entries = [(0, i, e.get("script")) for i, e in enumerate(m.get("object_events", [])) if e.get("type", "object") == "object"]
        entries += [(1, i, e.get("script")) for i, e in enumerate(m.get("bg_events", [])) if e.get("type") == "sign"]
        entries += [(2, i, e.get("script")) for i, e in enumerate(m.get("coord_events", [])) if e.get("type") == "trigger"]
        entries.append((3, 0, f"{m.get('shared_scripts_map', m['name'])}_MapScripts"))
        for kind, index, script in entries:
            if not script or script == "0x0":
                continue
            for site in reach(script):
                lst = owners_of.setdefault(site, [])
                if (m["id"], kind, index) not in lst:
                    lst.append((m["id"], kind, index))

    def mart_items(target: str) -> list[str] | None:
        """The ITEM_ constants of a pokemart list (None if it isn't one)."""
        if target not in label_at:
            return None
        items = []
        for t in stmts[label_at[target]:]:
            if t.name == "pokemartlistend" or (t.name == ".2byte" and t.args == ["ITEM_NONE"]):
                return items
            if t.name == ".align" and not items:
                continue
            if t.name != ".2byte" or len(t.args) != 1 or not t.args[0].startswith("ITEM_"):
                return None
            items.append(t.args[0])
        return None

    # Describe each reachable command: its key, anchor, limit and the bytes to look for.
    sites = []
    skipped = []
    per_label: dict[str, int] = {}
    for i in sorted(owners_of):
        s = stmts[i]
        under = next((stmts[j].labels[-1] for j in range(i, -1, -1) if stmts[j].labels), None)
        anchor_i = next((j for j in range(i, -1, -1) if any(g for _, g in stmts[j].labels) and stmts[j].file == s.file), None)
        if not under or anchor_i is None:
            skipped.append(f"{s.file}:{s.line}: no label above")
            continue
        anchor = next(n for n, g in reversed(stmts[anchor_i].labels) if g)
        limit_i = next((j for j in range(i + 1, len(stmts)) if any(g for _, g in stmts[j].labels) and stmts[j].file == s.file), None)
        limit = next(n for n, g in stmts[limit_i].labels if g) if limit_i is not None and stmts[limit_i].name != ".eof" else None
        if limit == anchor:
            # The label is defined twice (in both branches of an .if): this copy isn't assembled.
            skipped.append(f"{s.file}:{s.line}: {s.name} under a label defined twice")
            continue
        if s.name == "giveitem":
            item = s.args[0] if s.args else ""
            amount = s.args[1] if len(s.args) > 1 else "1"
            if not item.startswith("ITEM_") or not re.fullmatch(r"\d+", amount):
                skipped.append(f"{s.file}:{s.line}: giveitem {', '.join(s.args)} (not a fixed item and amount)")
                continue
            entry = {"kind": "CE_ITEM_SITE_GIFT", "item": item, "amount": amount, "list": None}
        else:
            target = s.args[0] if s.args else ""
            items = mart_items(target)
            if not items:
                skipped.append(f"{s.file}:{s.line}: pokemart {target} (not a list of ITEM_ constants)")
                continue
            entry = {"kind": "CE_ITEM_SITE_MART", "item": "ITEM_NONE", "amount": "0", "list": items}
        # Earlier commands of the same kind between the anchor and this one (any operands:
        # the engine and the probe match only the bytes that edits never change).
        nth = sum(stmts[j].name == s.name for j in range(anchor_i, i))
        label = under[0]
        per_label[label] = per_label.get(label, 0) + 1
        key = label if per_label[label] == 1 else f"{label}#{per_label[label]}"
        sites.append({**entry, "key": key, "anchor": anchor, "limit": limit, "nth": nth, "owners": owners_of[i], "where": f"{s.file}:{s.line}"})

    write(out_path, render(sites, skipped, ops, std_value(root, "STD_OBTAIN_ITEM")))


def render(sites: list[dict], skipped: list[str], ops: dict[str, int], std_obtain_item: int) -> str:
    gifts = sum(s["kind"] == "CE_ITEM_SITE_GIFT" for s in sites)
    lines = [
        "// Generated by tools/ce/gen_field_items.py from the map scripts. Do not edit.",
        f"// {gifts} item gifts and {len(sites) - gifts} Poké Marts reachable from Hoenn maps; skipped:",
        *(f"//   {s}" for s in skipped),
        "",
        f"#define CE_ITEM_SCRIPT_OP_SETORCOPYVAR {ops['SCR_OP_SETORCOPYVAR']}",
        f"#define CE_ITEM_SCRIPT_OP_CALL_STD {ops['SCR_OP_CALL_STD']}",
        f"#define CE_ITEM_SCRIPT_OP_POKEMART {ops['SCR_OP_POKEMART']}",
        f"#define CE_ITEM_SCRIPT_STD_OBTAIN_ITEM {std_obtain_item}",
        "",
    ]
    labels = sorted({s["anchor"] for s in sites} | {s["limit"] for s in sites if s["limit"]})
    lines += [f"extern const u8 {l}[];" for l in labels]
    lines.append("")
    for n, s in enumerate(sites):
        if s["list"]:
            lines.append(f"static const u16 sCeMartItems{n}[] = {{ {', '.join(s['list'])}, ITEM_NONE }};")
    lines += ["", "static const struct CeItemScriptSite gCeItemScriptSites[] =", "{"]
    start = 0
    for n, s in enumerate(sites):
        lines.append(
            f"    {{ .anchor = {s['anchor']}, .limit = {s['limit'] or 'NULL'}, .key = \"{s['key']}\", "
            f".martItems = {f'sCeMartItems{n}' if s['list'] else 'NULL'}, .item = {s['item']}, .amount = {s['amount']}, "
            f".kind = {s['kind']}, .nth = {s['nth']}, .ownerStart = {start}, .ownerCount = {len(s['owners'])} }}, // {s['where']}"
        )
        start += len(s["owners"])
    lines += ["};", "", "static const struct CeItemScriptOwner gCeItemScriptOwners[] =", "{"]
    for s in sites:
        for m, kind, index in s["owners"]:
            lines.append(f"    {{ .map = {m}, .kind = {OWNER_KINDS[kind]}, .index = {index} }},")
    lines += ["};", ""]
    return "\n".join(lines)


def write(path: Path, text: str) -> None:
    if path.exists() and path.read_text(encoding="utf-8") == text:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
