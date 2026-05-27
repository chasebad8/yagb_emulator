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
#define READ_FLD(reg, pos) (((reg) >> (pos)) & 0x1)

#define ZERO_POS 7
#define WRITE_Z(reg, val) (WRITE_FLD((reg), (val), ZERO_POS))
#define READ_Z(reg) (READ_FLD((reg), ZERO_POS))

#define SUBTRACT_POS 6
#define WRITE_N(reg, val) (WRITE_FLD((reg), (val), SUBTRACT_POS))
#define READ_N(reg) (READ_FLD((reg), SUBTRACT_POS))

#define HALF_CARRY_POS 5
#define WRITE_H(reg, val) (WRITE_FLD((reg), (val), HALF_CARRY_POS))
#define READ_H(reg) (READ_FLD((reg), HALF_CARRY_POS))

#define CARRY_POS 4
#define WRITE_C(reg, val) (WRITE_FLD((reg), (val), CARRY_POS))
#define READ_C(reg) (READ_FLD((reg), CARRY_POS))

/**
 * @brief read from a 8 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @return uint8_t
 */
static inline uint8_t read_r8(cpu_t *cpu, uint8_t reg_idx)
{
   if(reg_idx > REG_8_MAX)
   {
      LOG_ERROR("unexpected 8 bit cpu register index, %d", reg_idx);
      exit(-1);
   }
   else
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
   if(reg_idx > REG_8_MAX)
   {
      LOG_ERROR("unexpected 8 bit cpu register index, %d", reg_idx);
      exit(-1);
   }
   else
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
   if(reg_idx > REG_16_MAX)
   {
      LOG_ERROR("unexpected 16 bit cpu register index, %d", reg_idx);
      exit(-1);
   }
   else
   {
      switch(reg_idx)
      {
         case REG_BC: return cpu->BC; break;
         case REG_DE: return cpu->DE; break;
         case REG_HL: return cpu->HL; break;
         case REG_SP: return cpu->SP; break;
      }
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
   if(reg_idx > REG_16_MAX)
   {
      LOG_ERROR("unexpected 16 bit cpu register index, %d", reg_idx);
      exit(-1);
   }
   else
   {
      switch(reg_idx)
      {
         case REG_BC: cpu->BC = value; break;
         case REG_DE: cpu->DE = value; break;
         case REG_HL: cpu->HL = value; break;
         case REG_SP: cpu->SP = value; break;
      }
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

   if(reg_idx > MEM_16_MAX)
   {
      LOG_ERROR("unexpected 16 bit cpu register index, %d", reg_idx);
      exit(-1);
   }
   else
   {
      switch(reg_idx)
      {
         case MEM_BC:     addr = cpu->BC;   break;
         case MEM_DE:     addr = cpu->DE;   break;
         case MEM_HL_ADD: addr = cpu->HL++; break;
         case MEM_HL_SUB: addr = cpu->HL--; break;
      }

      return bus_read(cpu->bus, addr);
   }

   return 0;
}

/**
 * @brief write to a memory address that is pointed to by
 *        a 16 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @param value
 */
static inline void write_m16(cpu_t *cpu, uint8_t reg_idx, uint8_t value)
{
   uint8_t addr = 0;

   if(reg_idx > MEM_16_MAX)
   {
      LOG_ERROR("unexpected 16 bit cpu register index, %d", reg_idx);
      exit(-1);
   }
   else
   {
      switch(reg_idx)
      {
         case MEM_BC:     addr = cpu->BC;   break;
         case MEM_DE:     addr = cpu->DE;   break;
         case MEM_HL_ADD: addr = cpu->HL++; break;
         case MEM_HL_SUB: addr = cpu->HL--; break;
      }

      bus_write(cpu->bus, addr, value);
   }
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

/**********************************************************
                     BLOCK 0 OPCODES
***********************************************************/

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

   r16_idx_e dest_reg = ((opcode >> 4) & 0x3);

   write_r16(cpu, dest_reg, value);
}

/**
 * @brief write to memory address pointed to by a 16 bit
 *        register with 8 bit value
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_m16_a(cpu_t *cpu, uint8_t opcode)
{
   m16_idx_e src_reg = ((opcode >> 4) & 0x3);

   write_m16(cpu, src_reg, cpu->A);
}

/**
 * @brief load cpu register A with the value from
 *        the address pointed to in m16
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_a_m16(cpu_t *cpu, uint8_t opcode)
{
   r16_idx_e src_reg = ((opcode >> 4) & 0x3);

   cpu->A = read_m16(cpu, src_reg);
}

/**
 * @brief load the current stack pointer value
 *        into the memory address given by the
 *        next two immediate bytes
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_mi16_sp(cpu_t *cpu, uint8_t opcode)
{
   /* read the address from the next 2 immediate bytes (little endian) */
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t addr      = ((high_byte << 8) | low_byte);

   /* write SP to memory (little endian) */
   bus_write(cpu->bus, addr,   (cpu->SP & 0xFF));
   bus_write(cpu->bus, addr+1, (cpu->SP >> 8) & 0xFF);
}

/**
 * @brief increment the value in register r16 by 1
 *
 * @param cpu
 * @param opcode
 */
static void op_inc_r16(cpu_t *cpu, uint8_t opcode)
{
   r16_idx_e src_reg = (opcode >> 4) & 0x3;
   uint16_t  value   = read_r16(cpu, src_reg);

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
   r16_idx_e src_reg = (opcode >> 4) & 0x3;
   uint16_t  value   = read_r16(cpu, src_reg);

   write_r16(cpu, src_reg, --value);
}

/**
 * @brief add the value in r16 to HL
 *
 * @param cpu
 * @param opcode
 */
static void op_add_hl_r16(cpu_t *cpu, uint8_t opcode)
{
   r16_idx_e src_reg  = (opcode >> 4) & 0x3;
   uint16_t  src_val  = read_r16(cpu, src_reg);
   uint16_t  dest_val = cpu->HL;

   /* check half-carry (carry from bit 11 to bit 12) */
   uint16_t half_carry = ((dest_val & 0x0FFF) + (src_val & 0x0FFF)) & 0x1000;

   uint32_t result = (uint32_t)dest_val + (uint32_t)src_val;
   uint16_t final_result = (uint16_t)result;

   cpu->HL = final_result;

   /* Set flags */
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (half_carry != 0));
   WRITE_C(cpu->F, (result > 0xFFFF));
}

