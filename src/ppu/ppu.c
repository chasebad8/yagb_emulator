/**
 * @file ppu.c
 * @brief Game Boy PPU emulation implementation
 *
 * Implements the picture processing unit (PPU) for the YAGB Game Boy emulator.
 * Contains functions for managing VRAM and OAM, as well as PPU state management.
 *
 * @author Chase Badalato
 * @date 2026-06-18
 */

#include "ppu/ppu.h"
#include "common/logging.h"

/*
   The CPU builds and updates VRAM, while the PPU reads VRAM every frame and converts it into pixels on the LCD.
   During parts of that process (Mode 3), the CPU must stay out of VRAM so the PPU can read it without interference.

   every frame has 154 scanlines, each scanline takes 456 cycles to render
   visible scanlines are 0-143 (160 pixels wide 144 pixels tall), 144-153 are vblank (ie. not visible)
   456*154 = 70224 t-cycles per frame, 70224/60 = 1170.4 t-cycles per ms

   For every visible scanline, the PPU goes through 3 modes:
   - Mode 2: OAM search (80 t-cycles)
      - scan OAM for sprites that will be rendered on the current scanline
      - max 10 sprites can be rendered on a scanline, if more than 10 are found, the rest are ignored
      - if a sprite is found, its data is fetched from OAM and stored in a buffer for rendering in mode 3
      - cannot access OAM but can access VRAM

   - Mode 3: Pixel transfer (172 t-cycles)
      - CPU cannot access OAM or VRAM during this mode
      - read tile data from vram
      - read background and tile maps
      - read sprite data from OAM buffer
      - render pixels to the screen

   - Mode 0: HBlank (204 t-cycles)
      - after pixel transfer is complete, the PPU enters HBlank until the next scanline starts
      - CPU can access OAM and VRAM during this mode

   - Mode 1: VBlank (10*456 t-cycles)
      - after scanline 143 is rendered, the PPU enters VBlank until the next frame starts
      - CPU can access OAM and VRAM during this mode
      - PPU sends vblank interrupt to the CPU at the start of this mode

*/

void ppu_init(ppu_t *ppu_p)
{
   LOG_DEBUG("initializing ppu ...");
   LOG_DEBUG("ppu init success!");
}

void ppu_vram_write(ppu_t *ppu_p, uint16_t addr, uint8_t value)
{
   ppu_p->vram[addr] = value;
}

uint8_t ppu_vram_read(ppu_t *ppu_p, uint16_t addr)
{
   return ppu_p->vram[addr];
}

void ppu_oam_write(ppu_t *ppu_p, uint16_t addr, uint8_t value)
{
   ppu_p->oam[addr] = value;
}

uint8_t ppu_oam_read(ppu_t *ppu_p, uint16_t addr)
{
   return ppu_p->oam[addr];
}
