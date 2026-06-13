#include "ppu/ppu.h"
#include "common/logging.h"

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

/* it was decided to have the bus handle the ppu instance. Instead of 1 static ppu instance that
   is handled in here, we let the bus create a ppu instance and pass it to this file. This way the ppu engine
   just takes a given ppu instance and acts on it. */
// PPU (Pixel Processing Unit) implementation
