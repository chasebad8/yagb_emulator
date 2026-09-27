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
#include <stdlib.h>
#include "ppu/ppu.h"
#include "bus/bus.h"
#include "common/logging.h"

#define PPU_NUM_SCANLINES         (154)
#define PPU_CYCLES_PER_SCANLINE   (456)
#define PPU_CYCLES_PER_FRAME      (PPU_CYCLES_PER_SCANLINE * PPU_NUM_SCANLINES)
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

#define PPU_MAX_TILES (32 * 32)

#define PPU_IS_NEW_SCANLINE(ticks) ((ticks) == 0)
#define PPU_IS_NEW_FRAME(scanlines) ((scanlines) == 0)
#define PPU_LY_EQUALS_LYC(ly, lyc) ((ly) == (lyc))

/** Base addresses for tile maps (32x32 indices) */
#define PPU_TILE_MAP_0_BASE_ADDR (0x9800)
#define PPU_TILE_MAP_1_BASE_ADDR (0x9C00)

/** Base addresses for background tile data (8x8 pixel tile addresses) */
#define PPU_TILE_DATA_0_BASE_ADDR (0x8000)
#define PPU_TILE_DATA_1_BASE_ADDR (0x9000)

#define PPU_BYTES_PER_TILE (16)

/* the &0x1F is the same as %32 */
#define MODULO_32 0x1F
#define MODULO_8  0x07

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

/**
 * @brief
 *
 * @param ppu
 * @param tile_addr
 * @param pixel_index
 * @return uint8_t
 */
uint8_t ppu_get_tile_pixel_color_id(ppu_t *ppu, uint16_t tile_addr, uint8_t pixel_index)
{
   uint8_t tile_byte_low  = bus_read(ppu->bus, tile_addr);
   uint8_t tile_byte_high = bus_read(ppu->bus, tile_addr + 1);

   uint8_t curr_pixel_low_bit  = (tile_byte_low  >> (7 - (pixel_index % 8))) & 0x1;
   uint8_t curr_pixel_high_bit = (tile_byte_high >> (7 - (pixel_index % 8))) & 0x1;

   return (curr_pixel_high_bit << 1) | curr_pixel_low_bit;
}

/* tile map = the 32x32 tile map where the value is an index into the tile data
   tile data = the actual tile data, 16 bytes, contains pixel information */

uint8_t ppu_get_tile_index(ppu_t *ppu,
                                  uint8_t x_coord,
                                  uint8_t y_coord,
                                  enum tile_source_e tile_source)
{
   uint8_t tile_map_mode = (tile_source == TILE_SOURCE_BG) ?
      (bus_read_lcdc_reg(ppu->bus, LCDC_REG_BG_TILE_MAP_MASK)     >> LCDC_REG_BG_TILE_MAP_SHIFT) :
      (bus_read_lcdc_reg(ppu->bus, LCDC_REG_WINDOW_TILE_MAP_MASK) >> LCDC_REG_WINDOW_TILE_MAP_SHIFT);

   uint16_t tile_map_addr_offset = (tile_map_mode == 1) ? PPU_TILE_MAP_1_BASE_ADDR :
                                                          PPU_TILE_MAP_0_BASE_ADDR;

   return bus_read(ppu->bus, tile_map_addr_offset + (y_coord * 32) + x_coord);
}

/**
 * @brief $8000-$97FF tile data.
 *        LCDC bit 4 determines the tile
 *        data addressing mode:
 *
 * @param ppu
 * @param tile_index
 * @param tile_source
 */
uint16_t ppu_get_tile_data_addr(ppu_t             *ppu,
                                       uint8_t            tile_index,
                                       enum tile_source_e tile_source)
{
   uint8_t tile_data_mode =
      bus_read_lcdc_reg(ppu->bus, LCDC_REG_TILE_DATA_MASK) >> LCDC_REG_TILE_DATA_SHIFT;

   uint16_t tile_addr = 0;

