#!/usr/bin/env python3
import argparse
from pathlib import Path


def emit_array(data: bytes, bytes_per_line: int = 16) -> str:
    lines = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i + bytes_per_line]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Embed a WAD binary as C source for QMK UF2 inclusion.")
    parser.add_argument("input", help="Input WAD file")
    parser.add_argument("output", help="Output C file")
    parser.add_argument("--max-bytes", type=int, default=0, help="Maximum bytes to embed (0 = full file)")
    args = parser.parse_args()

    src = Path(args.input)
    out = Path(args.output)

    raw = src.read_bytes()
    if args.max_bytes > 0:
        data = raw[: args.max_bytes]
    else:
        data = raw

    rendered = f"""#include <stddef.h>
#include <stdint.h>

const uint8_t doom_wad_data[] = {{
{emit_array(data)}
}};

const size_t doom_wad_size_bytes = sizeof(doom_wad_data);
"""

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(rendered)

    print(f"Embedded {len(data)} / {len(raw)} bytes from {src} -> {out}")


if __name__ == "__main__":
    main()
