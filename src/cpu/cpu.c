/**
 * @file cpu.c
 * @brief Game Boy CPU emulation implementation
 *
 * Implements the central processing unit (CPU) for the YAGB Game Boy emulator.
 * Contains opcode handlers for 8-bit and 16-bit arithmetic operations, load/store
 * instructions, and increment/decrement operations. Provides helper functions for
 * reading and writing 8-bit and 16-bit registers with proper memory address handling.
 * Includes proper flag handling (Z, N, H, C) for ALU operations.
 *
 * @author Chase Badalato
 * @date 2026-05-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cpu.h"
#include "cpu_opcodes.h"
#include "common/logging.h"

#define SET_FLD(reg, pos) ((reg) |=  (1 << (pos)))
#define CLR_FLD(reg, pos) ((reg) &= ~(1 << (pos)))
#define WRITE_FLD(reg, val, pos) (((val) == 1) ? SET_FLD((reg),(pos)): CLR_FLD((reg),(pos)))

/**
 *  half carry is based on the 4 bits overflowing into the 5th bit.
 *  example: 0x0100 1111 + 1 becomes
 *           0x0101 0000 half carry = TRUE!
 */
#define HALF_CARRY_POS 1
#define WRITE_H(reg, val) (WRITE_FLD((reg), (val), HALF_CARRY_POS))

#define ZERO_POS 3
#define WRITE_Z(reg, val) (WRITE_FLD((reg), (val), ZERO_POS))

#define SUBTRACT_POS 2
#define WRITE_N(reg, val) (WRITE_FLD((reg), (val), SUBTRACT_POS))

#define CARRY_POS 0
#define WRITE_C(reg, val) (WRITE_FLD((reg), (val), CARRY_POS))

/**
 * @brief read from a 8 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @return uint8_t
 */
static inline uint8_t read_r8(cpu_t *cpu, uint8_t reg_idx)
{
   switch(reg_idx)
   {
      case REG_B: return cpu->B;
      case REG_C: return cpu->C;
      case REG_D: return cpu->D;
      case REG_E: return cpu->E;
      case REG_H: return cpu->H;
      case REG_L: return cpu->L;
      case REG_HL_MEM: return bus_read(cpu->bus, cpu->HL);
      case REG_A: return cpu->A;
   }
   return 0;
}

/**
 * @brief write to a 8 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @param value
 */
static inline void write_r8(cpu_t *cpu, uint8_t reg_idx, uint8_t value)
{
   switch(reg_idx)
   {
      case REG_B: cpu->B = value; break;
      case REG_C: cpu->C = value; break;
      case REG_D: cpu->D = value; break;
      case REG_E: cpu->E = value; break;
      case REG_H: cpu->H = value; break;
      case REG_L: cpu->L = value; break;
      case REG_HL_MEM: bus_write(cpu->bus, cpu->HL, value); break;
      case REG_A: cpu->A = value; break;
   }
}

/**
 * @brief read from a 16 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @return uint16_t
 */
static inline uint16_t read_r16(cpu_t *cpu, uint8_t reg_idx)
{
   switch(reg_idx)
   {
      case REG_BC: return cpu->BC; break;
      case REG_DE: return cpu->DE; break;
      case REG_HL: return cpu->HL; break;
      case REG_SP: return cpu->SP; break;
   }
   return 0;
}

/**
 * @brief write to a 16 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @param value
 */
static inline void write_r16(cpu_t *cpu, uint8_t reg_idx, uint16_t value)
{
   switch(reg_idx)
   {
      case REG_BC: cpu->BC = value; break;
      case REG_DE: cpu->DE = value; break;
      case REG_HL: cpu->HL = value; break;
      case REG_SP: cpu->SP = value; break;
   }
}

/**
 * @brief read from a memory address that is pointed to by
 *        a 16 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @return uint8_t
 */
