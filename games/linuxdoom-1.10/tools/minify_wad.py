#!/usr/bin/env python3
import argparse
import dataclasses
import os
import struct
from typing import Dict, List, Tuple, Set

MAP_LUMP_SEQUENCE = [
    "THINGS",
    "LINEDEFS",
    "SIDEDEFS",
    "VERTEXES",
    "SEGS",
    "SSECTORS",
    "NODES",
    "SECTORS",
    "REJECT",
    "BLOCKMAP",
]

AGGRESSIVE_DROP_LUMPS = {
    "TITLEPIC",
    "CREDIT",
    "HELP1",
    "HELP2",
    "WIMAP0",
}

REQUIRED_GLOBAL_FLATS = {
    "F_SKY1",
    "FLOOR7_2",
}

E1M1_SPRITE_PREFIX_KEEP = {
    "PLAY", "POSS", "TROO",
    "PISG", "PISF", "PUNG",
    "PUFF", "BLUD",
    "CLIP", "STIM", "MEDI",
    "BKEY", "RKEY", "YKEY", "BSKU", "RSKU", "YSKU",
}

EXTREME_SPRITE_PREFIX_KEEP = {
    "PLAY", "POSS", "TROO",
    "PISG", "PISF", "PUNG",
    "PUFF", "BLUD",
    "CLIP", "STIM",
}

PLAYER_START_TYPES = {1, 2, 3, 4}
MONSTER_TYPES_KEEP = {3001, 3004}


@dataclasses.dataclass
class Lump:
    name: str
    data: bytes


@dataclasses.dataclass
class TexturePatchRef:
    origin_x: int
    origin_y: int
    patch_index: int
    stepdir: int
    colormap: int


@dataclasses.dataclass
class TextureDef:
    name: str
    masked: int
    width: int
    height: int
    columndirectory: int
    patches: List[TexturePatchRef]


def read_wad(path: str) -> Tuple[bytes, List[Lump]]:
    with open(path, "rb") as f:
        header = f.read(12)
        ident, count, directory_offset = struct.unpack("<4sii", header)
        f.seek(directory_offset)
        directory = [struct.unpack("<ii8s", f.read(16)) for _ in range(count)]
        lumps = []
        for offset, size, raw_name in directory:
            name = raw_name.rstrip(b"\0").decode("ascii", errors="ignore").upper()
            f.seek(offset)
            lumps.append(Lump(name=name, data=f.read(size)))
    return ident, lumps


def write_wad(path: str, ident: bytes, lumps: List[Lump]) -> None:
    with open(path, "wb") as f:
        f.write(ident)
        f.write(struct.pack("<ii", len(lumps), 0))

        offsets = []
        for lump in lumps:
            offsets.append(f.tell())
            f.write(lump.data)

        directory_offset = f.tell()
        for lump, offset in zip(lumps, offsets):
            name_bytes = lump.name.encode("ascii", errors="ignore")[:8]
            name_bytes = name_bytes + b"\0" * (8 - len(name_bytes))
            f.write(struct.pack("<ii8s", offset, len(lump.data), name_bytes))

        f.seek(4)
        f.write(struct.pack("<ii", len(lumps), directory_offset))


def parse_pnames(data: bytes) -> List[str]:
    count = struct.unpack_from("<i", data, 0)[0]
    names = []
    base = 4
    for i in range(count):
        raw = data[base + i * 8 : base + (i + 1) * 8]
        names.append(raw.rstrip(b"\0").decode("ascii", errors="ignore").upper())
    return names


def build_pnames(names: List[str]) -> bytes:
    out = bytearray()
    out += struct.pack("<i", len(names))
    for name in names:
        raw = name.upper().encode("ascii", errors="ignore")[:8]
        out += raw + b"\0" * (8 - len(raw))
    return bytes(out)


def parse_texture_lump(data: bytes) -> List[TextureDef]:
    num = struct.unpack_from("<i", data, 0)[0]
    offsets = [struct.unpack_from("<i", data, 4 + i * 4)[0] for i in range(num)]
    textures = []

    for off in offsets:
        raw_name, masked, width, height, coldir, patch_count = struct.unpack_from("<8sihhih", data, off)
        name = raw_name.rstrip(b"\0").decode("ascii", errors="ignore").upper()
        patches = []
        p_off = off + 22
        for _ in range(patch_count):
            origin_x, origin_y, patch_index, stepdir, colormap = struct.unpack_from("<hhhhh", data, p_off)
            p_off += 10
            patches.append(
                TexturePatchRef(
                    origin_x=origin_x,
                    origin_y=origin_y,
                    patch_index=patch_index,
                    stepdir=stepdir,
                    colormap=colormap,
                )
            )
        textures.append(
            TextureDef(
                name=name,
                masked=masked,
                width=width,
                height=height,
                columndirectory=coldir,
                patches=patches,
            )
        )

    return textures


