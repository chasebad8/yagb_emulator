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

#define PPU_NUM_SCANLINES         (154)
#define PPU_NUM_VISIBLE_SCANLINES (144)
#define PPU_CYCLES_PER_SCANLINE   (456)
#define PPU_CYCLES_PER_FRAME      (PPU_CYCLES_PER_SCANLINE * PPU_NUM_VISIBLE_SCANLINES)
#define PPU_ELAPSED_CYCLES_PER_SCANLINE(cycles) ((cycles) % PPU_CYCLES_PER_SCANLINE)

#define PPU_MODE_0_CYCLES (204)
#define PPU_MODE_1_CYCLES ((PPU_NUM_SCANLINES - PPU_NUM_VISIBLE_SCANLINES) * PPU_CYCLES_PER_SCANLINE)
#define PPU_MODE_2_CYCLES (80)
#define PPU_MODE_3_CYCLES (172)

#define PPU_MODE_0_OFFSET (PPU_MODE_3_OFFSET + PPU_MODE_3_CYCLES)
#define PPU_MODE_1_OFFSET (PPU_MODE_0_OFFSET + PPU_MODE_0_CYCLES)
#define PPU_MODE_2_OFFSET (0)
#define PPU_MODE_3_OFFSET (PPU_MODE_2_OFFSET + PPU_MODE_2_CYCLES)

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

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_0_hblank(ppu_t *ppu)
{

}

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_1_vblank(ppu_t *ppu)
{

}

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_2_oam_query(ppu_t *ppu)
{

}

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_3_pixel_transfer(ppu_t *ppu)
{

}

void ppu_init(ppu_t *ppu_p)
{
   LOG_DEBUG("initializing ppu ...");
   LOG_DEBUG("ppu init success!");
}

/**
 * @brief state machine moved from mode 2, 3, 0 for 144
          scanlines before entering mode 1 for 10 scanlines
 *
 * @param ppu
 * @return statc
 */
static void ppu_update_state_machine(ppu_t *ppu)
{
   ppu->frame_count += ppu->tick_count / PPU_CYCLES_PER_FRAME;
   ppu->tick_count  = (ppu->tick_count + 1) % PPU_CYCLES_PER_FRAME;

   if (ppu->tick_count >= PPU_MODE_1_OFFSET)
   {
      ppu->state = STATE_1_VBLANK;
   }
   else if (PPU_ELAPSED_CYCLES_PER_SCANLINE(ppu->tick_count) >= PPU_MODE_0_OFFSET)
   {
      ppu->state = STATE_0_HBLANK;
   }
   else if (PPU_ELAPSED_CYCLES_PER_SCANLINE(ppu->tick_count) >= PPU_MODE_2_OFFSET)
   {
      ppu->state = STATE_2_OAM_QUERY;
   }
   else if (PPU_ELAPSED_CYCLES_PER_SCANLINE(ppu->tick_count) >= PPU_MODE_3_OFFSET)
   {
      ppu->state = STATE_3_PIXEL_TRANSFER;
   }
}

/**
 * @brief increment ppu by however
          many t-cycles have passed during
          cpu processing. handle 1 tick at
          a time.
 *
 * @param ppu_p
 * @param num_ticks
 */
void ppu_tick(ppu_t *ppu, uint8_t num_ticks)
{
   /* ppu operates 1 tick at a time */
   uint8_t consumed_ticks = num_ticks;

   while(consumed_ticks--)
   {
      ppu_update_state_machine(ppu);

      switch(ppu->state)
      {
         case STATE_0_HBLANK:
            ppu_mode_0_hblank(ppu);
            break;

         case STATE_1_VBLANK:
            ppu_mode_1_vblank(ppu);
            break;

         case STATE_2_OAM_QUERY:
            ppu_mode_2_oam_query(ppu);
            break;

         case STATE_3_PIXEL_TRANSFER:
            ppu_mode_3_pixel_transfer(ppu);
            break;
      }
   }
}

/**
 * @brief
 *
 * @param ppu_p
 * @param addr
 * @param value
 */
void ppu_vram_write(ppu_t *ppu_p, uint16_t addr, uint8_t value)
{
   ppu_p->vram[addr] = value;
}

/**
 * @brief
 *
 * @param ppu_p
 * @param addr
 * @return uint8_t
 */
uint8_t ppu_vram_read(ppu_t *ppu_p, uint16_t addr)
{
   return ppu_p->vram[addr];
}

/**
 * @brief
 *
 * @param ppu_p
 * @param addr
 * @param value
 */
void ppu_oam_write(ppu_t *ppu_p, uint16_t addr, uint8_t value)
{
   ppu_p->oam[addr] = value;
}

/**
 * @brief
 *
 * @param ppu_p
 * @param addr
 * @return uint8_t
 */
uint8_t ppu_oam_read(ppu_t *ppu_p, uint16_t addr)
{
   return ppu_p->oam[addr];
}
