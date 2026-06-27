/**
 * @file test_opcodes.c
 * @brief Game Boy CPU emulation implementation
 *
 * unit tests for important opcodes. Yes, copilot generated some of these. It is a
 * side project and I really did not want to write all of these
 *
 * @author Chase Badalato
 * @date 2026-05-31
 */

#include "unity.h"
#include "emulator.h"
#include "common/logging.h"
#include "cpu/cpu.h"
#include "cpu/cpu_opcodes.h"

#define READ_FLD(reg, pos) (((reg) >> (pos)) & 0x1)
#define WRITE_FLD(reg, val, pos) (((val) == 1) ? ((reg) |=  (1 << (pos))) : ((reg) &= ~(1 << (pos))))

#define ZERO_POS 7
#define READ_Z(reg) (READ_FLD((reg), ZERO_POS))
#define WRITE_Z(reg, val) WRITE_FLD((reg), (val), ZERO_POS)

#define SUBTRACT_POS 6
#define READ_N(reg) (READ_FLD((reg), SUBTRACT_POS))
#define WRITE_N(reg, val) WRITE_FLD((reg), (val), SUBTRACT_POS)

#define HALF_CARRY_POS 5
#define READ_H(reg) (READ_FLD((reg), HALF_CARRY_POS))
#define WRITE_H(reg, val) WRITE_FLD((reg), (val), HALF_CARRY_POS)

#define CARRY_POS 4
#define READ_C(reg) (READ_FLD((reg), CARRY_POS))
#define WRITE_C(reg, val) WRITE_FLD((reg), (val), CARRY_POS)

void test_op_ld(void)
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   /* ld [hl], imm8 */
   emu.cpu.PC = 0x1;
   emu.cpu.HL = 0x0002;
   emu.rom.rom[0x0001] = 0xAB;
   cpu_run_opcode(&emu.cpu, OP_LD_HL_N);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.rom.rom[0x0002]);

   /* ld r16, imm16 */
   emu.cpu.PC = 0;
   emu.rom.rom[0x0000] = 0xCD;
   emu.rom.rom[0x0001] = 0xAB;
   cpu_run_opcode(&emu.cpu, OP_LD_DE_NN);
   TEST_ASSERT_EQUAL_HEX(0xABCD, emu.cpu.DE);

   /* ld [r16mem], a */
   emu.cpu.PC = 0;
   emu.cpu.A = 0xAB;
   emu.cpu.DE = 0x0003;
   cpu_run_opcode(&emu.cpu, OP_LD_DE_A);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.rom.rom[0x0003]);

   /* ld a, [r16mem] */
   emu.cpu.PC = 0;
   emu.cpu.A = 0x00;
   emu.cpu.DE = 0x0003;
   cpu_run_opcode(&emu.cpu, OP_LD_A_DE);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.cpu.A);

   /* ldi hl, a */
   emu.cpu.PC = 0;
   emu.cpu.HL = 0x0005;
   emu.cpu.A = 0xAB;
   cpu_run_opcode(&emu.cpu, OP_LDI_HL_A);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.rom.rom[0x0005]);
   TEST_ASSERT_EQUAL_HEX(0x0006, emu.cpu.HL);

   /* ldd hl, a */
   emu.cpu.PC = 0;
   emu.cpu.HL = 0x0005;
   emu.cpu.A = 0xCD;
   cpu_run_opcode(&emu.cpu, OP_LDD_HL_A);
   TEST_ASSERT_EQUAL_HEX(0xCD, emu.rom.rom[0x0005]);
   TEST_ASSERT_EQUAL_HEX(0x0004, emu.cpu.HL);

   /* ldi a, hl */
   emu.cpu.PC = 0;
   emu.cpu.HL = 0x0007;
   emu.rom.rom[0x0007] = 0xEF;
   emu.cpu.A = 0x00;
   cpu_run_opcode(&emu.cpu, OP_LDI_A_HL);
   TEST_ASSERT_EQUAL_HEX(0xEF, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0008, emu.cpu.HL);

   /* ld hl, a */
   emu.cpu.PC = 0;
   emu.cpu.HL = 0x0010;
   emu.cpu.A = 0xAA;
   cpu_run_opcode(&emu.cpu, OP_LD_HL_A);
   TEST_ASSERT_EQUAL_HEX(0xAA, emu.rom.rom[0x0010]);
   TEST_ASSERT_EQUAL_HEX(0x0010, emu.cpu.HL);

   /* ldd a, hl */
   emu.cpu.PC = 0;
   emu.cpu.HL = 0x0007;
   emu.rom.rom[0x0007] = 0xF0;
   emu.cpu.A = 0x00;
   cpu_run_opcode(&emu.cpu, OP_LDD_A_HL);
   TEST_ASSERT_EQUAL_HEX(0xF0, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0006, emu.cpu.HL);

   /* ld [imm16], sp */
   emu.cpu.PC = 0;
   emu.cpu.SP = 0xABCD;
   emu.rom.rom[0x0000] = 0x05;  /* low byte of address */
   emu.rom.rom[0x0001] = 0x00;  /* high byte of address */
   cpu_run_opcode(&emu.cpu, OP_LD_NN_SP);
   TEST_ASSERT_EQUAL_HEX(0xCD, emu.rom.rom[0x0005]);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.rom.rom[0x0006]);

   emulator_unload_game_cartridge(&emu);
}

