#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cpu.h"
#include "cpu_opcodes.h"
#include "common/logging.h"

#define SET_FLD(reg, pos) ((reg) |=  (1 << (pos)))
#define CLR_FLD(reg, pos) ((reg) &= ~(1 << (pos)))
#define WRITE_FLD(reg, val, pos) (((val) == 1) ? SET_FLD((reg),(pos)): CLR_FLD((reg),(pos)))

#define ZERO_POS 3
#define WRITE_Z(reg, val) (WRITE_FLD((reg), (val), ZERO_POS))
#define WRITE_Z_IF_ZERO(reg, val) ((val == 0) ? WRITE_Z((reg), 1) : WRITE_Z((reg), 0))

#define SUBTRACT_POS 2
#define WRITE_N(reg, val) (WRITE_FLD((reg), (val), SUBTRACT_POS))

/* half carry is based on the 4 bits overflowing into the 5th bith.
   example: 0x0100 1111 + 1 becomes
            0x0101 0000 half carry = TRUE!
*/
#define HALF_CARRY_POS 1
#define WRITE_H(reg, val) (WRITE_FLD((reg), (val), HALF_CARRY_POS))

#define CARRY_POS 0
#define WRITE_C(reg, val) (WRITE_FLD((reg), (val), CARRY_POS))

/* Helper function to read 8-bit register by index (0-7, where 6 = [HL]) */
static inline uint8_t read_r8(cpu_t *cpu, uint8_t reg_idx)
{
   switch(reg_idx)
   {
      case 0: return cpu->B;
      case 1: return cpu->C;
      case 2: return cpu->D;
      case 3: return cpu->E;
      case 4: return cpu->H;
      case 5: return cpu->L;
      case 6: return bus_read(cpu->bus, cpu->HL);
      case 7: return cpu->A;
   }
   return 0;
}

/* Helper function to write 8-bit register by index (0-7, where 6 = [HL]) */
static inline void write_r8(cpu_t *cpu, uint8_t reg_idx, uint8_t value)
{
   switch(reg_idx)
   {
      case 0: cpu->B = value; break;
      case 1: cpu->C = value; break;
      case 2: cpu->D = value; break;
      case 3: cpu->E = value; break;
      case 4: cpu->H = value; break;
      case 5: cpu->L = value; break;
      case 6: bus_write(cpu->bus, cpu->HL, value); break;
      case 7: cpu->A = value; break;
   }
}

/*
 i8, i16 == immediate 8 or 16 (next byte or 2 from memory)
 r8, r16 == register 8 or 16  (group contiguous register if 16)
 m8, m16 == memory 8 or 16    (get value at register in memory pointed to by m8 or m16)
 opcode, dest, src
*/

static void op_unimplemented(cpu_t *cpu, uint8_t opcode)
{
   LOG_WARN("opcode 0x%0X is not implemented", opcode);

#ifdef EXIT_ON_BAD_OPCODE
   exit(-1);
#endif
}
#define TODO op_unimplemented

/*********************
    BLOCK 0 OPCODES
**********************/
static void op_nop(cpu_t *cpu, uint8_t opcode)
{
   LOG_DEBUG("opcode %0X NOP", opcode);
}

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
      case 0: cpu->BC = src_word; break;
      case 1: cpu->DE = src_word; break;
      case 2: cpu->HL = src_word; break;
      case 3: cpu->SP = src_word; break;
   }
}

/* load address pointed to by a 16 bit register with 8 bit value */
static void op_ld_m16_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t  src_reg  = ((opcode >> 4) & 0x3);
   uint16_t mem_addr = 0;

   switch(src_reg)
   {
      case 0: mem_addr = cpu->BC; break;
      case 1: mem_addr = cpu->DE; break;
      case 2: mem_addr = cpu->HL++; break;
      case 3: mem_addr = cpu->HL--; break;
   }

   bus_write(cpu->bus, mem_addr, cpu->A);
}

static void op_inc_r16(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg = (opcode >> 4) & 0x3;

   switch(src_reg)
   {
      case 0: cpu->BC++; break;
      case 1: cpu->DE++; break;
      case 2: cpu->HL++; break;
      case 3: cpu->SP++; break;

   }
}

static void op_dec_r16(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg = (opcode >> 4) & 0x3;

   switch(src_reg)
   {
      case 0: cpu->BC--; break;
      case 1: cpu->DE--; break;
      case 2: cpu->HL--; break;
      case 3: cpu->SP--; break;
   }
}