def build_texture_lump(textures: List[TextureDef]) -> bytes:
    out = bytearray()
    out += struct.pack("<i", len(textures))
    offset_table_pos = len(out)
    out += b"\0" * (4 * len(textures))

    offsets = []
    for tex in textures:
        offsets.append(len(out))
        raw_name = tex.name.encode("ascii", errors="ignore")[:8]
        raw_name = raw_name + b"\0" * (8 - len(raw_name))
        out += struct.pack("<8sihhih", raw_name, tex.masked, tex.width, tex.height, tex.columndirectory, len(tex.patches))
        for p in tex.patches:
            out += struct.pack("<hhhhh", p.origin_x, p.origin_y, p.patch_index, p.stepdir, p.colormap)

    for i, off in enumerate(offsets):
        struct.pack_into("<i", out, offset_table_pos + i * 4, off)

    return bytes(out)


def find_map_indices(lumps: List[Lump]) -> Dict[str, int]:
    out = {}
    for i, lump in enumerate(lumps):
        n = lump.name
        if len(n) == 4 and n.startswith("E") and n[2] == "M" and n[1].isdigit() and n[3].isdigit():
            out[n] = i
        elif len(n) == 5 and n.startswith("MAP") and n[3:].isdigit():
            out[n] = i
    return out


def is_map_marker(name: str) -> bool:
    if len(name) == 4 and name.startswith("E") and name[2] == "M" and name[1].isdigit() and name[3].isdigit():
        return True
    if len(name) == 5 and name.startswith("MAP") and name[3:].isdigit():
        return True
    return False


def extract_map_texture_and_flats(lumps: List[Lump], map_index: int) -> Tuple[Set[str], Set[str]]:
    by_name = {lumps[map_index + 1 + i].name: lumps[map_index + 1 + i].data for i in range(10)}

    sidedefs = by_name.get("SIDEDEFS", b"")
    sectors = by_name.get("SECTORS", b"")

    textures = set()
    flats = set()

    for i in range(0, len(sidedefs), 30):
        if i + 30 > len(sidedefs):
            break
        upper = sidedefs[i + 4 : i + 12].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        lower = sidedefs[i + 12 : i + 20].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        middle = sidedefs[i + 20 : i + 28].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        for t in (upper, lower, middle):
            if t and t != "-":
                textures.add(t)

    for i in range(0, len(sectors), 26):
        if i + 26 > len(sectors):
            break
        floor = sectors[i + 4 : i + 12].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        ceil = sectors[i + 12 : i + 20].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        if floor:
            flats.add(floor)
        if ceil:
            flats.add(ceil)

    textures.add("SKY1")
    return textures, flats


def rewrite_things_minimal(data: bytes) -> bytes:
    out = bytearray()
    for i in range(0, len(data), 10):
        if i + 10 > len(data):
            break
        x, y, angle, thing_type, flags = struct.unpack_from("<hhhhh", data, i)

        if thing_type in PLAYER_START_TYPES:
            out += struct.pack("<hhhhh", x, y, angle, thing_type, flags)
            continue

        if thing_type == 9:
            thing_type = 3004

        if thing_type in MONSTER_TYPES_KEEP:
            out += struct.pack("<hhhhh", x, y, angle, thing_type, flags)

    return bytes(out)


def _wad_name_bytes(name: str) -> bytes:
    raw = name.upper().encode("ascii", errors="ignore")[:8]
    return raw + b"\0" * (8 - len(raw))


def rewrite_sidedefs_canonical(data: bytes, wall_texture: str) -> bytes:
    tex = _wad_name_bytes(wall_texture)
    out = bytearray(data)
    for i in range(0, len(out), 30):
        if i + 30 > len(out):
            break
        for off in (4, 12, 20):
            current = out[i + off : i + off + 8]
            if current.rstrip(b"\0") == b"-":
                continue
            out[i + off : i + off + 8] = tex
    return bytes(out)


def rewrite_sectors_canonical(data: bytes, floor_flat: str, ceil_flat: str) -> bytes:
    floor = _wad_name_bytes(floor_flat)
    ceil = _wad_name_bytes(ceil_flat)
    out = bytearray(data)
    for i in range(0, len(out), 26):
        if i + 26 > len(out):
            break
        out[i + 4 : i + 12] = floor
        out[i + 12 : i + 20] = ceil
    return bytes(out)


