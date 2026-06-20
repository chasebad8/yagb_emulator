#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "ppu/ppu.h"
#include "cartridge/cartridge.h"
#include "input_output/input_output.h"

/* 8 KB wram max */
#define WRAM_SIZE 0x2000
/* 127 B hram max */
#define HRAM_SIZE 0x7F

/**
 * @brief interrupt flag masks
 *
 */
typedef enum
{
   IF_REG_VBLANK_MASK = 0x01,
   IF_REG_LCD_MASK    = 0x02,
   IF_REG_TIMER_MASK  = 0x04,
   IF_REG_SERIAL_MASK = 0x08,
   IF_REG_JOYPAD_MASK = 0x10

} if_reg_mask_t;

/**
 * @brief LCD Status register interrupt configuration
 *
 */
typedef enum
{
   STAT_REG_LYC_EQ_LY_CONTRIB_MASK  = 0x04,
   STAT_REG_MODE_0_INT_CONTRIB_MASK = 0x08,
   STAT_REG_MODE_1_INT_CONTRIB_MASK = 0x10,
   STAT_REG_MODE_2_INT_CONTRIB_MASK = 0x20,
   STAT_REG_LYC_INT_CONTRIB_MASK    = 0x40

} stat_reg_mask_t;

/**
 * @brief register names for io ram
 *
 */
typedef enum
{
   P1_JOYP_REG = 0xFF00, /* joypad */
   SB_REG      = 0xFF01, /* serial transfer data */
   SC_REG      = 0xFF02, /* serial transfer control */

   DIV_REG     = 0xFF04, /* divider register */
   TIMA_REG    = 0xFF05, /* timer counter */
   TMA_REG     = 0xFF06, /* timer modulo */
   TAC_REG     = 0xFF07, /* timer control */

   IF_REG      = 0xFF0F, /* interrupt flag */

   NR10_REG    = 0xFF10, /* sound channel 1 sweep */
   NR11_REG    = 0xFF11, /* sound channel 1 length timer & duty cycle */
   NR12_REG    = 0xFF12, /* sound channel 1 volume & envelope */
   NR13_REG    = 0xFF13, /* sound channel 1 period low */
   NR14_REG    = 0xFF14, /* sound channel 1 period high & control */

   NR21_REG    = 0xFF16, /* sound channel 2 length timer & duty cycle */
   NR22_REG    = 0xFF17, /* sound channel 2 volume & envelope */
   NR23_REG    = 0xFF18, /* sound channel 2 period low */
   NR24_REG    = 0xFF19, /* sound channel 2 period high & control */

   NR30_REG    = 0xFF1A, /* sound channel 3 dac enable */
   NR31_REG    = 0xFF1B, /* sound channel 3 length timer */
   NR32_REG    = 0xFF1C, /* sound channel 3 output level */
   NR33_REG    = 0xFF1D, /* sound channel 3 period low */
   NR34_REG    = 0xFF1E, /* sound channel 3 period high & control */

   NR41_REG    = 0xFF20, /* sound channel 4 length timer */
   NR42_REG    = 0xFF21, /* sound channel 4 volume & envelope */
   NR43_REG    = 0xFF22, /* sound channel 4 frequency & randomness */
   NR44_REG    = 0xFF23, /* sound channel 4 control */

   NR50_REG    = 0xFF24, /* master volume & vin panning */
   NR51_REG    = 0xFF25, /* sound panning */
   NR52_REG    = 0xFF26, /* sound on/off */

   /* PPU registers */
   LCDC_REG    = 0xFF40, /* lcd control */
   STAT_REG    = 0xFF41, /* lcd status */
   SCY_REG     = 0xFF42, /* scroll y */
   SCX_REG     = 0xFF43, /* scroll x. This is the offset in which to start drawing */
   LY_REG      = 0xFF44, /* lcd y coordinate */
   LYC_REG     = 0xFF45, /* ly compare */
   DMA_REG     = 0xFF46, /* oam dma source address & start */
   BGP_REG     = 0xFF47, /* bg palette data */
   OBP0_REG    = 0xFF48, /* obj palette 0 data */
   OBP1_REG    = 0xFF49, /* obj palette 1 data */
   WY_REG      = 0xFF4A, /* window y position */
   WX_REG      = 0xFF4B, /* window x position plus 7 */

   KEY0_REG    = 0xFF4C, /* cpu mode select */
   KEY1_REG    = 0xFF4D, /* prepare speed switch */
   VBK_REG     = 0xFF4F, /* vram bank */

   BANK_REG    = 0xFF50, /* boot rom mapping control */

   HDMA1_REG   = 0xFF51, /* vram dma source high */
   HDMA2_REG   = 0xFF52, /* vram dma source low */
   HDMA3_REG   = 0xFF53, /* vram dma destination high */
   HDMA4_REG   = 0xFF54, /* vram dma destination low */
   HDMA5_REG   = 0xFF55, /* vram dma length/mode/start */

   RP_REG      = 0xFF56, /* infrared communications port */

   BCPS_REG    = 0xFF68, /* background color palette specification */
   BCPD_REG    = 0xFF69, /* background color palette data */
   OCPS_REG    = 0xFF6A, /* obj color palette specification */
   OCPD_REG    = 0xFF6B, /* obj color palette data */
   OPRI_REG    = 0xFF6C, /* object priority mode */

   SVBK_REG    = 0xFF70, /* wram bank */

   PCM12_REG   = 0xFF76, /* audio digital outputs 1 & 2 */
   PCM34_REG   = 0xFF77, /* audio digital outputs 3 & 4 */

   IE_REG      = 0xFFFF  /* interrupt enable */

} register_addresses_t;


typedef struct bus_t
{
   /* pointer to ppu and rom instances so they are not passed
      in for every bus operation */
   io_t        *io;
   ppu_t       *ppu;
   cartridge_t *rom;

   uint8_t  wram[WRAM_SIZE];
   uint8_t  hram[HRAM_SIZE];

} bus_t;

void bus_init(bus_t       *bus_p,
              ppu_t       *ppu_p,
              cartridge_t *cartridge_p,
              io_t        *io_p);

void bus_write(bus_t    *bus_p,
               uint16_t  addr,
               uint8_t   value);

uint8_t bus_read(bus_t    *bus_p,
                 uint16_t  addr);

void bus_request_interrupt(bus_t          *bus,
                           if_reg_mask_t   interrupt_mask,
                           stat_reg_mask_t lcd_interrupt_contrib_mask);
#endif