void test_op_inc_dec(void)
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   /* inc r16 */
   emu.cpu.BC = 0xABFF;
   cpu_run_opcode(&emu.cpu, OP_INC_BC);
   TEST_ASSERT_EQUAL_HEX(0xAC00, emu.cpu.BC);

   /* dec r16 */
   cpu_run_opcode(&emu.cpu, OP_DEC_BC);
   TEST_ASSERT_EQUAL_HEX(0xABFF, emu.cpu.BC);

   /* inc r8 */
   emu.cpu.B = 0xAB;
   cpu_run_opcode(&emu.cpu, OP_INC_B);
   TEST_ASSERT_EQUAL_HEX(0xAC, emu.cpu.B);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));

   /* dec r8 */
   cpu_run_opcode(&emu.cpu, OP_DEC_B);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.cpu.B);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));

   /* inc h - half-carry boundary */
   emu.cpu.H = 0x0F;
   cpu_run_opcode(&emu.cpu, OP_INC_H);
   TEST_ASSERT_EQUAL_HEX(0x10, emu.cpu.H);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));

   /* dec l - zero to 0xFF */
   emu.cpu.L = 0x00;
   cpu_run_opcode(&emu.cpu, OP_DEC_L);
   TEST_ASSERT_EQUAL_HEX(0xFF, emu.cpu.L);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));

   /* inc r8 - half-carry */
   emu.cpu.C = 0xFF;
   cpu_run_opcode(&emu.cpu, OP_INC_C);
   TEST_ASSERT_EQUAL_HEX(0x00, emu.cpu.C);
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));

   /* dec r8 - half-borrow */
   cpu_run_opcode(&emu.cpu, OP_DEC_C);
   TEST_ASSERT_EQUAL_HEX(0xFF, emu.cpu.C);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));

   emulator_unload_game_cartridge(&emu);
}

