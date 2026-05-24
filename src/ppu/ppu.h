#ifndef PPU_H
#define PPU_H

#include <stdint.h>

/* 8 KB max */
#define VRAM_SIZE (0x2000)

typedef struct
{
   uint8_t vram[VRAM_SIZE];

} ppu_t;

void ppu_init(ppu_t *ppu_p);

// PPU (Pixel Processing Unit) header

#endif // PPU_H