   if(tile_index > PPU_MAX_TILES)
   {
      LOG_ERROR("invalid tile index received: 0x%0X", tile_index);
   }
   else
   {
      switch(tile_source)
      {
         /* sprites always use 0x8000 address map */
         case TILE_SOURCE_SPRITE:
            tile_addr = PPU_TILE_DATA_0_BASE_ADDR + (tile_index * PPU_BYTES_PER_TILE);
            break;

         case TILE_SOURCE_BG:
         case TILE_SOURCE_WINDOW:
            if(tile_data_mode == 0)
            {
               tile_addr = PPU_TILE_DATA_1_BASE_ADDR + ((int8_t)tile_index * PPU_BYTES_PER_TILE);
            }
            else
            {
               tile_addr = PPU_TILE_DATA_0_BASE_ADDR + (tile_index * PPU_BYTES_PER_TILE);
            }
            break;
      }
   }

   return tile_addr;
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

   sprite_attr.y_pos      = bus_read(ppu->bus, OAM_OFFSET + (4 * sprite_index));
   sprite_attr.x_pos      = bus_read(ppu->bus, OAM_OFFSET + (4 * sprite_index) + 1);
   sprite_attr.tile_index = bus_read(ppu->bus, OAM_OFFSET + (4 * sprite_index) + 2);
   sprite_attr.attributes = bus_read(ppu->bus, OAM_OFFSET + (4 * sprite_index) + 3);