void test_op_artithmetic(void)
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

     /* add a, r8 */
   emu.cpu.A = 0x80;
   emu.cpu.B = 0x01;
   cpu_run_opcode(&emu.cpu, OP_ADD_A_B);
   TEST_ASSERT_EQUAL_HEX(0x81, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* add a, r8 */
   emu.cpu.A = 0x80;
   emu.cpu.C = 0xFF;
   cpu_run_opcode(&emu.cpu, OP_ADD_A_C);
   TEST_ASSERT_EQUAL_HEX(0x7F, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_C(emu.cpu.F));

   /* add a, r8 */
   emu.cpu.A = 0x01;
   emu.cpu.C = 0x0F;
   cpu_run_opcode(&emu.cpu, OP_ADD_A_C);
   TEST_ASSERT_EQUAL_HEX(0x10, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* add a, imm8 */
   emu.cpu.A = 0x80;
   emu.rom.rom[0x0000] = 0x5;
   cpu_run_opcode(&emu.cpu, OP_ADD_A_N);
   TEST_ASSERT_EQUAL_HEX(0x85, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* adc a, imm8 with carry and half-carry */
   emu.cpu.PC = 0x0;
   emu.cpu.A = 0x0F;
   WRITE_C(emu.cpu.F, 1);
   emu.rom.rom[0x0000] = 0x01;
   cpu_run_opcode(&emu.cpu, OP_ADC_A_N);
   TEST_ASSERT_EQUAL_HEX(0x11, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* add sp, imm8 */
   emu.cpu.PC = 0x0;
   emu.cpu.SP = 0x0001;
   emu.rom.rom[0x0000] = 0xFF; /* -1 */
   cpu_run_opcode(&emu.cpu, OP_ADD_SP_N);
   TEST_ASSERT_EQUAL_HEX(0x0000, emu.cpu.SP);
   TEST_ASSERT_EQUAL_HEX(0x0001, emu.cpu.PC);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_C(emu.cpu.F));

   /* ld hl, sp+imm8 */
   emu.cpu.PC = 0x0;
   emu.cpu.SP = 0xFFF8;
   emu.cpu.HL = 0x0000;
   emu.rom.rom[0x0000] = 0x08;
   cpu_run_opcode(&emu.cpu, OP_LD_HL_SP_N);
   TEST_ASSERT_EQUAL_HEX(0x0000, emu.cpu.HL);
   TEST_ASSERT_EQUAL_HEX(0x0001, emu.cpu.PC);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_C(emu.cpu.F));

   /* ld sp, hl */
   emu.cpu.HL = 0x1234;
   emu.cpu.SP = 0x0000;
   cpu_run_opcode(&emu.cpu, OP_LD_SP_HL);
   TEST_ASSERT_EQUAL_HEX(0x1234, emu.cpu.SP);
   TEST_ASSERT_EQUAL_HEX(0x1234, emu.cpu.HL);

   /* add hl, r16 */
   emu.cpu.HL = 0x0FFF;
   emu.cpu.DE = 0x0001;
   cpu_run_opcode(&emu.cpu, OP_ADD_HL_DE);
   TEST_ASSERT_EQUAL_HEX(0x1000, emu.cpu.HL);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* add hl, r16 */
   emu.cpu.HL = 0x00FF;
   emu.cpu.DE = 0x0001;
   cpu_run_opcode(&emu.cpu, OP_ADD_HL_DE);
   TEST_ASSERT_EQUAL_HEX(0x100, emu.cpu.HL);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* sub a, r8 - normal subtraction */
   emu.cpu.A = 0x10;
   emu.cpu.B = 0x05;
   cpu_run_opcode(&emu.cpu, OP_SUB_A_B);
   TEST_ASSERT_EQUAL_HEX(0x0B, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));  /* 0x0 - 0x5 requires borrow */
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* sub a, r8 - result is zero */
   emu.cpu.A = 0x05;
   emu.cpu.B = 0x05;
   cpu_run_opcode(&emu.cpu, OP_SUB_A_B);
   TEST_ASSERT_EQUAL_HEX(0x00, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* sub a, r8 - with half-borrow */
   emu.cpu.A = 0x10;
   emu.cpu.B = 0x08;
   cpu_run_opcode(&emu.cpu, OP_SUB_A_B);
   TEST_ASSERT_EQUAL_HEX(0x08, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* sub a, imm8 - normal subtraction */
   emu.cpu.PC = 0x0;
   emu.cpu.A = 0x20;
   emu.rom.rom[0x0000] = 0x07;
   cpu_run_opcode(&emu.cpu, OP_SUB_A_N);
   TEST_ASSERT_EQUAL_HEX(0x19, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));  /* 0x0 - 0x7 requires borrow */
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* sbc a, r8 - subtract with no carry set */
   emu.cpu.A = 0x15;
   emu.cpu.C = 0x05;
   emu.cpu.F = 0x00;  /* clear carry */
   cpu_run_opcode(&emu.cpu, OP_SBC_A_C);
   TEST_ASSERT_EQUAL_HEX(0x10, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* sbc a, r8 - subtract with carry set */
   emu.cpu.A = 0x15;
   emu.cpu.C = 0x05;
   WRITE_C(emu.cpu.F, 1);  /* set carry */
   cpu_run_opcode(&emu.cpu, OP_SBC_A_C);
   TEST_ASSERT_EQUAL_HEX(0x0F, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* and a, r8 - result is nonzero */
   emu.cpu.A = 0xFF;
   emu.cpu.B = 0x0F;
   cpu_run_opcode(&emu.cpu, OP_AND_A_B);
   TEST_ASSERT_EQUAL_HEX(0x0F, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* and a, r8 - result is zero */
   emu.cpu.A = 0x0F;
   emu.cpu.B = 0xF0;
   cpu_run_opcode(&emu.cpu, OP_AND_A_B);
   TEST_ASSERT_EQUAL_HEX(0x00, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* and a, imm8 */
   emu.cpu.PC = 0x0;
   emu.cpu.A = 0xF3;
   emu.rom.rom[0x0000] = 0x1F;
   cpu_run_opcode(&emu.cpu, OP_AND_A_N);
   TEST_ASSERT_EQUAL_HEX(0x13, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* xor a, r8 - result is nonzero */
   emu.cpu.A = 0xFF;
   emu.cpu.C = 0x0F;
   cpu_run_opcode(&emu.cpu, OP_XOR_A_C);
   TEST_ASSERT_EQUAL_HEX(0xF0, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* xor a, r8 - result is zero */
   emu.cpu.A = 0xAA;
   emu.cpu.C = 0xAA;
   cpu_run_opcode(&emu.cpu, OP_XOR_A_C);
   TEST_ASSERT_EQUAL_HEX(0x00, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* xor a, imm8 */
   emu.cpu.PC = 0x0;
   emu.cpu.A = 0x5C;
   emu.rom.rom[0x0000] = 0xA3;
   cpu_run_opcode(&emu.cpu, OP_XOR_A_N);
   TEST_ASSERT_EQUAL_HEX(0xFF, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* or a, r8 - result is nonzero */
   emu.cpu.A = 0x0F;
   emu.cpu.D = 0xF0;
   cpu_run_opcode(&emu.cpu, OP_OR_A_D);
   TEST_ASSERT_EQUAL_HEX(0xFF, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* or a, r8 - result is zero */
   emu.cpu.A = 0x00;
   emu.cpu.D = 0x00;
   cpu_run_opcode(&emu.cpu, OP_OR_A_D);
   TEST_ASSERT_EQUAL_HEX(0x00, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* or a, imm8 */
   emu.cpu.PC = 0x0;
   emu.cpu.A = 0x0F;
   emu.rom.rom[0x0000] = 0x30;
   cpu_run_opcode(&emu.cpu, OP_OR_A_N);
   TEST_ASSERT_EQUAL_HEX(0x3F, emu.cpu.A);
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* cp a, r8 - comparison with no match */
   emu.cpu.A = 0x20;
   emu.cpu.E = 0x10;
   cpu_run_opcode(&emu.cpu, OP_CP_A_E);
   TEST_ASSERT_EQUAL_HEX(0x20, emu.cpu.A);  /* A should not change */
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* cp a, r8 - comparison with match (Z flag set) */
   emu.cpu.A = 0x42;
   emu.cpu.E = 0x42;
   cpu_run_opcode(&emu.cpu, OP_CP_A_E);
   TEST_ASSERT_EQUAL_HEX(0x42, emu.cpu.A);  /* A should not change */
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_C(emu.cpu.F));

   /* cp a, imm8 - comparison with borrow */
   emu.cpu.PC = 0x0;
   emu.cpu.A = 0x05;
   emu.rom.rom[0x0000] = 0x10;
   cpu_run_opcode(&emu.cpu, OP_CP_A_N);
   TEST_ASSERT_EQUAL_HEX(0x05, emu.cpu.A);  /* A should not change */
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_Z(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x0 , READ_H(emu.cpu.F));  /* 0x5 - 0x0 = 0x5, no borrow */
   TEST_ASSERT_EQUAL_HEX(0x1 , READ_C(emu.cpu.F));

   emulator_unload_game_cartridge(&emu);
}

void test_op_pc_misc(void)
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   /* jp cond, imm16 */
   emu.rom.rom[0x0000] = 0x05;
   emu.rom.rom[0x0001] = 0x00;
   WRITE_Z(emu.cpu.F, 0x0);
   cpu_run_opcode(&emu.cpu, OP_JP_Z_NN);
   TEST_ASSERT_EQUAL_HEX(0x0002, emu.cpu.PC);

   /* jp cond, imm16 */
   emu.cpu.PC = 0x0;
   WRITE_Z(emu.cpu.F, 0x1);
   cpu_run_opcode(&emu.cpu, OP_JP_Z_NN);
   TEST_ASSERT_EQUAL_HEX(0x0005, emu.cpu.PC);

   /* ret cond */
   emu.cpu.PC = 0x0;
   emu.cpu.SP = 0x000A;
   emu.rom.rom[0x000A] = 0x02;
   emu.rom.rom[0x000B] = 0x00;
   WRITE_C(emu.cpu.F, 0x0);
   cpu_run_opcode(&emu.cpu, OP_RET_NC);
   TEST_ASSERT_EQUAL_HEX(0x0002, emu.cpu.PC);

   /* ret cond */
   emu.cpu.PC = 0x0;
   emu.cpu.SP = 0x000A;
   emu.rom.rom[0x000A] = 0x02;
   emu.rom.rom[0x000B] = 0x00;
   WRITE_C(emu.cpu.F, 0x1);
   cpu_run_opcode(&emu.cpu, OP_RET_NC);
   TEST_ASSERT_EQUAL_HEX(0x0000, emu.cpu.PC);

   /* call cond, imm16 */
   emu.cpu.PC = 0x0002;
   emu.cpu.SP = 0x00AA;
   emu.rom.rom[0x0002] = 0x07;
   emu.rom.rom[0x0003] = 0x00;
   WRITE_C(emu.cpu.F, 0x0);
   cpu_run_opcode(&emu.cpu, OP_CALL_NC_NN);
   TEST_ASSERT_EQUAL_HEX(0x0007, emu.cpu.PC);
   TEST_ASSERT_EQUAL_HEX(0x00A8, emu.cpu.SP);
   TEST_ASSERT_EQUAL_HEX(0x04,   emu.rom.rom[0x00A8]);
   TEST_ASSERT_EQUAL_HEX(0x00,   emu.rom.rom[0x00A9]);

   /* call cond, imm16 */
   emu.cpu.PC = 0x0002;
   emu.cpu.SP = 0x00AA;
   emu.rom.rom[0x000A8] = 0x00;
   emu.rom.rom[0x000A9] = 0x00;
   WRITE_C(emu.cpu.F, 0x1);
   cpu_run_opcode(&emu.cpu, OP_CALL_NC_NN);
   TEST_ASSERT_EQUAL_HEX(0x0004, emu.cpu.PC);
   TEST_ASSERT_EQUAL_HEX(0x00AA, emu.cpu.SP);
   TEST_ASSERT_EQUAL_HEX(0x00,   emu.rom.rom[0x00A8]);
   TEST_ASSERT_EQUAL_HEX(0x00,   emu.rom.rom[0x00A9]);

   /* rst 0x18 */
   emu.cpu.PC = 0x1234;
   emu.cpu.SP = 0x00AA;
   cpu_run_opcode(&emu.cpu, OP_RST_3);
   TEST_ASSERT_EQUAL_HEX(0x0018, emu.cpu.PC);
   TEST_ASSERT_EQUAL_HEX(0x00A8, emu.cpu.SP);
   TEST_ASSERT_EQUAL_HEX(0x34,   emu.rom.rom[0x00A8]);
   TEST_ASSERT_EQUAL_HEX(0x12,   emu.rom.rom[0x00A9]);

   /* push r16stk */
   emu.cpu.BC = 0x1234;
   emu.cpu.SP = 0x00AA;
   cpu_run_opcode(&emu.cpu, OP_PUSH_BC);
   TEST_ASSERT_EQUAL_HEX(0x1234, emu.cpu.BC);
   TEST_ASSERT_EQUAL_HEX(0x00A8, emu.cpu.SP);
   TEST_ASSERT_EQUAL_HEX(0x34,   emu.rom.rom[0x00A8]);
   TEST_ASSERT_EQUAL_HEX(0x12,   emu.rom.rom[0x00A9]);

   /* pop r16stk */
   emu.cpu.BC = 0x0000;
   cpu_run_opcode(&emu.cpu, OP_POP_BC);
   TEST_ASSERT_EQUAL_HEX(0x1234, emu.cpu.BC);
   TEST_ASSERT_EQUAL_HEX(0x00AA, emu.cpu.SP);

   /* push af / pop af round trip */
   emu.cpu.AF = 0x0F0F;
   emu.cpu.SP = 0x00AA;
   cpu_run_opcode(&emu.cpu, OP_PUSH_AF);
   TEST_ASSERT_EQUAL_HEX(0x00A8, emu.cpu.SP);
   TEST_ASSERT_EQUAL_HEX(0x0F, emu.rom.rom[0x00A8]);
   TEST_ASSERT_EQUAL_HEX(0x0F, emu.rom.rom[0x00A9]);

   emu.cpu.AF = 0x0000;
   cpu_run_opcode(&emu.cpu, OP_POP_AF);
   TEST_ASSERT_EQUAL_HEX(0x0F0F, emu.cpu.AF);
   TEST_ASSERT_EQUAL_HEX(0x00AA, emu.cpu.SP);

   /* scf */
   WRITE_C(emu.cpu.F, 0);
   cpu_run_opcode(&emu.cpu, OP_SCF);
   TEST_ASSERT_EQUAL_HEX(1, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0, READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0, READ_H(emu.cpu.F));

   /* ccf */
   WRITE_C(emu.cpu.F, 0);
   cpu_run_opcode(&emu.cpu, OP_CCF);
   TEST_ASSERT_EQUAL_HEX(1, READ_C(emu.cpu.F));
   cpu_run_opcode(&emu.cpu, OP_CCF);
   TEST_ASSERT_EQUAL_HEX(0, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0, READ_N(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0, READ_H(emu.cpu.F));


   emulator_unload_game_cartridge(&emu);
}

void test_op_bit_shifting(void)
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   /* rlca */
   emu.cpu.A = 0x10;
   cpu_run_opcode(&emu.cpu, OP_RLCA);
   TEST_ASSERT_EQUAL_HEX(0x0, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x20, emu.cpu.A);

   /* rlca */
   emu.cpu.A = 0x81;
   cpu_run_opcode(&emu.cpu, OP_RLCA);
   TEST_ASSERT_EQUAL_HEX(1, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x03, emu.cpu.A);

   /* rrca */
   emu.cpu.A = 0x10;
   WRITE_C(emu.cpu.F, 0);
   cpu_run_opcode(&emu.cpu, OP_RRCA);
   TEST_ASSERT_EQUAL_HEX(0x0, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x08, emu.cpu.A);

   /* rrca */
   emu.cpu.A = 0x81;
   cpu_run_opcode(&emu.cpu, OP_RRCA);
   TEST_ASSERT_EQUAL_HEX(1, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0xC0, emu.cpu.A);

   /* rra */
   emu.cpu.A = 0x10;
   WRITE_C(emu.cpu.F, 0);
   cpu_run_opcode(&emu.cpu, OP_RRA);
   TEST_ASSERT_EQUAL_HEX(0x0, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x08, emu.cpu.A);

   /* rra */
   emu.cpu.A = 0x81;
   cpu_run_opcode(&emu.cpu, OP_RRA);
   TEST_ASSERT_EQUAL_HEX(1, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x40, emu.cpu.A);
   cpu_run_opcode(&emu.cpu, OP_RRA);
   TEST_ASSERT_EQUAL_HEX(0, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0xA0, emu.cpu.A);

   /* rla */
   emu.cpu.A = 0x81;
   WRITE_C(emu.cpu.F, 0);
   cpu_run_opcode(&emu.cpu, OP_RLA);
   TEST_ASSERT_EQUAL_HEX(1, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x02, emu.cpu.A);
   cpu_run_opcode(&emu.cpu, OP_RLA);
   TEST_ASSERT_EQUAL_HEX(0, READ_C(emu.cpu.F));
   TEST_ASSERT_EQUAL_HEX(0x05, emu.cpu.A);

   emulator_unload_game_cartridge(&emu);
}

int run_cpu_tests(void)
{
   UNITY_BEGIN();

   TEST_MESSAGE("Running opcode unit tests ...");

   RUN_TEST(test_op_ld);
   RUN_TEST(test_op_inc_dec);
   RUN_TEST(test_op_artithmetic);
   RUN_TEST(test_op_pc_misc);
   RUN_TEST(test_op_bit_shifting);

   TEST_MESSAGE("done.");

   return UNITY_END();
}