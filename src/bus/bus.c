#include <string.h>
#include <stdlib.h>
#include "bus/bus.h"
#include "common/logging.h"

/* --------------------------------------------------------------------------------------------------
from https://gbdev.io/pandocs/Memory_Map.html
0000	3FFF	16 KiB ROM bank 00	            From cartridge, usually a fixed bank
4000	7FFF	16 KiB ROM Bank 01–NN	         From cartridge, switchable bank via mapper (if any)
8000	9FFF	8 KiB Video RAM (VRAM)	         In CGB mode, switchable bank 0/1
A000	BFFF	8 KiB External RAM	            From cartridge, switchable bank if any
C000	CFFF	4 KiB Work RAM (WRAM)
D000	DFFF	4 KiB Work RAM (WRAM)	         In CGB mode, switchable bank 1–7
E000	FDFF	Echo RAM (mirror of C000–DDFF)	Nintendo says use of this area is prohibited.
FE00	FE9F	Object attribute memory (OAM)
FEA0	FEFF	Not Usable	                     Nintendo says use of this area is prohibited.
FF00	FF7F  I/O Registers
FF80	FFFE	High RAM (HRAM)
FFFF	FFFF	Interrupt Enable register (IE)
-------------------------------------------------------------------------------------------------- */
typedef enum {
    REGION_ROM,           /* 0x0000-0x7FFF */
    REGION_VRAM,          /* 0x8000-0x9FFF */
    REGION_CARTRIDGE_RAM, /* 0xA000-0xBFFF */
    REGION_WRAM,          /* 0xC000-0xDFFF */
    REGION_ECHO_RAM,      /* 0xE000-0xFDFF */
    REGION_OAM,           /* 0xFE00-0xFE9F */
    REGION_UNUSABLE,      /* 0xFEA0-0xFEFF */
    REGION_IO,            /* 0xFF00-0xFF7F */
    REGION_HRAM,          /* 0xFF80-0xFFFE */
    REGION_IE             /* 0xFFFF */
} memory_region_t;

/* this bus will be the service broker between the different components of the SoC.
   this bus should know where each device is memory mapped, but not WHAT the device does
   to reduce unecessary abstraction I will put the ram addresses in here. cartridge ROM will be
   handled in cartridge.c and VRAM will be handled in ppu.c */

void bus_init(bus_t       *bus_p,
              ppu_t       *ppu_p,
              cartridge_t *cartridge_p,
              io_t        *io_p)
{
   LOG_DEBUG("initializing bus ...");

   if(bus_p == NULL)
   {
      LOG_ERROR("bus pointer is NULL");
      exit(-1);
   }
   else if (ppu_p == NULL)
   {
      LOG_ERROR("ppu pointer is NULL");
      exit(-1);
   }
   else if (cartridge_p == NULL)
   {
      LOG_ERROR("cartridge pointer is NULL");
      exit(-1);
   }
   else
   {
      bus_p->io  = io_p;
      bus_p->ppu = ppu_p;
      bus_p->rom = cartridge_p;

      memset(bus_p->wram, 0, WRAM_SIZE * sizeof(uint8_t));
      memset(bus_p->hram, 0, HRAM_SIZE * sizeof(uint8_t));

      LOG_DEBUG("bus init success!");
   }
}

static const char* bus_memory_region_to_string(memory_region_t memory_region)
{
   switch(memory_region)
   {
      case REGION_ROM:             return "REGION_ROM";
      case REGION_VRAM:            return "REGION_VRAM";
      case REGION_CARTRIDGE_RAM:   return "REGION_CARTRIDGE_RAM";
      case REGION_WRAM:            return "REGION_WRAM";
      case REGION_ECHO_RAM:        return "REGION_ECHO_RAM";
      case REGION_OAM:             return "REGION_OAM";
      case REGION_UNUSABLE:        return "REGION_UNUSABLE";
      case REGION_IO:              return "REGION_IO";
      case REGION_HRAM:            return "REGION_HRAM";
   }
}

static memory_region_t bus_get_region(uint16_t addr)
{
    if (addr <= 0x7FFF)   return REGION_ROM;
    if (addr <= 0x9FFF)   return REGION_VRAM;
    if (addr <= 0xBFFF)   return REGION_CARTRIDGE_RAM;
    if (addr <= 0xDFFF)   return REGION_WRAM;
    if (addr <= 0xFDFF)   return REGION_ECHO_RAM;
    if (addr <= 0xFE9F)   return REGION_OAM;
    if (addr <= 0xFEFF)   return REGION_UNUSABLE;
    if (addr <= 0xFF7F)   return REGION_IO;
    if (addr <= 0xFFFE)   return REGION_HRAM;
    return REGION_IE;
}

