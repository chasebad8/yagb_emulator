#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "bus/bus.h"

typedef struct
{
   uint8_t A; /* accumulator */
   uint8_t F; /* flags */
   uint8_t B; /* general purpose registers */
   uint8_t C;
   uint8_t D;
   uint8_t E;
   uint8_t H;
   uint8_t L;
   uint16_t SP; /* stack pointer */
   uint16_t PC; /* program counter */

   bus_t *bus; /* pointer to bus instance */

} cpu_t;

typedef void (*opcode_handler_t)(cpu_t *cpu, uint8_t opcode);

void cpu_init(cpu_t *cpu, bus_t *bus);

void cpu_step(cpu_t *cpu);

#endif // CPU_H