def rewrite_sectors_min_light(data: bytes, min_light: int) -> bytes:
    out = bytearray(data)
    for i in range(0, len(out), 26):
        if i + 26 > len(out):
            break
        current = struct.unpack_from("<h", out, i + 20)[0]
        if current < min_light:
            struct.pack_into("<h", out, i + 20, min_light)
    return bytes(out)


def detect_first_map_surfaces(lumps: List[Lump], map_index: int) -> Tuple[str, str, str]:
    by_name = {lumps[map_index + 1 + i].name: lumps[map_index + 1 + i].data for i in range(10)}
    sidedefs = by_name.get("SIDEDEFS", b"")
    sectors = by_name.get("SECTORS", b"")

    wall = ""
    for i in range(0, len(sidedefs), 30):
        if i + 30 > len(sidedefs):
            break
        for off in (20, 4, 12):
            tex = sidedefs[i + off : i + off + 8].rstrip(b"\0").decode("ascii", errors="ignore").upper()
            if tex and tex != "-":
                wall = tex
                break
        if wall:
            break

    floor = ""
    ceil = ""
    if len(sectors) >= 26:
        floor = sectors[4:12].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        ceil = sectors[12:20].rstrip(b"\0").decode("ascii", errors="ignore").upper()

    return wall, floor, ceil


def _looks_like_patch(data: bytes) -> bool:
    if len(data) < 8:
        return False
    width, height, _, _ = struct.unpack_from("<hhhh", data, 0)
    if width <= 0 or height <= 0 or width > 2048 or height > 2048:
        return False
    if len(data) < 8 + width * 4:
        return False
    first_col_off = struct.unpack_from("<i", data, 8)[0]
    return 0 <= first_col_off < len(data)


def build_degraded_patch_like(data: bytes) -> bytes:
    if not _looks_like_patch(data):
        return data

    width, height, left_off, top_off = struct.unpack_from("<hhhh", data, 0)
    if width <= 0 or height <= 0:
        return data

    out = bytearray()
    out += struct.pack("<hhhh", width, height, left_off, top_off)

    ofs_pos = len(out)
    out += b"\0" * (width * 4)

    offsets: List[int] = []
    for _ in range(width):
        offsets.append(len(out))
        out += bytes((0, 1, 0, 0, 0, 0xFF))

    for i, off in enumerate(offsets):
        struct.pack_into("<i", out, ofs_pos + i * 4, off)

    return bytes(out)


