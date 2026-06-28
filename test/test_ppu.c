/**
 * @file test_ppu.c
 * @brief Game Boy PPU tests
 *
 * unit tests for ppu functions.
 *
 * @author Chase Badalato
 * @date 2026-05-31
 */

#include "unity.h"
#include "emulator.h"
#include "ppu/ppu.h"
#include "common/logging.h"

// void test_ppu_init( void )
// {
//    emulator_t emu = {0};

//    emulator_init(&emu);
//    emulator_load_game_cartridge(&emu, "");

//    TEST_ASSERT_NOT_EQUAL(emu.ppu.bus, NULL);
//    TEST_ASSERT_EQUAL(emu.ppu.frame_count, 0);
//    TEST_ASSERT_EQUAL(emu.ppu.state, STATE_2_OAM_QUERY);
//    TEST_ASSERT_EQUAL(emu.ppu.tick_count, 0);

//    emulator_unload_game_cartridge(&emu);
// }

void test_ppu_get_tile_index( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   /* the tile index is located in 0x9C00 */
   emu.io.io_ram[0x40] = 0;

   emu.ppu.vram[0x9800 - 0x8000] = 0xAB;
   emu.ppu.vram[0x9801 - 0x8000] = 0x5C;
   /* tile y=16 x=16 (aka pixel 128, 128) */
   emu.ppu.vram[(0x9800 + ((128/8) * 32) + (128/8)) - 0x8000] = 0x56;

   uint8_t tile_index = ppu_get_tile_index(&emu.ppu, 0, 0, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0xAB, tile_index);
   tile_index = ppu_get_tile_index(&emu.ppu, 7, 0, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0xAB, tile_index);
   tile_index = ppu_get_tile_index(&emu.ppu, 8, 0, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x5C, tile_index);
   tile_index = ppu_get_tile_index(&emu.ppu, 128, 128, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x56, tile_index);

   emu.io.io_ram[0x40] = 1 << 3;
   emu.ppu.vram[0x9C00 - 0x8000] = 0xDE;
   tile_index = ppu_get_tile_index(&emu.ppu, 0, 0, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0xDE, tile_index);

   emulator_unload_game_cartridge(&emu);
}

void test_ppu_get_tile_data_addr( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   emulator_unload_game_cartridge(&emu);
}

void test_ppu_init( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   TEST_ASSERT_NOT_EQUAL(emu.ppu.bus, NULL);
   TEST_ASSERT_EQUAL(emu.ppu.frame_count, 0);
   TEST_ASSERT_EQUAL(emu.ppu.state, STATE_2_OAM_QUERY);
   TEST_ASSERT_EQUAL(emu.ppu.tick_count, 0);

   emulator_unload_game_cartridge(&emu);
}

int run_ppu_tests(void)
{
   UNITY_BEGIN();

   RUN_TEST(test_ppu_init);
   RUN_TEST(test_ppu_get_tile_index);

   return UNITY_END();
}