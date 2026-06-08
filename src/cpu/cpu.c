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
 * @brief to string function for r8
 *        for logging
 *
 * @param reg_idx
 * @return const char*
 */
static const char* r8_to_string(r8_idx_e reg_idx)
{
   switch(reg_idx)
   {
      case REG_B: return "B";
      case REG_C: return "C";
      case REG_D: return "D";
      case REG_E: return "E";
      case REG_H: return "H";
      case REG_L: return "L";
      case REG_HL_MEM: return "[HL]";
      case REG_A: return "A";
   }
}

/**
 * @brief read from a 8 bit CPU register
 *
 * @param cpu
 * @param reg_idx
 * @return uint8_t
 */
static inline uint8_t read_r8(cpu_t *cpu, r8_idx_e reg_idx)
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

static const char* r16_to_string(r16_idx_e reg_idx)
{
   switch(reg_idx)
   {
      case REG_BC: return "BC"; break;
      case REG_DE: return "DE"; break;
      case REG_HL: return "HL"; break;
      case REG_SP: return "SP"; break;
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
         case REG_BC: return (cpu->B << 8) | cpu->C; break;
         case REG_DE: return (cpu->D << 8) | cpu->E; break;
         case REG_HL: return (cpu->H << 8) | cpu->L; break;
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
         case REG_BC: cpu->B = (value >> 8); cpu->C = (value & 0xFF); break;
         case REG_DE: cpu->D = (value >> 8); cpu->E = (value & 0xFF); break;
         case REG_HL: cpu->H = (value >> 8); cpu->L = (value & 0xFF); break;
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
   uint16_t addr = 0;

   if(reg_idx > MEM_16_MAX)
   {
      LOG_ERROR("unexpected 16 bit cpu register index, %d", reg_idx);
      exit(-1);
   }
   else
   {
      switch(reg_idx)
      {
         case MEM_BC:     addr = (cpu->B << 8) | cpu->C;   break;
         case MEM_DE:     addr = (cpu->D << 8) | cpu->E;   break;
         case MEM_HL_ADD: addr = (cpu->H << 8) | cpu->L; cpu->HL++; break;
         case MEM_HL_SUB: addr = (cpu->H << 8) | cpu->L; cpu->HL--; break;
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
   uint16_t addr = 0;

   if(reg_idx > MEM_16_MAX)
   {
      LOG_ERROR("unexpected 16 bit cpu register index, %d", reg_idx);
      exit(-1);
   }
   else
   {
      switch(reg_idx)
      {
         case MEM_BC:     addr = (cpu->B << 8) | cpu->C;   break;
         case MEM_DE:     addr = (cpu->D << 8) | cpu->E;   break;
         case MEM_HL_ADD: addr = (cpu->H << 8) | cpu->L; cpu->HL++; break;
         case MEM_HL_SUB: addr = (cpu->H << 8) | cpu->L; cpu->HL--; break;
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

/**
 * @brief opcode not yet implemented placeholder
 *
 * @param cpu
 * @param opcode
 */
static void op_invalid(cpu_t *cpu, uint8_t opcode)
{
   LOG_ERROR("opcode 0x%0X is not a valid opcode", opcode);

#ifdef EXIT_ON_BAD_OPCODE
   exit(-1);
#endif
}
#define INVALID op_invalid
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
   LOG_OPCODE("NOP");
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

   LOG_OPCODE("LD %s, 0x%04X", r16_to_string(dest_reg), value);
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

   LOG_OPCODE("LD [%s], A", r16_to_string((r16_idx_e)src_reg));
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

   LOG_OPCODE("LD A, [%s]", r16_to_string((r16_idx_e)src_reg));
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

   LOG_OPCODE("LD [0x%04X], SP", addr);
}

/**
 * @brief load the current stack pointer value
 *        into the memory address given by the
 *        next two immediate bytes
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_mi16_a(cpu_t *cpu, uint8_t opcode)
{
   /* read the address from the next 2 immediate bytes (little endian) */
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t addr      = ((high_byte << 8) | low_byte);

   bus_write(cpu->bus, addr, cpu->A);

   LOG_OPCODE("LD [0x%04X], A", addr);
}

/**
 * @brief load the current stack pointer value
 *        into the memory address given by the
 *        next two immediate bytes
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_a_mi16(cpu_t *cpu, uint8_t opcode)
{
   /* read the address from the next 2 immediate bytes (little endian) */
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t addr      = ((high_byte << 8) | low_byte);

   cpu->A = bus_read(cpu->bus, addr);

   LOG_OPCODE("LD A, [0x%04X]", addr);
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

   LOG_OPCODE("INC %s", r16_to_string(src_reg));
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

   LOG_OPCODE("DEC %s", r16_to_string(src_reg));
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

   LOG_OPCODE("ADD HL, %s", r16_to_string(src_reg));
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

   LOG_OPCODE("INC %s", r8_to_string(src_reg));
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

   LOG_OPCODE("DEC %s", r8_to_string(src_reg));
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

   LOG_OPCODE("LD %s, 0x%02X", r8_to_string(dest_reg), value);
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

   LOG_OPCODE("RLCA");
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

   LOG_OPCODE("RRCA");
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

   LOG_OPCODE("RLA");
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

   LOG_OPCODE("RRA");
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

   LOG_OPCODE("JR 0x%04X", cpu->PC);
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
   else if ((flag == 0) && (READ_Z(cpu->F) == false))
   {
      cpu->PC += offset;
   }
   else if ((flag == 1) && (READ_Z(cpu->F) == true))
   {
      cpu->PC += offset;
   }
   else if ((flag == 2) && (READ_C(cpu->F) == false))
   {
      cpu->PC += offset;
   }
   else if ((flag == 3) && (READ_C(cpu->F) == true))
   {
      cpu->PC += offset;
   }

   LOG_OPCODE("JR CC, 0x%04X", cpu->PC);
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

   LOG_OPCODE("CPL");
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

   LOG_OPCODE("SCF");
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
   WRITE_C(cpu->F, !READ_C(cpu->F));

   LOG_OPCODE("CCF");
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

   LOG_OPCODE("STOP");
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

   LOG_OPCODE("LD %s, %s", r8_to_string(dest_reg), r8_to_string(src_reg));
}

/**********************************************************
                     BLOCK 2 OPCODES
                     8-bit arithmetic
***********************************************************/

/**
 * @brief add two values from r8 src and dest and save
 *        result in dest
 *
 * @param cpu
 * @param opcode
 */
static void op_add_a_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg  = (opcode & 0x7);
   uint8_t  src_val  = read_r8(cpu, src_reg);
   uint8_t  dest_val = cpu->A;

   /*
    *  half carry is based on the 4 bits overflowing into the 5th bit.
    *  example: 0x0100 1111 + 1 becomes
    *           0x0101 0000 half carry = TRUE!
    */
   uint8_t  half_carry = ((dest_val & 0x0F) + (src_val & 0x0F)) & 0x10;
   uint16_t result = (uint16_t)dest_val + (uint16_t)src_val;
   uint8_t  final_result = (uint8_t)result;

   cpu->A = result;

   /* Set flags */
   WRITE_Z(cpu->F, (final_result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (half_carry != 0));
   WRITE_C(cpu->F, (result > 0xFF));

   LOG_OPCODE("ADD A, %s", r8_to_string(src_reg));
}

/**
 * @brief add with current carry flag
 *
 * @param cpu
 * @param opcode
 */
static void op_adc_a_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg  = (opcode & 0x7);
   uint8_t  carry    = (cpu->F >> CARRY_POS) & 0x1;
   uint8_t  src_val  = read_r8(cpu, src_reg);
   uint8_t  dest_val = cpu->A;

   /* check half-carry (carry from bit 3 to bit 4) including carry flag */
   uint8_t  half_carry = ((dest_val & 0x0F) + (src_val & 0x0F) + carry) & 0x10;
   uint16_t result = (uint16_t)dest_val + (uint16_t)src_val + (uint16_t)carry;
   uint8_t  final_result = (uint8_t)result;

   cpu->A = result;

   WRITE_Z(cpu->F, (final_result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (half_carry != 0));
   WRITE_C(cpu->F, (result > 0xFF));

   LOG_OPCODE("ADC A, %s", r8_to_string(src_reg));
}

/**
 * @brief subtract r8 from r8 and save result in dest
 *
 * @param cpu
 * @param opcode
 */
static void op_sub_a_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg  = (opcode & 0x7);
   uint8_t  src_val  = read_r8(cpu, src_reg);
   uint8_t  dest_val = cpu->A;

   /* check half-carry (borrow from bit 4) */
   uint8_t half_carry = (dest_val & 0x0F) < (src_val & 0x0F);
   uint8_t result = dest_val - src_val;

   cpu->A = result;

   /* Set flags */
   WRITE_Z(cpu->F, (result == 0));
   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, half_carry);
   WRITE_C(cpu->F, (dest_val < src_val));

   LOG_OPCODE("SUB A, %s", r8_to_string(src_reg));
}

/**
 * @brief subtract with current carry flag
 *
 * @param cpu
 * @param opcode
 */
static void op_sbc_a_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg  = (opcode & 0x7);
   r8_idx_e dest_reg = REG_A;
   uint8_t  carry    = (cpu->F >> CARRY_POS) & 0x1;
   uint8_t  src_val  = read_r8(cpu, src_reg);
   uint8_t  dest_val = read_r8(cpu, dest_reg);

   /* check half-carry (borrow from bit 4) including carry flag */
   uint8_t half_carry = (dest_val & 0x0F) < ((src_val & 0x0F) + carry);
   uint8_t result = dest_val - src_val - carry;

   cpu->A = result;

   /* Set flags */
   WRITE_Z(cpu->F, (result == 0));
   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, half_carry);
   WRITE_C(cpu->F, ((uint16_t)dest_val < ((uint16_t)src_val + (uint16_t)carry)));

   LOG_OPCODE("SBC A, %s", r8_to_string(src_reg));
}