static inline uint8_t read_m16(cpu_t *cpu, uint8_t reg_idx)
{
   uint8_t addr = 0;

   switch(reg_idx)
   {
      case 0: addr = cpu->BC;   break;
      case 1: addr = cpu->DE;   break;
      case 2: addr = cpu->HL++; break;
      case 3: addr = cpu->HL--; break;
   }

   return bus_read(cpu->bus, addr);
}

/**
 * @brief write to a memory address that is pointed to by
 *        a 16 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @param value
 */
static inline void write_m16(cpu_t *cpu, uint8_t reg_idx, uint16_t value)
{
   uint8_t addr = 0;

   switch(reg_idx)
   {
      case 0: addr = cpu->BC;   break;
      case 1: addr = cpu->DE;   break;
      case 2: addr = cpu->HL++; break;
      case 3: addr = cpu->HL--; break;
   }

   bus_write(cpu->bus, addr, value);
}

/**
 * @brief opcode not yet implemented placeholder
 *
 * @param cpu
 * @param opcode
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
/**
 * @brief eat a cycle of the CPU
 *
 * @param cpu
 * @param opcode
 */
static void op_nop(cpu_t *cpu, uint8_t opcode)
{
   LOG_DEBUG("opcode %0X NOP", opcode);
}

/**
 * @brief load a 16 bit register with a 16 bit value
 *        in the next 2 contiguous bytes of memory
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_r16_i16(cpu_t *cpu, uint8_t opcode)
{
   /* grab the next 2 bytes in memory (little endian)*/
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t value     = ((high_byte << 8) | low_byte);

   uint8_t dest_reg = ((opcode >> 4) & 0x3);

   write_r16(cpu, dest_reg, value);
}

/**
 * @brief write to memory address pointed to by a 16 bit
 *        register with 8 bit value
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_m16_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg = ((opcode >> 4) & 0x3);

   write_m16(cpu, src_reg, cpu->A);
}

/**
 * @brief increment the value in register r16 by 1
 *
 * @param cpu
 * @param opcode
 */
static void op_inc_r16(cpu_t *cpu, uint8_t opcode)
{
   uint8_t  src_reg = (opcode >> 4) & 0x3;
   uint16_t value   = read_r16(cpu, src_reg);

   write_r16(cpu, src_reg, ++value);
}

/**
 * @brief decrement the value in register r16 by 1.
 *
 * @param cpu
 * @param opcode
 */
static void op_dec_r16(cpu_t *cpu, uint8_t opcode)
{
   uint8_t  src_reg = (opcode >> 4) & 0x3;
   uint16_t value   = read_r16(cpu, src_reg);

   write_r16(cpu, src_reg, --value);
}

/**
 * @brief increment the value in register r8 by 1.
 *
 * @param cpu
 * @param opcode
 */
static void op_inc_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg = ((opcode >> 3) & 0x7);
   uint8_t val = read_r8(cpu, src_reg);

   write_r8(cpu, src_reg, ++val);

   WRITE_Z(cpu->F, (val == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, ((val & 0x0F) == 0x0));
}

/**
 * @brief decrement the value in register r8 by 1.
 *
 * @param cpu
 * @param opcode
 */
static void op_dec_r8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t src_reg = ((opcode >> 3) & 0x7);
   uint8_t val = read_r8(cpu, src_reg);

   write_r8(cpu, src_reg, --val);

   WRITE_Z(cpu->F, (val == 0));
   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, ((val & 0x0F) == 0xF));
}

/**
 * @brief load the immediate byte from memory into
 *        a 8 bit CPU register
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_r8_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t dest_reg = ((opcode >> 3) & 0x7);
   uint8_t value = bus_read(cpu->bus, cpu->PC++);

   write_r8(cpu, dest_reg, value);
}

/*********************
    BLOCK 1 OPCODES
**********************/
/**
 * @brief load r8 CPU register with value from
 *        other r8 CPU register
 *
 * @param cpu
 * @param opcode
 */
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
/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
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

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
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

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_add_r8_i8(cpu_t *cpu, uint8_t opcode)
{

}

/**
 * @brief opcode function pointer array
 *        this array can be a global as it is read only
 *
 */
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
