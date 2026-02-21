#!/usr/bin/env python3
import pathlib
import re
import struct
from collections import defaultdict

ROOT = pathlib.Path(__file__).resolve().parents[1]
WAD = ROOT / "linux" / "doom1.wad"
OUT = ROOT / "linux" / "wad_requirements_strict.md"

LOOKUP_RE = re.compile(r"W_(?:CacheLumpName|GetNumForName)\s*\(\s*\"([A-Za-z0-9_]+)\"")


def read_wad(path: pathlib.Path):
    with path.open("rb") as f:
        ident, count, directory_offset = struct.unpack("<4sii", f.read(12))
        f.seek(directory_offset)
        directory = [struct.unpack("<ii8s", f.read(16)) for _ in range(count)]

    lumps = []
    with path.open("rb") as f:
        for offset, size, raw_name in directory:
            name = raw_name.rstrip(b"\0").decode("ascii", errors="ignore").upper()
            f.seek(offset)
            lumps.append((name, size, f.read(size)))

    return ident, lumps


def scan_code_literals(root: pathlib.Path):
    literal_to_files = defaultdict(set)
    for cfile in root.glob("*.c"):
        text = cfile.read_text(errors="ignore")
        for match in LOOKUP_RE.finditer(text):
            literal_to_files[match.group(1).upper()].add(cfile.name)
    return literal_to_files


def map_usage(lumps):
    idx = {name: i for i, (name, _, _) in enumerate(lumps)}
    used_textures = set()
    used_flats = set()

    if "E1M1" not in idx:
        return used_textures, used_flats

    i = idx["E1M1"]
    by_name = {lumps[i + 1 + k][0]: lumps[i + 1 + k][2] for k in range(10)}

    sidedefs = by_name.get("SIDEDEFS", b"")
    sectors = by_name.get("SECTORS", b"")

    for off in range(0, len(sidedefs), 30):
        if off + 30 > len(sidedefs):
            break
        for tex_off in (4, 12, 20):
            tex = sidedefs[off + tex_off : off + tex_off + 8].rstrip(b"\0").decode("ascii", errors="ignore").upper()
            if tex and tex != "-":
                used_textures.add(tex)

    for off in range(0, len(sectors), 26):
        if off + 26 > len(sectors):
            break
        floor = sectors[off + 4 : off + 12].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        ceil = sectors[off + 12 : off + 20].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        if floor:
            used_flats.add(floor)
        if ceil:
            used_flats.add(ceil)

    return used_textures, used_flats


def texture_patch_usage(lumps):
    idx = {name: i for i, (name, _, _) in enumerate(lumps)}
    if "TEXTURE1" not in idx or "PNAMES" not in idx:
        return set(), set(), set()

    pnames_data = lumps[idx["PNAMES"]][2]
    pcount = struct.unpack_from("<i", pnames_data, 0)[0]
    pnames = []
    for i in range(pcount):
        raw = pnames_data[4 + i * 8 : 4 + (i + 1) * 8]
        pnames.append(raw.rstrip(b"\0").decode("ascii", errors="ignore").upper())

    texture_data = lumps[idx["TEXTURE1"]][2]
    tcount = struct.unpack_from("<i", texture_data, 0)[0]
    offsets = [struct.unpack_from("<i", texture_data, 4 + i * 4)[0] for i in range(tcount)]

    textures = set()
    used_patches = set()

    for off in offsets:
        name = texture_data[off : off + 8].rstrip(b"\0").decode("ascii", errors="ignore").upper()
        textures.add(name)
        patch_count = struct.unpack_from("<h", texture_data, off + 20)[0]
        patch_off = off + 22
        for _ in range(patch_count):
            _, _, patch_index, _, _ = struct.unpack_from("<hhhhh", texture_data, patch_off)
            patch_off += 10
            if 0 <= patch_index < len(pnames):
                used_patches.add(pnames[patch_index])

    return textures, set(pnames), used_patches


def main():
    ident, lumps = read_wad(WAD)
    if ident != b"IWAD":
        raise SystemExit("Expected IWAD")

    lump_names = {name for name, _, _ in lumps}
    literals = scan_code_literals(ROOT)
    used_textures, used_flats = map_usage(lumps)
    textures_all, pnames_all, used_patches = texture_patch_usage(lumps)

    map_lumps = {
        "E1M1",
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
    }

    strict_needed = set()
    strict_needed.update(map_lumps)
    strict_needed.update({"PLAYPAL", "COLORMAP", "TEXTURE1", "PNAMES", "F_SKY1", "FLOOR7_2", "STBAR"})
    strict_needed.update(used_textures)
    strict_needed.update(used_flats)
    strict_needed.update(used_patches)
    strict_needed.update(name for name in literals if name in lump_names)

    present_code_literals = sorted(name for name in literals if name in lump_names)
    missing_code_literals = sorted(name for name in literals if name not in lump_names)
    strict_present = sorted(name for name in strict_needed if name in lump_names)
    candidates = sorted(name for name in lump_names if name not in strict_needed)

    size_by_name = {name: size for name, size, _ in lumps}

    with OUT.open("w") as f:
        f.write("# Strict WAD Requirements Scan\n\n")
        f.write(f"- WAD: {WAD.relative_to(ROOT)}\n")
        f.write(f"- Total lumps: {len(lumps)}\n")
        f.write(f"- Code literal lookups found: {len(literals)}\n")
        f.write(f"- Strict-needed present lumps: {len(strict_present)}\n")
        f.write(f"- Candidate removable/review lumps: {len(candidates)}\n\n")

        f.write("## Present code-literal lookups\n")
        for name in present_code_literals:
            files = ", ".join(sorted(literals[name]))
            f.write(f"- {name} ({size_by_name[name]} bytes) <- {files}\n")

        f.write("\n## Missing code-literal lookups\n")
        for name in missing_code_literals:
            files = ", ".join(sorted(literals[name]))
            f.write(f"- {name} <- {files}\n")

        f.write("\n## E1M1-derived usage\n")
        f.write(f"- Used map textures: {sorted(used_textures)}\n")
        f.write(f"- Used map flats: {sorted(used_flats)}\n")
        f.write(f"- TEXTURE1 entries: {len(textures_all)}\n")
        f.write(f"- PNAMES entries: {len(pnames_all)}\n")
        f.write(f"- Used patch lumps via textures: {len(used_patches)}\n")

        f.write("\n## Strict-needed lumps in current WAD\n")
        for name in strict_present:
            f.write(f"- {name}: {size_by_name[name]}\n")

        f.write("\n## Candidate review/removal list\n")
        for name in sorted(candidates, key=lambda n: size_by_name[n], reverse=True):
            f.write(f"- {name}: {size_by_name[name]}\n")


if __name__ == "__main__":
    main()
