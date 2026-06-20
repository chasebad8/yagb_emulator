#include "input_output.h"

void io_init(io_t *io_p)
{
   LOG_DEBUG("initializing io ...");

   LOG_DEBUG("io init success!");
}

void io_ram_write(io_t *io_p, uint16_t addr, uint8_t value)
{
    io_p->io_ram[addr] = value;
}

uint8_t io_ram_read(io_t *io_p, uint16_t addr)
{
    return io_p->io_ram[addr];
}
