#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cpu.h"
#include "cpu_opcodes.h"
#include "common/logging.h"

/* this can be global, its read only */
static opcode_handler_t opcode_table[OP_MAX] = {
   op_nop,      op_ld_r16_i16, op_ld_m16_r8, NULL,        NULL,        NULL,        op_ld_r8_i8, NULL,
   NULL,        NULL,          NULL,         NULL,        NULL,        NULL,        op_ld_r8_i8, NULL,

   NULL,        op_ld_r16_i16, op_ld_m16_r8, NULL,        NULL,        NULL,        op_ld_r8_i8, NULL,
   NULL,        NULL,          NULL,         NULL,        NULL,        NULL,        op_ld_r8_i8, NULL,

   NULL,        op_ld_r16_i16, op_ld_m16_r8, NULL,        NULL,        NULL,        op_ld_r8_i8, NULL,
   NULL,        NULL,          NULL,         NULL,        NULL,        NULL,        op_ld_r8_i8, NULL,

   NULL,        op_ld_r16_i16, op_ld_m16_r8, NULL,        NULL,        NULL,        op_ld_r8_i8, NULL,
   NULL,        NULL,          NULL,         NULL,        NULL,        NULL,        op_ld_r8_i8, NULL,

   op_ld_r8_r8, op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8,
   op_ld_r8_r8, op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8,

   op_ld_r8_r8, op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8,
   op_ld_r8_r8, op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8,

   op_ld_r8_r8, op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8,
   op_ld_r8_r8, op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8,

   op_ld_r8_r8, op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, NULL,        op_ld_r8_r8,
   op_ld_r8_r8, op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8, op_ld_r8_r8,

   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

/* BLOCK 0 OPCODES */
static void op_nop(cpu_t *cpu, uint8_t opcode)
{
   LOG_DEBUG("opcode %0X NOP", opcode);
}

/*
 i8, i16 == immediate 8 or 16 (next byte or 2 from memory)
 r8, r16 == register 8 or 16  (group contiguous register if 16)
 m8, m16 == memory 8 or 16    (get value at register in memory pointed to by m8 or m16)
 opcode, dest, src
*/

/* load a 16 bit register with a 16 bit value */
static void op_ld_r16_i16(cpu_t *cpu, uint8_t opcode)
{
   /* grab the next 2 bytes in memory (little endian)*/
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t src_word  = ((high_byte << 8) | low_byte);

   uint8_t dest_reg = ((opcode >> 4) & 0x3);

   switch(dest_reg)
   {
      case 0: cpu->AF = src_word; break;
      case 1: cpu->BC = src_word; break;
      case 2: cpu->DE = src_word; break;
      case 3: cpu->HL = src_word; break;
   }
}

/* load address pointed to by a 16 bit register with 8 bit value */
static void op_ld_m16_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t  src_reg  = ((opcode >> 4) & 0x3);
   uint16_t mem_addr = 0;

   switch(src_reg)
   {
      case 0: mem_addr = cpu->AF; break;
      case 1: mem_addr = cpu->BC; break;
      case 2: mem_addr = cpu->DE; break;
      case 3: mem_addr = cpu->HL; break;
   }

   bus_write(cpu->bus, mem_addr, cpu->A);
}

/* load the immediate byte from memory into 8 bit register */
static void op_ld_r8_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t dest_reg = ((opcode >> 3) & 0x7);
   uint8_t value    = bus_read(cpu->bus, cpu->PC++);

   switch(dest_reg)
   {
      case 0: cpu->B = value; break;
      case 1: cpu->C = value; break;
      case 2: cpu->D = value; break;
      case 3: cpu->E = value; break;
      case 4: cpu->H = value; break;
      case 5: cpu->L = value; break;
      case 6: bus_write(cpu->bus, cpu->HL, value); break;  // LD [HL], n8
      case 7: cpu->A = value; break;
   }
}

static void op_ld_r8_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg  = (opcode & 0x7);
   uint8_t dest_reg = (opcode >> 3) & 0x7;
   uint8_t value    = 0;

   switch(src_reg)
   {
      case 0: value = cpu->B; break;
      case 1: value = cpu->C; break;
      case 2: value = cpu->D; break;
      case 3: value = cpu->E; break;
      case 4: value = cpu->H; break;
      case 5: value = cpu->L; break;
      case 6: value = bus_read(cpu->bus, cpu->HL); break;  // Read from [HL]
      case 7: value = cpu->A; break;
   }

   switch(dest_reg)
   {
      case 0: cpu->B = value; break;
      case 1: cpu->C = value; break;
      case 2: cpu->D = value; break;
      case 3: cpu->E = value; break;
      case 4: cpu->H = value; break;
      case 5: cpu->L = value; break;
      case 6: bus_write(cpu->bus, cpu->HL, value); break;  // Write to [HL]
      case 7: cpu->A = value; break;
   }
}

/* BLOCK 1 OPCODES */

/* BLOCK 2 OPCODES */

/* BLOCK 3 OPCODES */

static void op_unimplemented(cpu_t *cpu, uint8_t opcode)
{
   LOG_WARN("opcode 0x%0X is not implemented", opcode);

#ifdef EXIT_ON_BAD_OPCODE
   exit(-1);
#endif
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

      if(opcode_table[opcode] == NULL)
      {
         LOG_WARN("opcode 0x%0X is not implemented", opcode);
#ifdef EXIT_ON_BAD_OPCODE
         exit(-1);
#endif
      }
      else
      {
         opcode_table[opcode](cpu, opcode);
      }