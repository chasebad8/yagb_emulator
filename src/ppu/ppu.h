#ifndef PPU_H
#define PPU_H

#include <stdint.h>

/* 8 KB max */
#define VRAM_SIZE (0x2000)
#define OAM_SIZE  (0xA0)
typedef struct
{
   uint8_t vram[VRAM_SIZE];
   uint8_t oam[OAM_SIZE];

} ppu_t;

void ppu_init(ppu_t *ppu_p);

void ppu_vram_write(ppu_t *ppu_p, uint16_t addr, uint8_t value);
uint8_t ppu_vram_read(ppu_t *ppu_p, uint16_t addr);

void ppu_oam_write(ppu_t *ppu_p, uint16_t addr, uint8_t value);
uint8_t ppu_oam_read(ppu_t *ppu_p, uint16_t addr);

// PPU (Pixel Processing Unit) header

#endif // PPU_H
