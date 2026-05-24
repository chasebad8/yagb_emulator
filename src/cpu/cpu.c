#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cpu.h"
#include "cpu_opcodes.h"
#include "common/logging.h"

/* this can be global, its read only */
static opcode_handler_t opcode_table[OP_MAX]     = { 0 };
static bool             opcode_table_initialized = false;

static void op_nop(cpu_t *cpu, uint8_t opcode)
{
   LOG_DEBUG("opcode %0X NOP", opcode);
}

static void op_unimplemented(cpu_t *cpu, uint8_t opcode)
{
   LOG_WARN("opcode 0x%0X is not implemented", opcode);

#ifdef EXIT_ON_BAD_OPCODE
   exit(-1);
#endif
}

static void cpu_bind_opcodes(void)
{
   if(opcode_table_initialized == false)
   {
      for(uint8_t opcode = 0; opcode < OP_MAX; opcode++)
      {
         opcode_table[opcode] = op_unimplemented;
      }

      opcode_table[OP_NOP] = op_nop;
   }

   opcode_table_initialized = true;
}

void cpu_init(cpu_t *cpu, bus_t *bus)
{
   LOG_DEBUG("initializing cpu ...");

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

   cpu_bind_opcodes();

   LOG_DEBUG("cpu init success!");

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
      uint8_t opcode = bus_read(cpu->bus, cpu->PC++);
      opcode_table[opcode](cpu, opcode);
   }
}

// CPU implementation