static void op_inc_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg = ((opcode >> 3) & 0x7);
   uint8_t val = read_r8(cpu, src_reg);

   write_r8(cpu, src_reg, ++val);

   WRITE_Z(cpu->F, (val == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, ((val & 0x0F) == 0x0));
}

static void op_dec_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg = ((opcode >> 3) & 0x7);
   uint8_t val = read_r8(cpu, src_reg);

   write_r8(cpu, src_reg, --val);

   WRITE_Z(cpu->F, (val == 0));
   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, ((val & 0x0F) == 0xF));
}

/* load the immediate byte from memory into 8 bit register */
static void op_ld_r8_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t dest_reg = ((opcode >> 3) & 0x7);
   uint8_t value = bus_read(cpu->bus, cpu->PC++);

   write_r8(cpu, dest_reg, value);
}

/*********************
    BLOCK 1 OPCODES
**********************/
static void op_ld_r8_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg  = (opcode & 0x7);
   uint8_t dest_reg = (opcode >> 3) & 0x7;

   write_r8(cpu, dest_reg, read_r8(cpu, src_reg));
}

/*********************
    BLOCK 2 OPCODES
    8-bit arithmetic
**********************/
static void op_add_r8_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg  = (opcode & 0x7);
   uint8_t dest_reg = (opcode >> 3) & 0x7;
   uint8_t src_val  = read_r8(cpu, src_reg);
   uint8_t dest_val = read_r8(cpu, dest_reg);

   /* Check half-carry (carry from bit 3 to bit 4) */
   uint8_t half_carry = ((dest_val & 0x0F) + (src_val & 0x0F)) & 0x10;

   /* Perform addition with carry detection */
   uint16_t result = (uint16_t)dest_val + (uint16_t)src_val;
   uint8_t final_result = (uint8_t)result;

   /* Write result back to destination */
   write_r8(cpu, dest_reg, final_result);

   /* Set flags */
   WRITE_Z(cpu->F, (final_result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (half_carry != 0));
   WRITE_C(cpu->F, (result > 0xFF));
}

static void op_adc_r8_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg  = (opcode & 0x7);
   uint8_t dest_reg = (opcode >> 3) & 0x7;
   uint8_t carry    = (cpu->F >> CARRY_POS) & 0x1;
   uint8_t src_val  = read_r8(cpu, src_reg);
   uint8_t dest_val = read_r8(cpu, dest_reg);

   /* check half-carry (carry from bit 3 to bit 4) including carry flag */
   uint8_t half_carry = ((dest_val & 0x0F) + (src_val & 0x0F) + carry) & 0x10;

   uint16_t result = (uint16_t)dest_val + (uint16_t)src_val + (uint16_t)carry;
   uint8_t  final_result = (uint8_t)result;

   write_r8(cpu, dest_reg, final_result);

   WRITE_Z(cpu->F, (final_result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (half_carry != 0));
   WRITE_C(cpu->F, (result > 0xFF));
}

static void op_add_r8_i8(cpu_t *cpu, uint8_t opcode)
{

}


/* this can be global, its read only */
static const opcode_handler_t opcode_table[OP_MAX] =
{
   op_nop,      op_ld_r16_i16, op_ld_m16_r8, op_inc_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,
   TODO,        TODO,          TODO,         op_dec_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,

   TODO,        op_ld_r16_i16, op_ld_m16_r8, op_inc_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,
   TODO,        TODO,          TODO,         op_dec_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,

   TODO,        op_ld_r16_i16, op_ld_m16_r8, op_inc_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,
   TODO,        TODO,          TODO,         op_dec_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,

   TODO,        op_ld_r16_i16, op_ld_m16_r8, op_inc_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,
   TODO,        TODO,          TODO,         op_dec_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,

   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  TODO,         op_ld_r8_r8,
   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_add_r8_r8, op_add_r8_r8, op_add_r8_r8, op_add_r8_r8, op_add_r8_r8, op_add_r8_r8, op_add_r8_r8, op_add_r8_r8,
   op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8,

   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,
   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,

   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,
   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,

   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,
   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,

   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,
   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,

   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,
   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,

   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,
   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,

   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,
   TODO, TODO, TODO, TODO, TODO, TODO, TODO, TODO,
};

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
      uint8_t opcode = bus_read(cpu->bus, cpu->PC++);
      opcode_table[opcode](cpu, opcode);
   }
}