def main() -> None:
    parser = argparse.ArgumentParser(description="Create an aggressively minimized IWAD for E1M1-only, no-sound builds.")
    parser.add_argument("input_wad", nargs="?", default="linux/doom1.wad.bak")
    parser.add_argument("output_wad", nargs="?", default="linux/doom1.wad")
    parser.add_argument("--map", default="E1M1", help="Map marker to keep (default: E1M1)")
    parser.add_argument("--keep-demos", action="store_true")
    parser.add_argument("--keep-music", action="store_true")
    parser.add_argument("--keep-sfx", action="store_true")
    parser.add_argument("--aggressive-ui-strip", action="store_true", help="Drop non-essential title/help/intermission art")
    parser.add_argument("--strip-sprites-e1m1", action="store_true", help="Keep only a curated E1M1 gameplay sprite subset")
    parser.add_argument("--strip-menu-assets", action="store_true", help="Drop M_* menu/title patches")
    parser.add_argument("--strip-midi-assets", action="store_true", help="Drop GENMIDI/DMXGUS lumps")
    parser.add_argument("--strip-endscreen", action="store_true", help="Drop ENDOOM end screen lump")
    parser.add_argument("--canonical-wall-texture", default="", help="Rewrite all non-empty SIDEDEFS wall textures to this texture name")
    parser.add_argument("--canonical-floor-flat", default="", help="Rewrite all SECTORS floor flats to this flat name")
    parser.add_argument("--canonical-ceil-flat", default="", help="Rewrite all SECTORS ceiling flats to this flat name")
    parser.add_argument("--degrade-graphics", action="store_true", help="Replace kept patch/sprite graphics with tiny placeholder content")
    parser.add_argument("--bare-essentials", action="store_true", help="Extreme one-map minimization with canonical surfaces and reduced sprite set")
    parser.add_argument("--minimal-things", action="store_true", help="Reduce THINGS to minimal gameplay subset")
    parser.add_argument("--min-sector-light", type=int, default=-1, help="Set a minimum sector light level (0-255) on the kept map")
    args = parser.parse_args()

    args.map = args.map.upper()

    if args.min_sector_light < -1 or args.min_sector_light > 255:
        raise SystemExit("--min-sector-light must be -1 or within 0..255")

    ident, lumps = read_wad(args.input_wad)
    if ident != b"IWAD":
        raise SystemExit("Input must be an IWAD")

    map_indices = find_map_indices(lumps)
    if args.map not in map_indices:
        raise SystemExit(f"Map {args.map} not found")

    keep_map_idx = map_indices[args.map]

    canonical_wall = args.canonical_wall_texture.upper() if args.canonical_wall_texture else ""
    canonical_floor = args.canonical_floor_flat.upper() if args.canonical_floor_flat else ""
    canonical_ceil = args.canonical_ceil_flat.upper() if args.canonical_ceil_flat else ""

    if args.bare_essentials:
        args.aggressive_ui_strip = True
        args.strip_sprites_e1m1 = True
        args.strip_menu_assets = True
        args.strip_midi_assets = True
        args.strip_endscreen = True
        args.keep_sfx = False
        args.keep_music = False
        args.keep_demos = False
        args.degrade_graphics = True
        args.minimal_things = True

        auto_wall, auto_floor, auto_ceil = detect_first_map_surfaces(lumps, keep_map_idx)
        if not canonical_wall:
            canonical_wall = auto_wall
        if not canonical_floor:
            canonical_floor = auto_floor
        if not canonical_ceil:
            canonical_ceil = auto_ceil

    if bool(canonical_floor) != bool(canonical_ceil):
        raise SystemExit("--canonical-floor-flat and --canonical-ceil-flat must be provided together")

    if canonical_wall:
        used_textures = {canonical_wall, "SKY1"}
    else:
        used_textures, _ = extract_map_texture_and_flats(lumps, keep_map_idx)

    if canonical_floor and canonical_ceil:
        used_flats = {canonical_floor, canonical_ceil}
    else:
        _, used_flats = extract_map_texture_and_flats(lumps, keep_map_idx)

    used_flats.update(REQUIRED_GLOBAL_FLATS)

    # Parse and trim TEXTURE1 + PNAMES to only textures needed by kept map.
    lump_index_by_name = {l.name: i for i, l in enumerate(lumps)}
    if "TEXTURE1" not in lump_index_by_name or "PNAMES" not in lump_index_by_name:
        raise SystemExit("TEXTURE1/PNAMES missing")

    textures = parse_texture_lump(lumps[lump_index_by_name["TEXTURE1"]].data)
    pnames = parse_pnames(lumps[lump_index_by_name["PNAMES"]].data)

    texture_names = {t.name for t in textures}
    if canonical_wall and canonical_wall not in texture_names:
        raise SystemExit(f"Canonical wall texture not found: {canonical_wall}")

    flat_names = set()
    in_flats_scan = False
    for lump in lumps:
        if lump.name == "F_START":
            in_flats_scan = True
            continue
        if lump.name == "F_END":
            break
        if in_flats_scan:
            flat_names.add(lump.name)

    if canonical_floor and canonical_floor not in flat_names:
        raise SystemExit(f"Canonical floor flat not found: {canonical_floor}")
    if canonical_ceil and canonical_ceil not in flat_names:
        raise SystemExit(f"Canonical ceil flat not found: {canonical_ceil}")

    kept_textures = [t for t in textures if t.name in used_textures]

    used_patch_indices: Set[int] = set()
    for tex in kept_textures:
        for p in tex.patches:
            if 0 <= p.patch_index < len(pnames):
                used_patch_indices.add(p.patch_index)

    old_to_new_patch: Dict[int, int] = {}
    new_pnames: List[str] = []
    for old_idx in sorted(used_patch_indices):
        old_to_new_patch[old_idx] = len(new_pnames)
        new_pnames.append(pnames[old_idx])

    for tex in kept_textures:
        for p in tex.patches:
            p.patch_index = old_to_new_patch[p.patch_index]

    new_texture1 = build_texture_lump(kept_textures)
    new_pnames_data = build_pnames(new_pnames)

    map_lump_keep = {args.map, *MAP_LUMP_SEQUENCE}

    # Determine flat names to keep between F_START/F_END markers.
    in_flats = False
    flat_keep_names = set(used_flats)

    # Determine texture patch lumps to keep between P_START/P_END markers.
    patch_keep_names = set(new_pnames)
    in_patches = False
    in_sprites = False

    out_lumps: List[Lump] = []
    for i, lump in enumerate(lumps):
        n = lump.name

        if args.minimal_things and i == keep_map_idx + 1 and n == "THINGS":
            out_lumps.append(Lump(n, rewrite_things_minimal(lump.data)))
            continue
        if canonical_wall and i > keep_map_idx and i <= keep_map_idx + 10 and n == "SIDEDEFS":
            out_lumps.append(Lump(n, rewrite_sidedefs_canonical(lump.data, canonical_wall)))
            continue
        if i > keep_map_idx and i <= keep_map_idx + 10 and n == "SECTORS":
            sector_data = lump.data
            if canonical_floor and canonical_ceil:
                sector_data = rewrite_sectors_canonical(sector_data, canonical_floor, canonical_ceil)
            if args.min_sector_light >= 0:
                sector_data = rewrite_sectors_min_light(sector_data, args.min_sector_light)
            out_lumps.append(Lump(n, sector_data))
            continue

        if n == "TEXTURE1":
            out_lumps.append(Lump(n, new_texture1))
            continue
        if n == "PNAMES":
            out_lumps.append(Lump(n, new_pnames_data))
            continue

        if is_map_marker(n):
            if n != args.map:
                continue
            out_lumps.append(lump)
            continue

        # Drop map payload lumps for non-kept maps; keep only payload after kept map marker.
        if i > 0 and is_map_marker(lumps[i - 1].name) and lumps[i - 1].name != args.map and n in MAP_LUMP_SEQUENCE:
            continue

        if n in MAP_LUMP_SEQUENCE:
            if n in map_lump_keep and i > keep_map_idx and i <= keep_map_idx + 10:
                out_lumps.append(lump)
            elif i <= keep_map_idx or i > keep_map_idx + 10:
                continue
            continue

        if n == "P_START":
            in_patches = True
            out_lumps.append(lump)
            continue
        if n == "P_END":
            in_patches = False
            out_lumps.append(lump)
            continue

        if n == "F_START":
            in_flats = True
            out_lumps.append(lump)
            continue
        if n == "F_END":
            in_flats = False
            out_lumps.append(lump)
            continue

        if in_patches:
            if n in patch_keep_names:
                out_lumps.append(lump)
            continue

        if n == "S_START":
            in_sprites = True
            out_lumps.append(lump)
            continue
        if n == "S_END":
            in_sprites = False
            out_lumps.append(lump)
            continue

        if in_sprites:
            if not args.strip_sprites_e1m1:
                out_lumps.append(lump)
            else:
                prefix = n[:4]
                keep_set = EXTREME_SPRITE_PREFIX_KEEP if args.bare_essentials else E1M1_SPRITE_PREFIX_KEEP
                if prefix in keep_set:
                    if args.degrade_graphics:
                        out_lumps.append(Lump(n, build_degraded_patch_like(lump.data)))
                    else:
                        out_lumps.append(lump)
            continue

        if in_flats:
            if n in flat_keep_names:
                out_lumps.append(lump)
            continue

        if args.aggressive_ui_strip:
            if n in AGGRESSIVE_DROP_LUMPS:
                continue
            if n.startswith("WI"):
                continue

        if args.strip_menu_assets and n.startswith("M_"):
            continue

        if args.strip_midi_assets and (n == "GENMIDI" or n == "DMXGUS"):
            continue

        if args.strip_endscreen and n == "ENDOOM":
            continue

        if not args.keep_sfx and (n.startswith("DS") or n.startswith("DP")):
            continue
        if not args.keep_music and n.startswith("D_"):
            continue
        if not args.keep_demos and n.startswith("DEMO"):
            continue

        out_lumps.append(lump)

    os.makedirs(os.path.dirname(args.output_wad), exist_ok=True)
    write_wad(args.output_wad, ident, out_lumps)

    in_size = os.path.getsize(args.input_wad)
    out_size = os.path.getsize(args.output_wad)
    print(f"Input : {args.input_wad} ({in_size} bytes)")
    print(f"Output: {args.output_wad} ({out_size} bytes)")
    print(f"Saved : {in_size - out_size} bytes ({(in_size - out_size) * 100.0 / in_size:.2f}%)")
    print(f"Kept textures: {len(kept_textures)} / {len(textures)}")
    print(f"Kept patches : {len(new_pnames)} / {len(pnames)}")
    print(f"Kept flats   : {len(flat_keep_names)}")


if __name__ == "__main__":
    main()
