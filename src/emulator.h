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

/* 255 tiles by 8x8 pixels */
#define VRAM_DEBUG_BUFFER_SIZE 16384

typedef struct
{
   io_t        io;
   cpu_t       cpu;
   bus_t       bus;
   ppu_t       ppu;
   cartridge_t rom;

   uint32_t rgb_frame_buffer[FRAME_BUFFER_SIZE];
   uint32_t vram_dump_buffer[VRAM_DEBUG_BUFFER_SIZE];

   /* system tick counter using T-cycles.
      each memory read is 4 T-cycles */
   uint32_t sys_tick;

} emulator_t;

void emulator_init(emulator_t *emulator);

void emulator_load_game_cartridge(emulator_t *emulator, const char *game_cartridge_path);

void emulator_unload_game_cartridge(emulator_t *emulator);

void emulator_run(emulator_t *emulator);

void emulator_shutdown(emulator_t *emulator);
#endif