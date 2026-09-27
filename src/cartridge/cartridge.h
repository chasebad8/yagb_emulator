#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stddef.h>
#include <stdint.h>

/* 32 KB max */
#define MAX_ROM_SIZE 0x8000
#define MBC1_MAX_ROM_SIZE 0x200000
#define MBC1_RAM_MAX_SIZE 0x8000

typedef enum
{
   CARTRIDGE_MAPPER_ROM_ONLY,
   CARTRIDGE_MAPPER_MBC1
} cartridge_mapper_t;

typedef struct
{
   size_t rom_size;
   size_t rom_bank_count;
   uint8_t *rom;
   uint8_t *ram;
   size_t ram_size;
   size_t ram_bank_count;
   cartridge_mapper_t mapper;
   uint8_t mbc1_ram_enabled;
   uint8_t mbc1_rom_bank_low5;
   uint8_t mbc1_bank_high2;
   uint8_t mbc1_banking_mode;

} cartridge_t;

void cartridge_init(cartridge_t *cartridge_p);

void cartridge_load(cartridge_t *cartridge_p, const char* cartridge_path);

void cartridge_unload(cartridge_t *cartridge_p);

uint8_t cartridge_read(cartridge_t *cartridge_p, uint16_t addr);

void cartridge_write(cartridge_t *cartridge_p, uint16_t addr, uint8_t value);

uint8_t cartridge_ram_read(cartridge_t *cartridge_p, uint16_t addr);

void cartridge_ram_write(cartridge_t *cartridge_p, uint16_t addr, uint8_t value);

#endif // CARTRIDGE_H