/**
 * @brief AND A with r8
 *
 * @param cpu
 * @param opcode
 */
static void op_and_a_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg = opcode & 0x7;
   uint8_t  value = (cpu->A & read_r8(cpu, src_reg));

   cpu->A = value;

   WRITE_Z(cpu->F, (value == 0x0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 1);
   WRITE_C(cpu->F, 0);

   LOG_OPCODE("AND A, %s", r8_to_string(src_reg));
}

/**
 * @brief XOR A with r8
 *
 * @param cpu
 * @param opcode
 */
static void op_xor_a_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg = opcode & 0x7;
   uint8_t  value = (cpu->A ^ read_r8(cpu, src_reg));

   cpu->A = value;

   WRITE_Z(cpu->F, (value == 0x0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, 0);

   LOG_OPCODE("XOR A, %s", r8_to_string(src_reg));
}

/**
 * @brief OR A with r8
 *
 * @param cpu
 * @param opcode
 */
static void op_or_a_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg = opcode & 0x7;
   uint8_t  value = (cpu->A | read_r8(cpu, src_reg));

   cpu->A = value;

   WRITE_Z(cpu->F, (value == 0x0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, 0);

   LOG_OPCODE("OR A, %s", r8_to_string(src_reg));
}

/**
 * @brief compare the value of A and r8.
 *        subtract r8 from a to set flags but do
 *        not retain the result
 *
 * @param cpu
 * @param opcode
 */
