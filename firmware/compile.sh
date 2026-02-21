#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOOM_DIR="${SCRIPT_DIR}/../games/linuxdoom-1.10"
WAD_EMBED_SCRIPT="${SCRIPT_DIR}/doom/embed_wad.py"
WAD_INPUT="${DOOM_DIR}/linux/doom1.wad"
WAD_C_OUTPUT="${SCRIPT_DIR}/doom/doom_wad_data.c"

INPUT_DIR="/home/noah/qmk_firmware/.build/"
OUTPUT_DIR="/mnt/c/Users/Noah/Desktop/Files/Programmioong/Hardware Design/hackpad-attemps/macropad-v2/firmware"
LOCAL_OUT_DIR="/home/noah/qmk_firmware/keyboards/noahshpad/firmware/build"

if [ -d "${DOOM_DIR}" ]; then
	if [ -n "${TINYWAD_FLAGS:-}" ]; then
		make -C "${DOOM_DIR}" tinywad TINYWAD_FLAGS="${TINYWAD_FLAGS}"
	else
		make -C "${DOOM_DIR}" tinywad
	fi
	python3 "${WAD_EMBED_SCRIPT}" "${WAD_INPUT}" "${WAD_C_OUTPUT}"
fi

qmk compile -kb noahshpad/firmware -km NoahSheppard

if [ ! -d "$OUTPUT_DIR" ]; then
	echo "Output directory does not exist: $OUTPUT_DIR"
	exit 1
fi

cp -r "${INPUT_DIR}"* "$OUTPUT_DIR/"
cp -r "${INPUT_DIR}"* "$LOCAL_OUT_DIR/"
rm -rf "${INPUT_DIR}"*