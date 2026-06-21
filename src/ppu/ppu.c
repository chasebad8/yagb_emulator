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

#include <string.h>
#include <stdbool.h>
#include "ppu/ppu.h"
#include "bus/bus.h"
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

#define PPU_SPRITE_Y_OFFSET (16)

typedef struct
{
   uint8_t y_pos;
   uint8_t x_pos;
   uint8_t tile_index;
   uint8_t attributes;

} sprite_attr_t;

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

static void ppu_get_tile(ppu_t             *ppu,
                         uint8_t            tile_index,
                         enum tile_source_e tile_source)
{
   /* check the type */
   if(tile_source == TILE_SOURCE_SPRITE)
   {

   }
}

/**
 * @brief based on the sprite index, return
 *        a struct of attr from OAM ram
 *
 * @param ppu
 * @param sprite_index
 * @return sprite_attr_t
 */
static sprite_attr_t ppu_get_sprite_attr(ppu_t *ppu, uint8_t sprite_index)
{
   sprite_attr_t sprite_attr = { 0 };

   sprite_attr.y_pos      = bus_read(ppu->bus, OAM_OFFSET + (4 * sprite_index++));
   sprite_attr.y_pos      = bus_read(ppu->bus, OAM_OFFSET + (4 * sprite_index++));
   sprite_attr.tile_index = bus_read(ppu->bus, OAM_OFFSET + (4 * sprite_index++));
   sprite_attr.attributes = bus_read(ppu->bus, OAM_OFFSET + (4 * sprite_index++));

   return sprite_attr
}

/**
 * @brief read the current scanline and current sprite height.
 *        then, compare if any pixels of the current sprite will
 *        be on the current scanline.
 *
 * @param ppu
 * @param sprite_y_pos
 * @return true
 * @return false
 */
static bool ppu_is_sprite_on_scanline(ppu_t *ppu, uint8_t sprite_y_pos)
{
   uint8_t curr_scanline  = bus_read(ppu->bus, LY_REG);
   bool    sprite_is_tall = (bus_read(ppu->bus, LCDC_REG) & 0x4) >> 3;
   uint8_t sprite_height  = ((sprite_is_tall == true) ? 16 : 8);

   if ((curr_scanline >= sprite_y_pos) && (curr_scanline < (sprite_y_pos + sprite_height)))
   {
      return true;
   }
   else
   {
      return false;
   }
}

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_0_hblank(ppu_t *ppu)
{
   bus_request_interrupt(ppu->bus, IF_REG_LCD_MASK, STAT_REG_MODE_0_INT_CONTRIB_MASK);
}

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_1_vblank(ppu_t *ppu)
{
   bus_request_interrupt(ppu->bus, IF_REG_VBLANK_MASK, 0);
   bus_request_interrupt(ppu->bus, IF_REG_LCD_MASK, STAT_REG_MODE_1_INT_CONTRIB_MASK);
}

/**
 * @brief loop through OAM to see if any of the 40
 *        possible sprites are on the current scan
 *        line. If they are, add to the list.
 *
 * @param ppu
 */
static void ppu_mode_2_oam_query(ppu_t *ppu)
{
   uint8_t sprite_cnt = 0;

   bus_request_interrupt(ppu->bus, IF_REG_LCD_MASK, STAT_REG_MODE_2_INT_CONTRIB_MASK);

   for (uint8_t sprite_index = 0; sprite_index < PPU_MAX_SPRITES; sprite_index++)
   {
      uint8_t sprite_y_pos = ppu_get_sprite_attr(ppu).y_pos - PPU_SPRITE_Y_OFFSET;

      if (ppu_is_sprite_on_scanline(ppu, sprite_y_pos) == true)
      {
         ppu->sprite_arr[sprite_cnt] = sprite_index;
         sprite_cnt++;
      }

      /* max of 10 sprites per scanline */
      if (sprite_cnt == 10)
      {
         break;
      }
   }
}

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_3_pixel_transfer(ppu_t *ppu)
{
   /* this is where I will need to call fifo updating */
   /* this is also where I will need to pass off to SDL driver */

   /* for x pixels in row
         fetch background
         fetch window

         place sprites (already fetched)
   */
}

/**
 * @brief the ppu is both a producer and a consumer
 *        and therefore we need to include the bus.
 *
 * @param ppu_p
 * @param bus_p
 */
void ppu_init(ppu_t *ppu_p, bus_t *bus_p)
{
   LOG_DEBUG("initializing ppu ...");

   ppu_p->bus         = bus_p;
   ppu_p->state       = STATE_2_OAM_QUERY;
   ppu_p->tick_count  = 0;
   ppu_p->frame_count = 0;

   memset(ppu_p->vram, 0, VRAM_SIZE);
   memset(ppu_p->oam,  0, OAM_SIZE);

   LOG_DEBUG("ppu init success!");
}

/**
 * @brief state machine moved from mode 2, 3, 0 for 144
          scanlines before entering mode 1 for 10 scanlines
 *
 * @param ppu
 */
static void ppu_update_state_machine(ppu_t *ppu)
{
   ppu->frame_count += ppu->tick_count / PPU_CYCLES_PER_FRAME;
   ppu->tick_count  += ppu->tick_count % PPU_CYCLES_PER_FRAME;

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

   /* update state in STAT reg */
   bus_write(ppu->bus, STAT_REG, bus_read(ppu->bus, IF_REG) | ppu->state);
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
void ppu_step(ppu_t *ppu, uint8_t num_ticks)
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