static void op_cp_a_r8(cpu_t *cpu, uint8_t opcode)
{
   r8_idx_e src_reg = opcode & 0x7;
   uint8_t  value = read_r8(cpu, src_reg);

   WRITE_Z(cpu->F, (cpu->A == value));
   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, (cpu->A & 0xF) < (value & 0xF));
   WRITE_C(cpu->F, (cpu->A < value));

   LOG_OPCODE("CP A, %s", r8_to_string(src_reg));
}

/**********************************************************
                     BLOCK 3 OPCODES
***********************************************************/

/**
 * @brief add immediate 8-bit value to register A
 *
 * @param cpu
 * @param opcode
 */
static void op_add_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t imm_val  = bus_read(cpu->bus, cpu->PC++);
   uint8_t dest_val = cpu->A;

   /* check half-carry (carry from bit 3 to bit 4) */
   uint8_t half_carry = ((dest_val & 0x0F) + (imm_val & 0x0F)) & 0x10;

   uint16_t result = (uint16_t)dest_val + (uint16_t)imm_val;
   uint8_t  final_result = (uint8_t)result;

   cpu->A = final_result;

   /* Set flags */
   WRITE_Z(cpu->F, (final_result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (half_carry != 0));
   WRITE_C(cpu->F, (result > 0xFF));

   LOG_OPCODE("ADD A, 0x%02X", imm_val);
}

