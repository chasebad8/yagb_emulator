#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "ppu/ppu.h"
#include "cartridge/cartridge.h"

/* 8 KB wram max */
#define WRAM_SIZE 0x2000
/* 127 B hram max */
#define HRAM_SIZE 0x7F

typedef struct
{
   /* pointer to ppu and rom instances so they are not passed
      in for every bus operation */
   ppu_t       *ppu;
   cartridge_t *rom;
   uint8_t  wram[WRAM_SIZE];
   uint8_t  hram[HRAM_SIZE];

} bus_t;

void bus_init(bus_t       *bus_p,
              ppu_t       *ppu_p,
              cartridge_t *cartridge_p);

void bus_write(bus_t    *bus_p,
               uint16_t  addr,
               uint8_t   value);

uint8_t bus_read(bus_t    *bus_p,
                 uint16_t  addr);

#endif