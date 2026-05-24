#ifndef EMULATOR_H
#define EMULATOR_H

#include "cpu/cpu.h"
#include "bus/bus.h"
#include "ppu/ppu.h"
#include "cartridge/cartridge.h"

/*
        ┌────────────┐
        │    CPU     │
        └─────┬──────┘
              │ bus_read / bus_write
              ▼
        ┌────────────┐
        │    BUS     │  ← memory routing
        └─────┬──────┘
      ┌───────┼────────┬─────────┐
      ▼       ▼        ▼         ▼
   ROM       VRAM    WRAM       HRAM
*/

/* 0x0000 - 0x7FFF,
   0xA000 - 0xBFFF (possibly)
   ┌─────────┐
   │   ROM   │
   └─────────┘
*/

/* 0x8000 - 0x9FFF
   ┌──────────┐
   │   VRAM   │
   └──────────┘
*/

/* 0xC000 - 0xDFFF
   ┌──────────┐
   │   WRAM   │
   └──────────┘
*/

/* 0xFF80 - 0xFFFE
   ┌──────────┐
   │   HRAM   │
   └──────────┘
*/

typedef struct
{
   cpu_t       cpu;
   bus_t       bus;
   ppu_t       ppu;
   cartridge_t rom;

} emulator_t;

void emulator_init(emulator_t *emulator);

#endif