/**
 * @brief add signed immediate 8-bit value to the stack pointer
 *
 * @param cpu
 * @param opcode
 */
static void op_add_sp_i8(cpu_t *cpu, uint8_t opcode)
{
   int8_t  imm_val = (int8_t)bus_read(cpu->bus, cpu->PC++);
   uint16_t orig_sp = cpu->SP;

   uint16_t result = cpu->SP + imm_val;

   WRITE_Z(cpu->F, 0);
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (((orig_sp & 0x0F) + ((uint16_t)imm_val & 0x0F)) > 0x0F));
   WRITE_C(cpu->F, (((orig_sp & 0xFF) + ((uint16_t)imm_val & 0xFF)) > 0xFF));

   cpu->SP = result;

   LOG_OPCODE("ADD SP, 0x%02X", (uint8_t)imm_val);
}

/**
 * @brief load HL with SP plus signed immediate offset
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_hl_sp_i8(cpu_t *cpu, uint8_t opcode)
{
   int8_t  imm_val = (int8_t)bus_read(cpu->bus, cpu->PC++);
   uint16_t orig_sp = cpu->SP;

   uint16_t result = cpu->SP + imm_val;

   WRITE_Z(cpu->F, 0);
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (((orig_sp & 0x0F) + ((uint16_t)imm_val & 0x0F)) > 0x0F));
   WRITE_C(cpu->F, (((orig_sp & 0xFF) + ((uint16_t)imm_val & 0xFF)) > 0xFF));

   cpu->HL = result;

   LOG_OPCODE("LD HL, SP+0x%02X", (uint8_t)imm_val);
}

/**
 * @brief load SP from HL
 *
 * @param cpu
 * @param opcode
 */
static void op_ld_sp_hl(cpu_t *cpu, uint8_t opcode)
{
   cpu->SP = cpu->HL;

   LOG_OPCODE("LD SP, HL");
}

/**
 * @brief add immediate 8-bit value to register A
 *        with carry
 *
 * @param cpu
 * @param opcode
 */
static void op_adc_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t imm_val  = bus_read(cpu->bus, cpu->PC++);
   uint8_t carry    = (cpu->F >> CARRY_POS) & 0x1;
   uint8_t dest_val = cpu->A;

   /* check half-carry (carry from bit 3 to bit 4) including carry flag */
   uint8_t half_carry = ((dest_val & 0x0F) + (imm_val & 0x0F) + carry) & 0x10;

   uint16_t result = (uint16_t)dest_val + (uint16_t)imm_val + (uint16_t)carry;
   uint8_t  final_result = (uint8_t)result;

   cpu->A = final_result;

   WRITE_Z(cpu->F, (final_result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, (half_carry != 0));
   WRITE_C(cpu->F, (result > 0xFF));

   LOG_OPCODE("ADC A, 0x%02X", imm_val);
}

/**
 * @brief subtract immediate 8-bit value from register A
 *
 * @param cpu
 * @param opcode
 */
static void op_sub_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t imm_val  = bus_read(cpu->bus, cpu->PC++);
   uint8_t dest_val = cpu->A;

   /* check half-carry (borrow from bit 4) */
   uint8_t half_carry = (dest_val & 0x0F) < (imm_val & 0x0F);

   uint8_t result = dest_val - imm_val;

   cpu->A = result;

   /* Set flags */
   WRITE_Z(cpu->F, (result == 0));
   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, half_carry);
   WRITE_C(cpu->F, (dest_val < imm_val));

   LOG_OPCODE("SUB A, 0x%02X", imm_val);
}

/**
 * @brief subtract immediate 8-bit value plus carry from register A
 *
 * @param cpu
 * @param opcode
 */
