#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "bus/bus.h"

typedef struct
{
   /*
      note: the gameboy is little endian (lsb at lowest addr)
            this is why F byte comes before A byte
   */
   union
   {
      struct
      {
         uint8_t F; /* also flag register */
         uint8_t A;
      };
      uint16_t AF;
   };

   union
   {
      struct
      {
         uint8_t C;
         uint8_t B;
      };
      uint16_t BC;
   };

   union
   {
      struct
      {
         uint8_t E;
         uint8_t D;
      };
      uint16_t DE;
   };

   union
   {
      struct
      {
         uint8_t L;
         uint8_t H;
      };
      uint16_t HL;
   };

   uint16_t SP; /* stack pointer */
   uint16_t PC; /* program counter */

   bus_t *bus; /* pointer to bus instance */

} cpu_t;

typedef void (*opcode_handler_t)(cpu_t *cpu, uint8_t opcode);

void cpu_init(cpu_t *cpu, bus_t *bus);

void cpu_step(cpu_t *cpu);

#endif // CPU_H
