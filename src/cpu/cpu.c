#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "common/logging.h"

void cpu_init(cpu_t *cpu, bus_t *bus)
{
   cpu->A = 0;
   cpu->B = 0;
   cpu->C = 0;
   cpu->D = 0;
   cpu->E = 0;
   cpu->F = 0;
   cpu->H = 0;
   cpu->L = 0;
   cpu->PC = 0;
   cpu->SP = 0;

   cpu->bus = bus;
}

void cpu_step(cpu_t *cpu)
{
   /* fetch, decode, execute baby */
   if(cpu->bus == NULL)
   {
      LOG_ERROR("cpu not connected to bus");
      exit(-1);
   }
   else
   {
      /* fetch */
      LOG_DEBUG("FETCH");
      uint8_t opcode = bus_read(cpu->bus, cpu->PC++);
      LOG_DEBUG("opcode: 0x%0X", opcode);

      /*decode and execute */
      LOG_DEBUG("DECODE & EXECUTE");
      switch(opcode)
      {
         case 0x00:
            LOG_DEBUG("NO OP");
            break;
         default:
            LOG_WARN("opcode not implemented");
            break;
      }

      if(opcode != 0x0)
      {
         exit(1);
      }
   }
}

// CPU implementation
