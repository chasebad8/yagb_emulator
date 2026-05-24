#include "ppu/ppu.h"
#include "common/logging.h"

void ppu_init(ppu_t *ppu_p)
{
   LOG_DEBUG("initializing ppu ...");
   LOG_DEBUG("ppu init success!");
}

/* it was decided to have the bus handle the ppu instance. Instead of 1 static ppu instance that
   is handled in here, we let the bus create a ppu instance and pass it to this file. This way the ppu engine
   just takes a given ppu instance and acts on it. */
// PPU (Pixel Processing Unit) implementation