static void op_sbc_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t imm_val  = bus_read(cpu->bus, cpu->PC++);
   uint8_t carry    = (cpu->F >> CARRY_POS) & 0x1;
   uint8_t dest_val = cpu->A;

   /* check half-carry (borrow from bit 4) including carry flag */
   uint8_t half_carry = (dest_val & 0x0F) < ((imm_val & 0x0F) + carry);

   uint8_t result = dest_val - imm_val - carry;

   cpu->A = result;

   /* Set flags */
   WRITE_Z(cpu->F, (result == 0));
   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, half_carry);
   WRITE_C(cpu->F, (dest_val < (imm_val + carry)));

   LOG_OPCODE("SBC A, 0x%02X", imm_val);
}

/**
 * @brief AND immediate 8-bit value with register A
 *
 * @param cpu
 * @param opcode
 */
static void op_and_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t imm_val  = bus_read(cpu->bus, cpu->PC++);
   uint8_t dest_val = cpu->A;

   uint8_t result = dest_val & imm_val;

   cpu->A = result;

   /* Set flags */
   WRITE_Z(cpu->F, (result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 1);
   WRITE_C(cpu->F, 0);

   LOG_OPCODE("AND A, 0x%02X", imm_val);
}

/**
 * @brief OR immediate 8-bit value with register A
 *
 * @param cpu
 * @param opcode
 */
static void op_or_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t imm_val  = bus_read(cpu->bus, cpu->PC++);
   uint8_t dest_val = cpu->A;

   uint8_t result = dest_val | imm_val;

   cpu->A = result;

   /* Set flags */
   WRITE_Z(cpu->F, (result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, 0);

   LOG_OPCODE("OR A, 0x%02X", imm_val);
}

/**
 * @brief XOR immediate 8-bit value with register A
 *
 * @param cpu
 * @param opcode
 */
static void op_xor_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t imm_val  = bus_read(cpu->bus, cpu->PC++);
   uint8_t dest_val = cpu->A;

   uint8_t result = dest_val ^ imm_val;

   cpu->A = result;

   /* Set flags */
   WRITE_Z(cpu->F, (result == 0));
   WRITE_N(cpu->F, 0);
   WRITE_H(cpu->F, 0);
   WRITE_C(cpu->F, 0);

   LOG_OPCODE("XOR A, 0x%02X", imm_val);
}

/**
 * @brief compare immediate 8-bit value with register A
 *        (subtract but don't store result, only set flags)
 *
 * @param cpu
 * @param opcode
 */
static void op_cp_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t imm_val  = bus_read(cpu->bus, cpu->PC++);
   uint8_t dest_val = cpu->A;

   /* check half-carry (borrow from bit 4) */
   uint8_t half_carry = (dest_val & 0x0F) < (imm_val & 0x0F);

   uint8_t result = dest_val - imm_val;

   /* Set flags */
   WRITE_Z(cpu->F, (result == 0));
   WRITE_N(cpu->F, 1);
   WRITE_H(cpu->F, half_carry);
   WRITE_C(cpu->F, (dest_val < imm_val));

   LOG_OPCODE("CP A, 0x%02X", imm_val);
}

/**
 * @brief pop the top of the stack and
 *        place it into PC.
 *
 * @param cpu
 * @param opcode
 */
static void op_ret(cpu_t *cpu, uint8_t opcode)
{
   uint8_t pop_addr_low  = bus_read(cpu->bus, cpu->SP++);
   uint8_t pop_addr_high = bus_read(cpu->bus, cpu->SP++);

   cpu->PC = ((pop_addr_high << 8) | (pop_addr_low));

   LOG_OPCODE("RET");
}

/**
 * @brief pop the top of the stack and
 *        place it into PC.
 *
 * @param cpu
 * @param opcode
 */
