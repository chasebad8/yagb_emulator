#ifndef INPUT_OUTPUT_H
#define INPUT_OUTPUT_H

#include <stdint.h>
#include "common/logging.h"

#define IO_RAM_SIZE (0x80) // 128 bytes of IO RAM

typedef struct
{
   uint8_t io_ram[IO_RAM_SIZE];

} io_t;

void io_init(io_t *io_p);

void io_ram_write(io_t *io_p, uint16_t addr, uint8_t value);
uint8_t io_ram_read(io_t *io_p, uint16_t addr);

// Input output handling header

#endif // INPUT_OUTPUT_H
