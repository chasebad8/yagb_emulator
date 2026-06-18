#ifndef PPU_H
#define PPU_H

#include <stdint.h>

/* 8 KB max */
#define VRAM_SIZE (0x2000)
#define OAM_SIZE  (0xA0)

#define LY_REG       0xFF44
#define LYC_REG      0xFF45

#define LCD_STAT_REG 0xFF41
#define LCD_STAT_PPU_MODE_MASK  0x03
#define LCD_STAT_LYC_EQ_LY_MASK 0x04
#define LCD_STAT_MODE_0_MASK    0x08
#define LCD_STAT_MODE_1_MASK    0x10
#define LCD_STAT_MODE_2_MASK    0x20
#define LCD_STAT_MODE_0_LYC_INT_ENABLE_MASK 0x08

#define LCD_CTRL_REG 0xFF40


// enum
// {

// } ppu_mode_e;

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