static void op_ret_cc(cpu_t *cpu, uint8_t opcode)
{
   uint8_t pop_addr_low  = 0;
   uint8_t pop_addr_high = 0;
   uint8_t flag = (opcode >> 3) & 0x3;
   bool    ret = false;

   if ((flag == 0) && (READ_Z(cpu->F) == false))
   {
      ret = true;
   }
   else if ((flag == 1) && (READ_Z(cpu->F) == true))
   {
      ret = true;
   }
   else if ((flag == 2) && (READ_C(cpu->F) == false))
   {
      ret = true;
   }
   else if ((flag == 3) && (READ_C(cpu->F) == true))
   {
      ret = true;
   }

   if (ret == true)
   {
      pop_addr_low  = bus_read(cpu->bus, cpu->SP++);
      pop_addr_high = bus_read(cpu->bus, cpu->SP++);
      cpu->PC = ((pop_addr_high << 8) | (pop_addr_low));
   }

   LOG_OPCODE("RET CC");
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_jp_c_i16(cpu_t *cpu, uint8_t opcode)
{
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t addr      = ((high_byte << 8) | low_byte);
   uint8_t  flag      = (opcode >> 3) & 0x3;

   if ((flag == 0) && (READ_Z(cpu->F) == false))
   {
      cpu->PC = addr;
   }
   else if ((flag == 1) && (READ_Z(cpu->F) == true))
   {
      cpu->PC = addr;
   }
   else if ((flag == 2) && (READ_C(cpu->F) == false))
   {
      cpu->PC = addr;
   }
   else if ((flag == 3) && (READ_C(cpu->F) == true))
   {
      cpu->PC = addr;
   }

   LOG_OPCODE("JP CC, 0x%04X", addr);
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_jp_i16(cpu_t *cpu, uint8_t opcode)
{
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t addr      = ((high_byte << 8) | low_byte);

   cpu->PC = addr;

   LOG_OPCODE("JP 0x%04X", addr);
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_jp_hl(cpu_t *cpu, uint8_t opcode)
{
   cpu->PC = cpu->HL;

   LOG_OPCODE("JP HL");
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_call_c_i16(cpu_t *cpu, uint8_t opcode)
{
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t addr = ((high_byte << 8) | low_byte);
   uint8_t  flag = (opcode >> 3) & 0x3;
   bool     call = false;

   if ((flag == 0) && (READ_Z(cpu->F) == false))
   {
      call = true;
   }
   else if ((flag == 1) && (READ_Z(cpu->F) == true))
   {
      call = true;
   }
   else if ((flag == 2) && (READ_C(cpu->F) == false))
   {
      call = true;
   }
   else if ((flag == 3) && (READ_C(cpu->F) == true))
   {
      call = true;
   }

   if (call == true)
   {
      bus_write(cpu->bus, --cpu->SP, ((cpu->PC >> 8) & 0xFF));
      bus_write(cpu->bus, --cpu->SP, (cpu->PC & 0xFF));
      cpu->PC = addr;
   }

   LOG_OPCODE("CALL CC, 0x%04X", addr);
}

/**
 * @brief push pc+1 addr to the stack and
 *        jump to the current addr stored in
 *        pc. this essentially is calling a
 *        function.
 *
 * @param cpu
 * @param opcode
 */
static void op_call_i16(cpu_t *cpu, uint8_t opcode)
{
   uint8_t  low_byte  = bus_read(cpu->bus, cpu->PC++);
   uint8_t  high_byte = bus_read(cpu->bus, cpu->PC++);
   uint16_t addr      = ((high_byte << 8) | low_byte);

   bus_write(cpu->bus, --cpu->SP, ((cpu->PC >> 8) & 0xFF));
   bus_write(cpu->bus, --cpu->SP, (cpu->PC & 0xFF));
   cpu->PC = addr;

   LOG_OPCODE("CALL 0x%04X", addr);
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_pop(cpu_t *cpu, uint8_t opcode)
{
   uint8_t reg = (opcode >> 4) & 0x3;
   uint8_t pop_addr_low  = bus_read(cpu->bus, cpu->SP++);
   uint8_t pop_addr_high = bus_read(cpu->bus, cpu->SP++);
   uint16_t value = ((pop_addr_high << 8) | (pop_addr_low));

   switch(reg)
   {
      case 0: cpu->BC = value; break;
      case 1: cpu->DE = value; break;
      case 2: cpu->HL = value; break;
      case 3: cpu->AF = value; break;
   }

   LOG_OPCODE("POP %s", r16_to_string((r16_idx_e)reg));
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_push(cpu_t *cpu, uint8_t opcode)
{
   uint8_t reg = (opcode >> 4) & 0x3;
   uint16_t value = 0;

   switch(reg)
   {
      case 0: value = cpu->BC; break;
      case 1: value = cpu->DE; break;
      case 2: value = cpu->HL; break;
      case 3: value = cpu->AF; break;
   }

   bus_write(cpu->bus, --cpu->SP, ((value >> 8) & 0xFF));
   bus_write(cpu->bus, --cpu->SP, (value & 0xFF));

   LOG_OPCODE("PUSH %s", r16_to_string((r16_idx_e)reg));
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_ldh_c_a(cpu_t *cpu, uint8_t opcode)
{
   bus_write(cpu->bus, 0xFF00 + cpu->C, cpu->A);

   LOG_OPCODE("LDH [C], A");
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_ldh_i8_a(cpu_t *cpu, uint8_t opcode)
{
   uint8_t offset = bus_read(cpu->bus, cpu->PC++);
   bus_write(cpu->bus, 0xFF00 + offset, cpu->A);

   LOG_OPCODE("LDH [0x%02X], A", offset);
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_ldh_a_c(cpu_t *cpu, uint8_t opcode)
{
   cpu->A = bus_read(cpu->bus, 0xFF00 + cpu->C);

   LOG_OPCODE("LDH A, [C]");
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_ldh_a_i8(cpu_t *cpu, uint8_t opcode)
{
   uint8_t offset = bus_read(cpu->bus, cpu->PC++);
   cpu->A = bus_read(cpu->bus, 0xFF00 + offset);

   LOG_OPCODE("LDH A, [0x%02X]", offset);
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_ei(cpu_t *cpu, uint8_t opcode)
{
   cpu->IME = 1;

   LOG_OPCODE("EI");
}

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
static void op_di(cpu_t *cpu, uint8_t opcode)
{
   cpu->IME = 0;

   LOG_OPCODE("DI");
}

/**
 * @brief restart - push current PC to stack and
 *        jump to one of 8 fixed locations in memory.
 *        the target location is determined by the
 *        opcode divided by 8.
 *
 * @param cpu
 * @param opcode
 */
static void op_rst_tgt3(cpu_t *cpu, uint8_t opcode)
{
   uint8_t rst_addr = ((opcode >> 3) & 0x7) << 3;

   bus_write(cpu->bus, --cpu->SP, ((cpu->PC >> 8) & 0xFF));
   bus_write(cpu->bus, --cpu->SP, (cpu->PC & 0xFF));
   cpu->PC = rst_addr;

   LOG_OPCODE("RST 0x%04X", rst_addr);
}

/**
 * @brief opcode function pointer array
 *        this array can be a global as it is read only
 *
 */
static const opcode_handler_t opcode_table[OP_MAX] =
{
   op_nop,         op_ld_r16_i16, op_ld_m16_a,  op_inc_r16,   op_inc_r8,     op_dec_r8,    op_ld_r8_i8,  op_rlca,
   op_ld_mi16_sp,  op_add_hl_r16, op_ld_a_m16,  op_dec_r16,   op_inc_r8,     op_dec_r8,    op_ld_r8_i8,  op_rrca,

   op_stop,        op_ld_r16_i16, op_ld_m16_a,  op_inc_r16,   op_inc_r8,     op_dec_r8,    op_ld_r8_i8,  op_rla,
   op_jr_e8,       op_add_hl_r16, op_ld_a_m16,  op_dec_r16,   op_inc_r8,     op_dec_r8,    op_ld_r8_i8,  op_rra,

   op_jr_cc_e8,    op_ld_r16_i16, op_ld_m16_a,  op_inc_r16,   op_inc_r8,     op_dec_r8,    op_ld_r8_i8,  TODO,
   op_jr_cc_e8,    op_add_hl_r16, op_ld_a_m16,  op_dec_r16,   op_inc_r8,     op_dec_r8,    op_ld_r8_i8,  op_cpl,

   op_jr_cc_e8,    op_ld_r16_i16, op_ld_m16_a,  op_inc_r16,   op_inc_r8,     op_dec_r8,    op_ld_r8_i8,  op_scf,
   op_jr_cc_e8,    op_add_hl_r16, op_ld_a_m16,  op_dec_r16,   op_inc_r8,     op_dec_r8,    op_ld_r8_i8,  op_ccf,

   op_ld_r8_r8,    op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,    op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,    op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,    op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,    op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,
   op_ld_r8_r8,    op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_ld_r8_r8,    op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,   op_ld_r8_r8,  TODO,         op_ld_r8_r8,
   op_ld_r8_r8,    op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,   op_ld_r8_r8,  op_ld_r8_r8,  op_ld_r8_r8,

   op_add_a_r8,    op_add_a_r8,   op_add_a_r8,  op_add_a_r8,  op_add_a_r8,   op_add_a_r8,  op_add_a_r8,  op_add_a_r8,
   op_adc_a_r8,    op_adc_a_r8,   op_adc_a_r8,  op_adc_a_r8,  op_adc_a_r8,   op_adc_a_r8,  op_adc_a_r8,  op_adc_a_r8,

   op_sub_a_r8,    op_sub_a_r8,   op_sub_a_r8,  op_sub_a_r8,  op_sub_a_r8,   op_sub_a_r8,  op_sub_a_r8,  op_sub_a_r8,
   op_sbc_a_r8,    op_sbc_a_r8,   op_sbc_a_r8,  op_sbc_a_r8,  op_sbc_a_r8,   op_sbc_a_r8,  op_sbc_a_r8,  op_sbc_a_r8,

   op_and_a_r8,    op_and_a_r8,   op_and_a_r8,  op_and_a_r8,  op_and_a_r8,   op_and_a_r8,  op_and_a_r8,  op_and_a_r8,
   op_xor_a_r8,    op_xor_a_r8,   op_xor_a_r8,  op_xor_a_r8,  op_xor_a_r8,   op_xor_a_r8,  op_xor_a_r8,  op_xor_a_r8,

   op_or_a_r8,     op_or_a_r8,    op_or_a_r8,   op_or_a_r8,   op_or_a_r8,    op_or_a_r8,   op_or_a_r8,   op_or_a_r8,
   op_cp_a_r8,     op_cp_a_r8,    op_cp_a_r8,   op_cp_a_r8,   op_cp_a_r8,    op_cp_a_r8,   op_cp_a_r8,   op_cp_a_r8,

   op_ret_cc,      op_pop,        op_jp_c_i16,  op_jp_i16,    op_call_c_i16, op_push,      op_add_a_i8,  op_rst_tgt3,
   op_ret_cc,      op_ret,        op_jp_c_i16,  TODO,         op_call_c_i16, op_call_i16,  op_adc_a_i8,  op_rst_tgt3,

   op_ret_cc,      op_pop,        op_jp_c_i16,  INVALID,      op_call_c_i16, op_push,      op_sub_a_i8,  op_rst_tgt3,
   op_ret_cc,      TODO,          op_jp_c_i16,  INVALID,      op_call_c_i16, INVALID,      op_sbc_a_i8,  op_rst_tgt3,

   op_ldh_i8_a,    op_pop,        op_ldh_c_a,   INVALID,      INVALID,       op_push,      op_and_a_i8,  op_rst_tgt3,
   op_add_sp_i8,   op_jp_hl,      op_ld_mi16_a, INVALID,      INVALID,       INVALID,      op_xor_a_i8,  op_rst_tgt3,

   op_ldh_a_i8,    op_pop,        op_ldh_a_c,   op_di,        INVALID,       op_push,      op_or_a_i8,   op_rst_tgt3,
   op_ld_hl_sp_i8, op_ld_sp_hl,   op_ld_a_mi16, op_ei,        INVALID,       INVALID,      op_cp_a_i8,   op_rst_tgt3,
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

   cpu->A  = 0x0;
   cpu->B  = 0x0;
   cpu->C  = 0x0;
   cpu->D  = 0x0;
   cpu->E  = 0x0;
   cpu->F  = 0x0;
   cpu->H  = 0x0;
   cpu->L  = 0x0;
   cpu->PC = 0x0;
   cpu->SP = 0x0;

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

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
void cpu_debug_run_opcode(cpu_t* cpu, uint8_t opcode)
{
   opcode_table[opcode](cpu, opcode);
}
