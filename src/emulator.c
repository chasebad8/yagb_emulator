#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "emulator.h"
#include "common/logging.h"

/* an emulator instance will be created for 1 emulator. Technically we can support multiple emulators at once. */
void emulator_init(emulator_t *emulator)
{
   LOG_INFO("initializing emulator ...");

   if(emulator == NULL)
   {
      LOG_ERROR("emulator pointer is NULL");
      exit(-1);
   }
   else
   {
      /* memory broker */
      bus_init(&emulator->bus, &emulator->ppu, &emulator->rom, &emulator->io);

      /* central processing unit */
      cpu_init(&emulator->cpu, &emulator->bus);

      /* pixel processing unit */
      ppu_init(&emulator->ppu, &emulator->bus);

      /* game cartridge handling unit */
      cartridge_init(&emulator->rom);

      io_init(&emulator->io);

      LOG_INFO("emulator init success!\n");
   }
}

void emulator_load_game_cartridge(emulator_t *emulator, const char *game_cartridge_path)
{
   if(emulator == NULL)
   {
      LOG_ERROR("emulator pointer is NULL");
      exit(-1);
   }
   else if (game_cartridge_path == NULL)
   {
      LOG_ERROR("invalid path to game cartridge");
      exit(-1);
   }
   else
   {
      cartridge_load(&emulator->rom, game_cartridge_path);
   }
}

void emulator_unload_game_cartridge(emulator_t *emulator)
{
   if(emulator == NULL)
   {
      LOG_ERROR("emulator pointer is NULL");
      exit(-1);
   }
   else
   {
      cartridge_unload(&emulator->rom);
   }
}