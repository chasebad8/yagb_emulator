#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>

/* 32 KB max */
#define MAX_ROM_SIZE 0x8000

typedef struct
{
   long  rom_size;
   uint8_t *rom;

} cartridge_t;

void cartridge_init(cartridge_t *cartridge_p);

void cartridge_load(cartridge_t *cartridge_p, const char* cartridge_path);

void cartridge_unload(cartridge_t *cartridge_p);

uint8_t cartridge_read(cartridge_t *cartridge_p, uint16_t addr);

void cartridge_write(cartridge_t *cartridge_p, uint16_t addr, uint8_t value);

#endif // CARTRIDGE_H
