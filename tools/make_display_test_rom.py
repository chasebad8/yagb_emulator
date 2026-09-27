#!/usr/bin/env python3
"""Generate a 32 KiB ROM-only DMG background rendering test ROM."""

from pathlib import Path


ROM_SIZE = 0x8000
SCREEN_WIDTH = 160
SCREEN_HEIGHT = 144
MAP_WIDTH = 32
MAP_HEIGHT = 32
CODE_START = 0x0150


def set_rect(image, x, y, width, height, color):
    for row in range(y, y + height):
        image[row][x:x + width] = [color] * width


def draw_text(image, text, x, y, color, scale=3):
    glyphs = {
        "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
        "B": ("11110", "10001", "10001", "11110", "10001", "10001", "11110"),
        "G": ("01110", "10001", "10000", "10111", "10001", "10001", "01110"),
        "Y": ("10001", "10001", "01010", "00100", "00100", "00100", "00100"),
    }

    cursor_x = x
    for character in text:
        for glyph_y, line in enumerate(glyphs[character]):
            for glyph_x, bit in enumerate(line):
                if bit == "1":
                    set_rect(image,
                             cursor_x + glyph_x * scale,
                             y + glyph_y * scale,
                             scale,
                             scale,
                             color)
        cursor_x += 6 * scale


def make_image():
    image = [[0] * SCREEN_WIDTH for _ in range(SCREEN_HEIGHT)]

    set_rect(image, 4, 4, 152, 136, 3)
    set_rect(image, 8, 8, 144, 128, 0)

    for shade in range(4):
        set_rect(image, 16 + shade * 32, 16, 32, 40, shade)

    set_rect(image, 16, 60, 128, 36, 1)
    text_width = 4 * 5 * 3 + 3 * 3
    draw_text(image, "YAGB", (SCREEN_WIDTH - text_width) // 2, 66, 3)

    for tile_y in range(4):
        for tile_x in range(16):
            shade = (tile_x + tile_y) % 4
            set_rect(image, 16 + tile_x * 8, 104 + tile_y * 8, 8, 8, shade)

    return image


def encode_tiles(image):
    tile_ids = {}
    tile_data = bytearray()
    tile_map = [0] * (MAP_WIDTH * MAP_HEIGHT)

    for tile_y in range(SCREEN_HEIGHT // 8):
        for tile_x in range(SCREEN_WIDTH // 8):
            pixels = tuple(
                tuple(image[tile_y * 8 + row][tile_x * 8:tile_x * 8 + 8])
                for row in range(8)
            )
            tile_id = tile_ids.get(pixels)
            if tile_id is None:
                tile_id = len(tile_ids)
                tile_ids[pixels] = tile_id
                for row in pixels:
                    low_plane = 0
                    high_plane = 0
                    for pixel in row:
                        low_plane = (low_plane << 1) | (pixel & 1)
                        high_plane = (high_plane << 1) | ((pixel >> 1) & 1)
                    tile_data.extend((low_plane, high_plane))

            tile_map[tile_y * MAP_WIDTH + tile_x] = tile_id

    if len(tile_ids) > 255:
        raise ValueError(f"image uses {len(tile_ids)} tiles; maximum supported is 255")

    return tile_data, tile_map, len(tile_ids)


def emit_write_bytes(code, address, values):
    code.extend((0x21, address & 0xFF, address >> 8))  # LD HL, address
    for value in values:
        code.extend((0x3E, value, 0x22))  # LD A, value; LD (HL+), A


def make_rom():
    rom = bytearray(ROM_SIZE)
    rom[0x0100:0x0103] = bytes((0xC3, CODE_START & 0xFF, CODE_START >> 8))

    title = b"YAGB-DISPLAY"
    rom[0x0134:0x0143] = title.ljust(15, b" ")
    rom[0x0143] = 0x00
    rom[0x0147] = 0x00  # ROM-only cartridge
    rom[0x0148] = 0x00  # 32 KiB ROM
    rom[0x0149] = 0x00  # No cartridge RAM

    image = make_image()
    tile_data, tile_map, tile_count = encode_tiles(image)

    code = bytearray((
        0xF3,                    # DI
        0x31, 0xFE, 0xFF,        # LD SP, $FFFE
        0xAF,                    # XOR A
        0xEA, 0x40, 0xFF,        # LCDC = 0 (LCD off while VRAM is initialized)
        0xEA, 0x42, 0xFF,        # SCY = 0
        0xEA, 0x43, 0xFF,        # SCX = 0
        0xEA, 0x44, 0xFF,        # LY = 0
        0xEA, 0x45, 0xFF,        # LYC = 0
        0xEA, 0x4A, 0xFF,        # WY = 0
        0xEA, 0x4B, 0xFF,        # WX = 0
        0xEA, 0x41, 0xFF,        # STAT = 0 (no mode interrupts enabled)
        0xEA, 0xFF, 0xFF,        # IE = 0
    ))
    emit_write_bytes(code, 0x8000, tile_data)
    emit_write_bytes(code, 0x9800, tile_map)
    code.extend((
        0x3E, 0xE4,              # LD A, $E4 (identity BGP mapping on a real DMG)
        0xEA, 0x47, 0xFF,        # BGP = $E4
        0x3E, 0x91,              # LCD on, BG on, unsigned tile data, map at $9800
        0xEA, 0x40, 0xFF,        # LCDC = $91
        0x18, 0xFE,              # JR to itself
    ))

    if CODE_START + len(code) > ROM_SIZE:
        raise ValueError("generated program does not fit in the 32 KiB ROM")
    rom[CODE_START:CODE_START + len(code)] = code

    header_checksum = 0
    for value in rom[0x0134:0x014D]:
        header_checksum = (header_checksum - value - 1) & 0xFF
    rom[0x014D] = header_checksum

    global_checksum = sum(rom[:0x014E]) + sum(rom[0x0150:])
    rom[0x014E] = (global_checksum >> 8) & 0xFF
    rom[0x014F] = global_checksum & 0xFF

    return rom, tile_count, len(code)


def main():
    output = Path(__file__).with_name("display_test.gb")
    rom, tile_count, code_size = make_rom()
    output.write_bytes(rom)
    print(f"Wrote {output} ({len(rom)} bytes, {tile_count} unique tiles, {code_size} code bytes)")


if __name__ == "__main__":
    main()