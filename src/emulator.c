#include <stdint.h>
#include "emulator.h"
#include "common/logging.h"

/* an emulator instance will be created for 1 emulator. Technically we can support multiple emulators at once. */
void emulator_init(emulator_t *emulator)
{
   LOG_DEBUG("initializing emulator ...");

   /* memory broker */
   bus_init(&emulator->bus, &emulator->ppu, &emulator->rom);

   /* central processing unit */
   cpu_init(&emulator->cpu, &emulator->bus);

   /* pixel processing unit */
   ppu_init(&emulator->ppu);

   /* game cartridge handling unit */
   cartridge_init(&emulator->rom);

   LOG_DEBUG("emulator init success!\n");
}