/**
 * @brief increment the value in register r8 by 1.
 *
 * @param cpu
 * @param opcode
 */
static void op_inc_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg = ((opcode >> 3) & 0x7);
   uint8_t  val = read_r8(cpu, src_reg);

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
   r8_idx_e src_reg = ((opcode >> 3) & 0x7);
   uint8_t  val = read_r8(cpu, src_reg);

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
   r8_idx_e dest_reg = ((opcode >> 3) & 0x7);
   uint8_t  value = bus_read(cpu->bus, cpu->PC++);

   write_r8(cpu, dest_reg, value);
}

/**
 * @brief rotate register A left by 1 and save the
 *        value of the bit that was rotated out
 *
 * @param cpu
 * @param opcode
 */
static void op_rlca(cpu_t *cpu, uint8_t opcode)
{
   /* save the bit being lost after shifting by 7 bits */
   uint8_t carry_bit = ((cpu->A >> 7) & 0x1);

   /* shift the reg by 1 and or back in carry */
   cpu->A = ((cpu->A << 1) | carry_bit);

   WRITE_Z(cpu->F, 0);
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, carry_bit);
}

/**
 * @brief rotate register A right by 1 and save the
 *        value of the bit that was rotated out
 *
 * @param cpu
 * @param opcode
 */
static void op_rrca(cpu_t *cpu, uint8_t opcode)
{
   /* save the bit being lost */
   uint8_t carry_bit = (cpu->A & 0x1);

   /* shift the reg by 1 and or back in carry after shifting */
   cpu->A = ((cpu->A >> 1) | (carry_bit << 7));

   WRITE_Z(cpu->F, 0);
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, carry_bit);
}

/**
 * @brief rotate register A left by 1, setting the
 *        carry flag to the shifted out bit and setting
 *        the shifted in bit to the old carry flag value.
 *
 * @param cpu
 * @param opcode
 */
static void op_rla(cpu_t *cpu, uint8_t opcode)
{
   uint8_t carry_bit = ((cpu->A >> 7) & 1);

   cpu->A = ((cpu->A << 1) | READ_FLD(cpu->F, CARRY_POS));

   WRITE_Z(cpu->F, 0);
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, carry_bit);
}

/**
 * @brief rotate register A right by 1 and save the
 *        value of the bit that was rotated out
 *
 * @param cpu
 * @param opcode
 */
static void op_rra(cpu_t *cpu, uint8_t opcode)
{
   /* save the bit being lost */
   uint8_t carry_bit = (cpu->A & 0x1);

   /* shift the reg by 1 and or back in carry after shifting */
   cpu->A = ((cpu->A >> 1) | (READ_C(cpu->F) << 7));

   WRITE_Z(cpu->F, 0);
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, carry_bit);
}

/**
 * @brief relative jump to address n16.
 *        the target address n16 is encoded as a signed
 *        8-bit offset from the address immediately following
 *        the JR instruction, so it must be between -128 and
 *        127 bytes away
 *
 * @param cpu
 * @param opcode
 */
static void op_jr_e8(cpu_t *cpu, uint8_t opcode)
{
   int8_t offset = bus_read(cpu->bus, cpu->PC++);

   if ((cpu->PC + offset) < 0)
   {
      LOG_ERROR("invalid jump attempted, addr will underflow 0x%0X", (cpu->PC + offset));
      exit(-1);
   }
   else
   {
      cpu->PC += offset;
   }
}

/**
 * @brief relative jump to address n16 if a condition is met.
 *        the target address n16 is encoded as a signed
 *        8-bit offset from the address immediately following
 *        the JR instruction, so it must be between -128 and
 *        127 bytes away
 *
 * @param cpu
 * @param opcode
 */
