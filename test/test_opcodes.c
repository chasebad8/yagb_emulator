#include "unity.h"
#include "emulator.h"
#include "common/logging.h"
#include "cpu/cpu.h"
#include "cpu/cpu_opcodes.h"

void setUp(void)
{
   ;
}

void tearDown(void)
{
   ;
}

void test_op_ld(void)
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   /* ld [hl], imm8 */
   emu.cpu.PC = 0x1;
   emu.cpu.HL = 0x0002;
   emu.rom.rom[0x0001] = 0xAB;
   cpu_debug_run_opcode(&emu.cpu, OP_LD_HL_N);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.rom.rom[0x0002]);

   /* ld r16, imm16 */
   emu.cpu.PC = 0;
   emu.rom.rom[0x0000] = 0xCD;
   emu.rom.rom[0x0001] = 0xAB;
   cpu_debug_run_opcode(&emu.cpu, OP_LD_DE_NN);
   TEST_ASSERT_EQUAL_HEX(0xABCD, emu.cpu.DE);

   /* ld [r16mem], a */
   emu.cpu.PC = 0;
   emu.cpu.A = 0xAB;
   emu.cpu.DE = 0x0003;
   cpu_debug_run_opcode(&emu.cpu, OP_LD_DE_A);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.rom.rom[0x0003]);

   /* ld a, [r16mem] */
   emu.cpu.PC = 0;
   emu.cpu.A = 0x00;
   emu.cpu.DE = 0x0003;
   cpu_debug_run_opcode(&emu.cpu, OP_LD_A_DE);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.cpu.A);

   /* ld [imm16], sp */
   emu.cpu.PC = 0;
   emu.cpu.SP = 0xABCD;
   emu.rom.rom[0x0000] = 0x05;  /* low byte of address */
   emu.rom.rom[0x0001] = 0x00;  /* high byte of address */
   cpu_debug_run_opcode(&emu.cpu, OP_LD_NN_SP);
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
   cpu_debug_run_opcode(&emu.cpu, OP_INC_BC);
   TEST_ASSERT_EQUAL_HEX(0xAC00, emu.cpu.BC);

   /* dec r16 */
   cpu_debug_run_opcode(&emu.cpu, OP_DEC_BC);
   TEST_ASSERT_EQUAL_HEX(0xABFF, emu.cpu.BC);

   /* inc r8 */
   cpu_debug_run_opcode(&emu.cpu, OP_INC_B);
   TEST_ASSERT_EQUAL_HEX(0xAC, emu.cpu.B);

   /* dec r8 */
   cpu_debug_run_opcode(&emu.cpu, OP_DEC_B);
   TEST_ASSERT_EQUAL_HEX(0xAB, emu.cpu.B);

   /* inc r8 */
   cpu_debug_run_opcode(&emu.cpu, OP_INC_C);
   TEST_ASSERT_EQUAL_HEX(0x00, emu.cpu.C);

   /* dec r8 */
   cpu_debug_run_opcode(&emu.cpu, OP_DEC_C);
   TEST_ASSERT_EQUAL_HEX(0xFF, emu.cpu.C);

   emulator_unload_game_cartridge(&emu);


}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_op_ld);
    RUN_TEST(test_op_inc_dec);

    return UNITY_END();
}