   return sprite_attr;
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
static bool ppu_is_sprite_on_scanline(ppu_t *ppu, int16_t sprite_y_pos)
{
   int16_t curr_scanline  = bus_read(ppu->bus, LY_REG);
   bool    sprite_is_tall = bus_read_lcdc_reg(ppu->bus, LCDC_REG_OBJ_SIZE_MASK) >> LCDC_REG_OBJ_SIZE_SHIFT;
   int16_t sprite_height  = ((sprite_is_tall == true) ? 16 : 8);

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
   ;
}

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_1_vblank(ppu_t *ppu)
{
   ppu->window_y_active = false;
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

   ppu->sprite_count = 0;

   for (uint8_t sprite_index = 0; sprite_index < PPU_MAX_SPRITES; sprite_index++)
   {
      int16_t sprite_y_pos = ppu_get_sprite_attr(ppu, sprite_index).y_pos - PPU_SPRITE_Y_OFFSET;

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

   ppu->sprite_count = sprite_cnt;
}

/**
 * @brief
 *
 * @param ppu
 */
static void ppu_mode_3_pixel_transfer(ppu_t *ppu)
{
   uint8_t  curr_scanline = bus_read(ppu->bus, LY_REG);
   uint8_t  tile_index    = 0;
   uint16_t tile_addr     = 0;
   uint8_t  x_coord       = 0;
   uint8_t  y_coord       = 0;
   uint8_t  bg_color_ids[PPU_NUM_PIXELS_PER_SCANLINE];

   bool lcd_enabled =
      bus_read_lcdc_reg(ppu->bus, LCDC_REG_LCD_ENABLE_MASK) >> LCDC_REG_LCD_ENABLE_SHIFT;
   bool is_win_bg_enabled = false;
   bool is_window_enabled = false;
   bool window_render_started = false;
   enum tile_source_e tile_source = TILE_SOURCE_WINDOW;

   if(lcd_enabled == false)
   {
      return;
   }

   if (ppu->rendered_scanline == curr_scanline)
   {
      return;
   }

   ppu->rendered_scanline = curr_scanline;

   for (uint8_t pixel_index = 0; pixel_index < PPU_NUM_PIXELS_PER_SCANLINE; pixel_index++)
   {
      is_win_bg_enabled =
         bus_read_lcdc_reg(ppu->bus, LCDC_REG_WIN_BG_ENABLE_MASK) >> LCDC_REG_WIN_BG_ENABLE_SHIFT;
      is_window_enabled =
         bus_read_lcdc_reg(ppu->bus, LCDC_REG_WINDOW_ENABLE_MASK) >> LCDC_REG_WINDOW_ENABLE_SHIFT;

      if (is_win_bg_enabled == true)
      {
         uint8_t wx = bus_read(ppu->bus, WX_REG);
         uint8_t scx = bus_read(ppu->bus, SCX_REG);
         uint8_t scy = bus_read(ppu->bus, SCY_REG);

         if ((window_render_started == false) &&
             (is_window_enabled == true) &&
             (ppu->window_y_active == true) &&
             (pixel_index == wx - 7))
         {
            window_render_started = true;
         }

         uint16_t pixel_x_abs = 0;
         uint16_t pixel_y_abs = 0;

         if (window_render_started == true)
         {
            tile_source = TILE_SOURCE_WINDOW;
            pixel_x_abs = pixel_index - (wx - 7);
            pixel_y_abs = ppu->window_line;
         }
         else
         {
            tile_source = TILE_SOURCE_BG;
            pixel_x_abs = pixel_index + scx;
            pixel_y_abs = curr_scanline + scy;
         }

         /* compute tile coords */
         x_coord = (pixel_x_abs / 8) & MODULO_32;
         y_coord = (pixel_y_abs / 8) & MODULO_32;

         tile_index = ppu_get_tile_index(ppu, x_coord, y_coord, tile_source);

         /* row within tile (0-7) */
         uint8_t row_in_tile = pixel_y_abs & MODULO_8;
         tile_addr  = ppu_get_tile_data_addr(ppu, tile_index, tile_source) + (row_in_tile * 2);

         /* column within tile (0-7) */
         uint8_t col_in_tile = pixel_x_abs & MODULO_8;

         bg_color_ids[pixel_index] = ppu_get_tile_pixel_color_id(ppu, tile_addr, col_in_tile);
      }
      else
      {
         bg_color_ids[pixel_index] = 0;
      }

      ppu->frame_buffer[(PPU_NUM_PIXELS_PER_SCANLINE * curr_scanline) + pixel_index] =
         bg_color_ids[pixel_index];
   }

   if (window_render_started == true)
   {
      ppu->window_rendered_this_line = true;
   }

   bool objects_enabled =
      (bus_read_lcdc_reg(ppu->bus, LCDC_REG_OBJ_ENABLE_MASK) >> LCDC_REG_OBJ_ENABLE_SHIFT) != 0;
   if (objects_enabled == false)
   {
      return;
   }

   bool tall_sprites =
      (bus_read_lcdc_reg(ppu->bus, LCDC_REG_OBJ_SIZE_MASK) >> LCDC_REG_OBJ_SIZE_SHIFT) != 0;
   int16_t sprite_height = tall_sprites ? 16 : 8;

   for (uint8_t pixel_x = 0; pixel_x < PPU_NUM_PIXELS_PER_SCANLINE; pixel_x++)
   {
      bool pixel_has_sprite = false;
      int16_t best_sprite_x = 256;
      uint8_t best_sprite_index = 0xFF;
      uint8_t best_color_id = 0;
      uint8_t best_attributes = 0;

      for (uint8_t selected_index = 0; selected_index < ppu->sprite_count; selected_index++)
      {
         uint8_t sprite_index = ppu->sprite_arr[selected_index];
         sprite_attr_t sprite = ppu_get_sprite_attr(ppu, sprite_index);
         int16_t sprite_left = (int16_t)sprite.x_pos - 8;
         int16_t sprite_top = (int16_t)sprite.y_pos - PPU_SPRITE_Y_OFFSET;
         int16_t sprite_row = (int16_t)curr_scanline - sprite_top;
         int16_t sprite_col = (int16_t)pixel_x - sprite_left;

         if ((sprite_col < 0) || (sprite_col >= 8) ||
             (sprite_row < 0) || (sprite_row >= sprite_height))
         {
            continue;
         }

         if (sprite.attributes & 0x40)
         {
            sprite_row = sprite_height - 1 - sprite_row;
         }
         if (sprite.attributes & 0x20)
         {
            sprite_col = 7 - sprite_col;
         }

         uint8_t sprite_tile = sprite.tile_index;
         if (tall_sprites)
         {
            sprite_tile &= 0xFE;
            sprite_tile += sprite_row / 8;
            sprite_row %= 8;
         }

         uint16_t sprite_addr = 0x8000 + (sprite_tile * PPU_BYTES_PER_TILE) + (sprite_row * 2);
         uint8_t color_id = ppu_get_tile_pixel_color_id(ppu, sprite_addr, sprite_col);
         if (color_id == 0)
         {
            continue;
         }

         if ((pixel_has_sprite == false) || (sprite_left < best_sprite_x) ||
             ((sprite_left == best_sprite_x) && (sprite_index < best_sprite_index)))
         {
            pixel_has_sprite = true;
            best_sprite_x = sprite_left;
            best_sprite_index = sprite_index;
            best_color_id = color_id;
            best_attributes = sprite.attributes;
         }
      }

      if (pixel_has_sprite == false)
      {
         continue;
      }

      if ((best_attributes & 0x80) && (bg_color_ids[pixel_x] != 0))
      {
         continue;
      }

      uint16_t palette_register = (best_attributes & 0x10) ? OBP1_REG : OBP0_REG;
      uint8_t palette = bus_read(ppu->bus, palette_register);
      uint8_t shade = (palette >> (best_color_id * 2)) & 0x03;
      ppu->frame_buffer[(PPU_NUM_PIXELS_PER_SCANLINE * curr_scanline) + pixel_x] = shade;
   }
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
   ppu_p->lyc_triggered = false;
   ppu_p->window_line = 0;
   ppu_p->window_y_active = false;
   ppu_p->window_rendered_this_line = false;
   ppu_p->sprite_count = 0;
   ppu_p->rendered_scanline = 0xFF;
   ppu_p->oam_scanline = 0xFF;

   memset(ppu_p->vram, 0, VRAM_SIZE);
   memset(ppu_p->oam,  0, OAM_SIZE);
   memset(ppu_p->frame_buffer, 0, FRAME_BUFFER_SIZE);

   /* for fun init all vram tiles to the same image */
   for(uint16_t mult = 0; mult < 255; mult ++)
   {
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR]      = 0xFF;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 1]  = 0x00;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 2]  = 0x7E;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 3]  = 0xFF;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 4]  = 0x85;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 5]  = 0x81;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 6]  = 0x89;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 7]  = 0x83;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 8]  = 0x93;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 9]  = 0x85;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 10] = 0xA5;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 11] = 0x8B;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 12] = 0xC9;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 13] = 0x97;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 14] = 0x7E;
      ppu_p->vram[PPU_TILE_DATA_0_BASE_ADDR + (mult * PPU_BYTES_PER_TILE) - PPU_TILE_DATA_0_BASE_ADDR + 15] = 0xFF;
   }

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
   uint8_t scanline = bus_read(ppu->bus, LY_REG);

   ppu->tick_count = (ppu->tick_count + 1) % PPU_CYCLES_PER_SCANLINE;

   if(PPU_IS_NEW_SCANLINE(ppu->tick_count) == true)
   {
      if(PPU_IS_NEW_FRAME(scanline) == true)
      {
         ppu->frame_count++;
      }

      if (scanline == PPU_NUM_SCANLINES - 1)
      {
         ppu->window_line = 0;
         ppu->window_rendered_this_line = false;
      }
      else if (ppu->window_rendered_this_line == true)
      {
         ppu->window_line++;
         ppu->window_rendered_this_line = false;
      }

      bus_write(ppu->bus, LY_REG, ++scanline % PPU_NUM_SCANLINES);
   }

   if(bus_read(ppu->bus, LYC_REG) == scanline)
   {
      if(ppu->lyc_triggered == false)
      {
         bus_request_interrupt(ppu->bus, IF_REG_LCD_MASK, STAT_REG_LYC_INT_CONTRIB_MASK);
         ppu->lyc_triggered = true;
      }

      bus_write_stat_reg(ppu->bus, STAT_REG_LYC_EQ_LY_MASK, 1 << STAT_REG_LYC_EQ_LY_SHIFT);
   }
   else
   {
      bus_write_stat_reg(ppu->bus, STAT_REG_LYC_EQ_LY_MASK, 0 << STAT_REG_LYC_EQ_LY_SHIFT);
      ppu->lyc_triggered = false;
   }

   if(scanline == bus_read(ppu->bus, WY_REG))
   {
      ppu->window_y_active = true;
   }

   // LOG_DEBUG("ppu->tick_count %d, frame count %d, mode %d LY %d", ppu->tick_count,
   //                                                                ppu->frame_count,
   //                                                                bus_read_stat_reg(ppu->bus, STAT_REG_PPU_MODE_MASK),
   //                                                                bus_read(ppu->bus, LY_REG));

   uint8_t new_state = ppu->state;

   if (scanline >= PPU_NUM_VISIBLE_SCANLINES)
   {
      new_state = STATE_1_VBLANK;
   }
   else
   {
      uint32_t elapsed = PPU_ELAPSED_CYCLES_PER_SCANLINE(ppu->tick_count);

      if (elapsed >= PPU_MODE_0_OFFSET)
      {
         new_state = STATE_0_HBLANK;
      }
      else if (elapsed >= PPU_MODE_3_OFFSET)
      {
         new_state = STATE_3_PIXEL_TRANSFER;
      }
      else
      {
         new_state = STATE_2_OAM_QUERY;
      }
   }

   if(new_state != ppu->state)
   {
      ppu->state = new_state;
      bus_write_stat_reg(ppu->bus, STAT_REG_PPU_MODE_MASK, new_state << STAT_REG_PPU_MODE_SHIFT);

      switch(ppu->state)
      {
         case STATE_0_HBLANK:
            bus_request_interrupt(ppu->bus, IF_REG_LCD_MASK, STAT_REG_MODE_0_INT_CONTRIB_MASK);
            break;

         case STATE_1_VBLANK:
            bus_request_interrupt(ppu->bus, IF_REG_VBLANK_MASK, 0);
            bus_request_interrupt(ppu->bus, IF_REG_LCD_MASK, STAT_REG_MODE_1_INT_CONTRIB_MASK);
            break;

         case STATE_2_OAM_QUERY:
            bus_request_interrupt(ppu->bus, IF_REG_LCD_MASK, STAT_REG_MODE_2_INT_CONTRIB_MASK);
            break;

         default:
            break;
      }
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
void ppu_step(ppu_t *ppu, uint8_t num_ticks)
{
   bool lcd_enabled =
      (bus_read_lcdc_reg(ppu->bus, LCDC_REG_LCD_ENABLE_MASK) >> LCDC_REG_LCD_ENABLE_SHIFT) != 0;

   if (lcd_enabled == false)
   {
      ppu->tick_count = 0;
      ppu->state = STATE_0_HBLANK;
      ppu->window_line = 0;
      ppu->window_y_active = false;
      ppu->window_rendered_this_line = false;
      ppu->sprite_count = 0;
      ppu->oam_scanline = 0xFF;
      ppu->rendered_scanline = 0xFF;
      bus_write(ppu->bus, LY_REG, 0);
      bus_write_stat_reg(ppu->bus, STAT_REG_PPU_MODE_MASK, STATE_0_HBLANK);
      return;
   }

   /* ppu operates 1 tick at a time */
   uint8_t consumed_ticks = num_ticks;

   while(consumed_ticks-- > 0)
   {
      ppu_update_state_machine(ppu);

      if (ppu->state == STATE_2_OAM_QUERY)
      {
         uint8_t scanline = bus_read(ppu->bus, LY_REG);
         if (ppu->oam_scanline != scanline)
         {
            ppu_mode_2_oam_query(ppu);
            ppu->oam_scanline = scanline;
         }
      }

      switch(ppu->state)
      {
         case STATE_0_HBLANK:
            ppu_mode_0_hblank(ppu);
            break;

         case STATE_1_VBLANK:
            ppu_mode_1_vblank(ppu);
            break;

         case STATE_2_OAM_QUERY:
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