uint8_t bus_read(bus_t *bus_p, uint16_t addr)
{
   if ((bus_p->ppu == NULL) || (bus_p->rom == NULL))
   {
      LOG_ERROR("PPU (%p) or ROM (%p) not connected to bus", bus_p->ppu, bus_p->rom);
      exit(-1);
   }
   else
   {
      switch (bus_get_region(addr))
      {
         case REGION_ROM:
            return cartridge_read(bus_p->rom, addr);
         case REGION_VRAM:
            return ppu_vram_read(bus_p->ppu, addr - 0x8000);
         case REGION_CARTRIDGE_RAM:
            LOG_ERROR("%s read not implemented", bus_memory_region_to_string(bus_get_region(addr)));
            exit(-1);
         case REGION_WRAM:
            return bus_p->wram[addr - 0xC000];
         case REGION_ECHO_RAM:
            LOG_ERROR("%s read not implemented", bus_memory_region_to_string(bus_get_region(addr)));
            exit(-1);
         case REGION_OAM:
            return ppu_oam_read(bus_p->ppu, addr - 0xFE00);
         case REGION_UNUSABLE:
            LOG_ERROR("illegal read of unusable memory requested: 0x%04X", addr);
            exit(-1);
         case REGION_IO:
            return io_ram_read(bus_p->io, addr - 0xFF00);
         case REGION_HRAM:
            return bus_p->hram[addr - 0xFF80];
         case REGION_IE:
            LOG_ERROR("%s read not implemented", bus_memory_region_to_string(bus_get_region(addr)));
            exit(-1);
         default:
            return 0xFF;
      }
   }
}

void bus_write(bus_t *bus_p, uint16_t addr, uint8_t value)
{
   if ((bus_p->ppu == NULL) || (bus_p->rom == NULL))
   {
      LOG_ERROR("PPU (%p) or ROM (%p) not connected to bus", bus_p->ppu, bus_p->rom);
      exit(-1);
   }
   else
   {
      switch (bus_get_region(addr))
      {
         case REGION_ROM:
#ifndef DEBUG_MODE
            LOG_ERROR("illegal write of rom requested: 0x%04X", addr);
            exit(-1);
#else
            cartridge_write(bus_p->rom, addr, value);
#endif
            break;
         case REGION_VRAM:
            ppu_vram_write(bus_p->ppu, addr - 0x8000, value);
            break;
         case REGION_CARTRIDGE_RAM:
            LOG_ERROR("%s write not implemented", bus_memory_region_to_string(bus_get_region(addr)));
            exit(-1);
            break;
         case REGION_WRAM:
            bus_p->wram[addr - 0xC000] = value;
            break;
         case REGION_ECHO_RAM:
            LOG_ERROR("%s write not implemented", bus_memory_region_to_string(bus_get_region(addr)));
            exit(-1);
            break;
         case REGION_OAM:
            ppu_oam_write(bus_p->ppu, addr - 0xFE00, value);
            break;
         case REGION_UNUSABLE:
            LOG_ERROR("illegal write of unusable memory requested: 0x%04X", addr);
            exit(-1);
         case REGION_IO:
            io_ram_write(bus_p->io, addr - 0xFF00, value);
            break;
         case REGION_HRAM:
            bus_p->hram[addr - 0xFF80] = value;
            break;
         case REGION_IE:
            LOG_ERROR("%s write not implemented", bus_memory_region_to_string(bus_get_region(addr)));
            exit(-1);
            break;
      }
   }
}

/**
 * @brief set interrupt mask flag in IF reg.
 *        if lcd interrupt, confirm the contributor
 *        is set in LCD STAT reg before raising.
 *
 */
void bus_request_interrupt(bus_t          *bus,
                           if_reg_mask_t   interrupt_mask,
                           stat_reg_mask_t lcd_interrupt_contrib_mask)
{
   if ((interrupt_mask == IF_REG_LCD_MASK) &&
       ((bus_read(bus, STAT_REG) & lcd_interrupt_contrib_mask) == 0))
   {
      return;
   }

   bus_write(bus, IF_REG, bus_read(bus, IF_REG) | interrupt_mask);
}

void bus_get_lcd_stat_mask(bus_t *bus);
