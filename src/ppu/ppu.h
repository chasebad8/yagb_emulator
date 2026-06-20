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

enum ppu_state_e
{
   STATE_0_HBLANK,
   STATE_1_VBLANK,
   STATE_2_OAM_QUERY,
   STATE_3_PIXEL_TRANSFER,

};

/* we have to forward declare the bus structure
   this only works because the var in the ppu_t struct is a pointer
   and since it is a pointer we know the size of the datatype (32 bits) */
typedef struct bus_t bus_t;

typedef struct
{
   bus_t *bus;

   uint8_t vram[VRAM_SIZE];
   uint8_t oam[OAM_SIZE];

   enum ppu_state_e state;

   uint16_t tick_count;
   uint32_t frame_count;

} ppu_t;

void ppu_init(ppu_t *ppu_p, bus_t *bus_p);

void ppu_tick(ppu_t *ppu_p, uint8_t num_ticks);

void ppu_vram_write(ppu_t *ppu_p, uint16_t addr, uint8_t value);
uint8_t ppu_vram_read(ppu_t *ppu_p, uint16_t addr);

void ppu_oam_write(ppu_t *ppu_p, uint16_t addr, uint8_t value);
uint8_t ppu_oam_read(ppu_t *ppu_p, uint16_t addr);

#endif // PPU_H