static void op_jr_cc_e8(cpu_t *cpu, uint8_t opcode)
{
   int8_t  offset = bus_read(cpu->bus, cpu->PC++);
   uint8_t flag   = (opcode >> 3) & 0x3;

   if ((cpu->PC + offset) < 0)
   {
      LOG_ERROR("invalid jump attempted, addr will underflow 0x%0X", (cpu->PC + offset));
      exit(-1);
   }
   else if ((offset == 0) && (READ_N(cpu->F) == false))
   {
      cpu->PC += offset;
   }
   else if ((offset == 1) && (READ_N(cpu->F) == true))
   {
      cpu->PC += offset;
   }
   else if ((offset == 2) && (READ_C(cpu->F) == false))
   {
      cpu->PC += offset;
   }
   else if ((offset == 3) && (READ_C(cpu->F) == true))
   {
      cpu->PC += offset;
   }
}

/**
 * @brief bitwise NOT
 *
 * @param cpu
 * @param opcode
 */
static void op_cpl(cpu_t *cpu, uint8_t opcode)
{
   cpu->A = ~cpu->A;

   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, 1);
}

/**
 * @brief SET carry flag
 *
 * @param cpu
 * @param opcode
 */
static void op_scf(cpu_t *cpu, uint8_t opcode)
{
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, 1);
}

/**
 * @brief NOT carry flag
 *
 * @param cpu
 * @param opcode
 */
static void op_ccf(cpu_t *cpu, uint8_t opcode)
{
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, ~READ_C(cpu->F));
}

/**
 * @brief for now just end program
 *
 * @param cpu
 * @param opcode
 */
static void op_stop(cpu_t *cpu, uint8_t opcode)
{
   LOG_INFO("opcode %0X STOP ... exiting", opcode);
}

/**********************************************************
                     BLOCK 1 OPCODES
             8-bit register-to-register loads
***********************************************************/

/**
 * @brief load r8 CPU register with value from
 *        other r8 CPU register
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_r8_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg  = (opcode & 0x7);
   r8_idx_e dest_reg = (opcode >> 3) & 0x7;

   write_r8(cpu, dest_reg, read_r8(cpu, src_reg));
}

/**********************************************************
                     BLOCK 2 OPCODES
                     8-bit arithmetic
***********************************************************/

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_add_r8_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg  = (opcode & 0x7);
   r8_idx_e dest_reg = (opcode >> 3) & 0x7;
   uint8_t  src_val  = read_r8(cpu, src_reg);
   uint8_t  dest_val = read_r8(cpu, dest_reg);

   /*
    *  half carry is based on the 4 bits overflowing into the 5th bit.
    *  example: 0x0100 1111 + 1 becomes
    *           0x0101 0000 half carry = TRUE!
    */
   uint8_t half_carry = ((dest_val & 0x0F) + (src_val & 0x0F)) & 0x10;

   /* Perform addition with carry detection */
   uint16_t result = (uint16_t)dest_val + (uint16_t)src_val;
   uint8_t  final_result = (uint8_t)result;

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
   r8_idx_e src_reg  = (opcode & 0x7);
   r8_idx_e dest_reg = (opcode >> 3) & 0x7;
   uint8_t  carry    = (cpu->F >> CARRY_POS) & 0x1;
   uint8_t  src_val  = read_r8(cpu, src_reg);
   uint8_t  dest_val = read_r8(cpu, dest_reg);

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

/**********************************************************
                     BLOCK 3 OPCODES
***********************************************************/

/**
 * @brief opcode function pointer array
 *        this array can be a global as it is read only
 *
 */
static const opcode_handler_t opcode_table[OP_MAX] =
{
   op_nop,        op_ld_r16_i16, op_ld_m16_a,  op_inc_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  op_rlca,
   op_ld_mi16_sp, op_add_hl_r16, op_ld_a_m16,  op_dec_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  op_rrca,

   op_stop,       op_ld_r16_i16, op_ld_m16_a,  op_inc_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  op_rla,
   op_jr_e8,      op_add_hl_r16, op_ld_a_m16,  op_dec_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  op_rra,

   op_jr_cc_e8,   op_ld_r16_i16, op_ld_m16_a,  op_inc_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  TODO,
   op_jr_cc_e8,   op_add_hl_r16, op_ld_a_m16,  op_dec_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  op_cpl,

   op_jr_cc_e8,   op_ld_r16_i16, op_ld_m16_a,  op_inc_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  op_scf,
   op_jr_cc_e8,   op_add_hl_r16, op_ld_a_m16,  op_dec_r16,   op_inc_r8,    op_dec_r8,    op_ld_r8_i8,  op_ccf,

   op_ld_r8_r8,   op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,   op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,   op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,   op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,   op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,   op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,   op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  TODO,         op_ld_r8_r8,
   op_ld_r8_r8,   op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_add_r8_r8,  op_add_r8_r8,  op_add_r8_r8, op_add_r8_r8, op_add_r8_r8, op_add_r8_r8, op_add_r8_r8, op_add_r8_r8,
   op_adc_r8_r8,  op_adc_r8_r8,  op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8, op_adc_r8_r8,

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

/**
 * @brief
 *
 * @param cpu
 * @param bus
 */
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

/**
 * @brief
 *
 * @param cpu
 */
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
