/**
 * @file test_ppu.c
 * @brief Game Boy PPU tests
 *
 * unit tests for ppu functions.
 *
 * @author Chase Badalato
 * @date 2026-05-31
 */

#include <SDL2/SDL.h>
#include <string.h>

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

static uint32_t emulator_2bb_to_rgba(uint8_t pixel)
{
   if (pixel == 0b00)
   {
      return 0x8cad28;
   }
   else if (pixel == 0b01)
   {
      return 0x6c9421;
   }
   else if (pixel == 0b10)
   {
      return 0x426b29;
   }
   else if (pixel == 0b11)
   {
      return 0x214231;
   }
}

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

   emu.io.io_ram[0x40] = 1 << 4;

   uint16_t addr = ppu_get_tile_data_addr(&emu.ppu, 0, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x8000, addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 1, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x8000 + 16, addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 16, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x8000 + (16 * 16), addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 31, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x8000 + (16 * 31), addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 32, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x8000 + (16 * 32), addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 255, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x8000 + (16 * 255), addr);

   emu.io.io_ram[0x40] = 0;

   addr = ppu_get_tile_data_addr(&emu.ppu, 0, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x9000, addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 1, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x9000 + 16, addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 16, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x9000 + (16 * 16), addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 31, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x9000 + (16 * 31), addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 32, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x9000 + (16 * 32), addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 127, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x9000 + (16 * 127), addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, 128, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x8800, addr);
   addr = ppu_get_tile_data_addr(&emu.ppu, -1, TILE_SOURCE_BG);
   TEST_ASSERT_EQUAL_HEX(0x9000 - (16 * 1), addr);

   emulator_unload_game_cartridge(&emu);
}

void test_ppu_get_tile_pixel_color_id( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   emu.io.io_ram[0x40] = 0;

   emu.ppu.vram[0x8000 - 0x8000] = 0xFF;
   emu.ppu.vram[0x8001 - 0x8000] = 0x00;
   emu.ppu.vram[0x8002 - 0x8000] = 0x7E;
   emu.ppu.vram[0x8003 - 0x8000] = 0xFF;
   emu.ppu.vram[0x8004 - 0x8000] = 0x85;
   emu.ppu.vram[0x8005 - 0x8000] = 0x81;
   emu.ppu.vram[0x8006 - 0x8000] = 0x89;
   emu.ppu.vram[0x8007 - 0x8000] = 0x83;
   emu.ppu.vram[0x8008 - 0x8000] = 0x93;
   emu.ppu.vram[0x8009 - 0x8000] = 0x85;
   emu.ppu.vram[0x800A - 0x8000] = 0xA5;
   emu.ppu.vram[0x800B - 0x8000] = 0x8B;
   emu.ppu.vram[0x800C - 0x8000] = 0xC9;
   emu.ppu.vram[0x800D - 0x8000] = 0x97;
   emu.ppu.vram[0x800E - 0x8000] = 0x7E;
   emu.ppu.vram[0x800F - 0x8000] = 0xFF;

   emu.ppu.vram[0x8010 - 0x8000] = 0x80; emu.ppu.vram[0x8011 - 0x8000] = 0x00;
   emu.ppu.vram[0x8012 - 0x8000] = 0x40; emu.ppu.vram[0x8013 - 0x8000] = 0x00;
   emu.ppu.vram[0x8014 - 0x8000] = 0x20; emu.ppu.vram[0x8015 - 0x8000] = 0x00;
   emu.ppu.vram[0x8016 - 0x8000] = 0x10; emu.ppu.vram[0x8017 - 0x8000] = 0x00;
   emu.ppu.vram[0x8018 - 0x8000] = 0x08; emu.ppu.vram[0x8019 - 0x8000] = 0x00;
   emu.ppu.vram[0x801A - 0x8000] = 0x04; emu.ppu.vram[0x801B - 0x8000] = 0x00;
   emu.ppu.vram[0x801C - 0x8000] = 0x02; emu.ppu.vram[0x801D - 0x8000] = 0x00;
   emu.ppu.vram[0x801E - 0x8000] = 0x01; emu.ppu.vram[0x801F - 0x8000] = 0x00;

   uint8_t tile_pixel_colour = ppu_get_tile_pixel_color_id(&emu.ppu, 0x8000, 0x00);
   TEST_ASSERT_EQUAL_HEX(0x01, tile_pixel_colour);
   tile_pixel_colour = ppu_get_tile_pixel_color_id(&emu.ppu, 0x8000, 0x10);
   TEST_ASSERT_EQUAL_HEX(0x01, tile_pixel_colour);

   uint32_t pixel_arr[128] = { 0 };
   uint8_t pixel_col = 0;

   const int SCALE = 32;

   for(uint8_t pixel = 0; pixel < 64; pixel++)
   {
      pixel_col = ppu_get_tile_pixel_color_id(&emu.ppu,
                                              ppu_get_tile_data_addr(&emu.ppu, 0x0, TILE_SOURCE_BG) + ((pixel / 8) * 2),
                                              pixel);

      pixel_arr[pixel] = emulator_2bb_to_rgba(pixel_col);
   }

   SDL_Init(SDL_INIT_VIDEO);

   SDL_Window *window = SDL_CreateWindow(
      "Tile Test",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      8 * SCALE,
      8 * SCALE,
      SDL_WINDOW_SHOWN);

   SDL_Rect dst = {
         0,
         0,
         8 * SCALE,
         8 * SCALE
      };

   SDL_Renderer *renderer = SDL_CreateRenderer(
      window,
      -1,
      SDL_RENDERER_ACCELERATED);

   SDL_Texture *texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             8,
                                             8);
   /* cpu_process_interrupts(); */
   SDL_UpdateTexture(texture, NULL, pixel_arr, 8 * sizeof(uint32_t));

   SDL_RenderClear(renderer);
   SDL_RenderCopy(renderer, texture, NULL, &dst);
   SDL_RenderPresent(renderer);

   SDL_Delay(5000);

   for(uint8_t pixel = 0; pixel < 64; pixel++)
   {
      pixel_col = ppu_get_tile_pixel_color_id(&emu.ppu,
                                              ppu_get_tile_data_addr(&emu.ppu, 0x1, TILE_SOURCE_BG) + ((pixel / 8) * 2),
                                              pixel);

      pixel_arr[pixel] = emulator_2bb_to_rgba(pixel_col);
   }

   /* cpu_process_interrupts(); */
   SDL_UpdateTexture(texture, NULL, pixel_arr, 8 * sizeof(uint32_t));

   SDL_RenderClear(renderer);
   SDL_RenderCopy(renderer, texture, NULL, &dst);
   SDL_RenderPresent(renderer);

   SDL_Delay(5000);

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

void test_ppu_vram_oam_rw( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   /* VRAM read/write */
   ppu_vram_write(&emu.ppu, 0x1234 - 0x8000, 0x5A);
   TEST_ASSERT_EQUAL_HEX(0x5A, ppu_vram_read(&emu.ppu, 0x1234 - 0x8000));

   /* OAM read/write */
   ppu_oam_write(&emu.ppu, 10, 0x77);
   TEST_ASSERT_EQUAL_HEX(0x77, ppu_oam_read(&emu.ppu, 10));

   emulator_unload_game_cartridge(&emu);
}

void test_ppu_tile_pixel_various_patterns( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   /* prepare a tile with known patterns across rows */
   /* row 0: 0b10101010 / 0b01010101 => pixels alternate */
   emu.ppu.vram[0x8000 - 0x8000] = 0xAA; /* low */
   emu.ppu.vram[0x8001 - 0x8000] = 0x55; /* high */

   /* check pixels 0..7 */
   uint8_t cols[8];
   for (uint8_t i = 0; i < 8; i++)
   {
      cols[i] = ppu_get_tile_pixel_color_id(&emu.ppu, 0x8000, i);
   }

   /* expected pattern: 10 (2),01 (1),10,01... depends on bit mapping -> assert known values */
   TEST_ASSERT_EQUAL_HEX(0x01, cols[0]);
   TEST_ASSERT_EQUAL_HEX(0x02, cols[1]);

   emulator_unload_game_cartridge(&emu);
}

void test_ppu_window_line_counter_pauses_when_hidden( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   emu.io.io_ram[0x40] = 0xA1;
   emu.io.io_ram[0x4A] = 0;
   emu.io.io_ram[0x4B] = 7;
   emu.io.io_ram[0x44] = 0;
   emu.io.io_ram[0x45] = 0xFF;

   for (uint16_t tick = 0; tick < 456; tick++)
   {
      ppu_step(&emu.ppu, 1);
   }
   TEST_ASSERT_EQUAL_UINT8(1, emu.ppu.window_line);

   emu.io.io_ram[0x4B] = 0xF0;
   for (uint16_t tick = 0; tick < 456; tick++)
   {
      ppu_step(&emu.ppu, 1);
   }
   TEST_ASSERT_EQUAL_UINT8(1, emu.ppu.window_line);

   emu.io.io_ram[0x4B] = 7;
   for (uint16_t tick = 0; tick < 456; tick++)
   {
      ppu_step(&emu.ppu, 1);
   }
   TEST_ASSERT_EQUAL_UINT8(2, emu.ppu.window_line);

   emulator_unload_game_cartridge(&emu);
}

void test_ppu_renders_sprite_and_preserves_transparent_pixels( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   emu.io.io_ram[0x40] = 0x93;
   emu.io.io_ram[0x47] = 0xE4;
   emu.io.io_ram[0x48] = 0xE4;
   emu.io.io_ram[0x4A] = 0xFF;
   emu.io.io_ram[0x45] = 0xFF;

   emu.ppu.vram[0] = 0x80;
   emu.ppu.vram[1] = 0x00;
   for (uint8_t row = 0; row < 8; row++)
   {
      emu.ppu.vram[16 + (row * 2)] = 0x00;
      emu.ppu.vram[16 + (row * 2) + 1] = 0xFF;
   }
   emu.ppu.vram[0x1800] = 1;

   ppu_oam_write(&emu.ppu, 0, 16);
   ppu_oam_write(&emu.ppu, 1, 8);
   ppu_oam_write(&emu.ppu, 2, 0);
   ppu_oam_write(&emu.ppu, 3, 0);

   ppu_step(&emu.ppu, 80);

   TEST_ASSERT_EQUAL_UINT8(1, emu.ppu.frame_buffer[0]);
   TEST_ASSERT_EQUAL_UINT8(2, emu.ppu.frame_buffer[1]);

   emulator_unload_game_cartridge(&emu);
}

void test_bus_boot_rom_overlay_disables_through_ff50( void )
{
   emulator_t emu = {0};
   uint8_t boot_rom[BOOT_ROM_SIZE] = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   emu.rom.rom[0x0000] = 0xA5;
   emu.rom.rom[0x0100] = 0x5A;
   boot_rom[0x0000] = 0x3C;
   bus_map_boot_rom(&emu.bus, boot_rom);

   TEST_ASSERT_EQUAL_HEX8(0x3C, bus_read(&emu.bus, 0x0000));
   TEST_ASSERT_EQUAL_HEX8(0x5A, bus_read(&emu.bus, 0x0100));
   TEST_ASSERT_EQUAL_HEX8(0x00, bus_read(&emu.bus, BANK_REG));

   bus_write(&emu.bus, 0x0000, 0x0A);
   bus_write(&emu.bus, 0x2000, 0x02);
   TEST_ASSERT_EQUAL_HEX8(0x3C, bus_read(&emu.bus, 0x0000));

   bus_write(&emu.bus, BANK_REG, 0x00);
   TEST_ASSERT_EQUAL_HEX8(0x3C, bus_read(&emu.bus, 0x0000));

   bus_write(&emu.bus, BANK_REG, 0x01);
   TEST_ASSERT_EQUAL_HEX8(0xA5, bus_read(&emu.bus, 0x0000));
   TEST_ASSERT_EQUAL_HEX8(0x01, bus_read(&emu.bus, BANK_REG));

   bus_write(&emu.bus, BANK_REG, 0x00);
   TEST_ASSERT_EQUAL_HEX8(0xA5, bus_read(&emu.bus, 0x0000));

   emulator_unload_game_cartridge(&emu);
}

void test_mbc1_rom_and_ram_banking( void )
{
   cartridge_t cartridge;
   uint8_t test_rom[4 * 0x4000];
   uint8_t test_ram[4 * 0x2000] = {0};

   cartridge_init(&cartridge);
   for (uint8_t bank = 0; bank < 4; bank++)
   {
      memset(&test_rom[bank * 0x4000], bank, 0x4000);
   }
   cartridge.rom = test_rom;
   cartridge.rom_size = sizeof(test_rom);
   cartridge.rom_bank_count = 4;
   cartridge.ram = test_ram;
   cartridge.ram_size = sizeof(test_ram);
   cartridge.ram_bank_count = 4;
   cartridge.mapper = CARTRIDGE_MAPPER_MBC1;

   cartridge.mapper = CARTRIDGE_MAPPER_ROM_ONLY;
   TEST_ASSERT_EQUAL_HEX8(1, cartridge_read(&cartridge, 0x4000));
   cartridge.mapper = CARTRIDGE_MAPPER_MBC1;

   TEST_ASSERT_EQUAL_HEX8(0, cartridge_read(&cartridge, 0x0000));
   TEST_ASSERT_EQUAL_HEX8(1, cartridge_read(&cartridge, 0x4000));

   cartridge_write(&cartridge, 0x2000, 2);
   TEST_ASSERT_EQUAL_HEX8(2, cartridge_read(&cartridge, 0x4000));
   cartridge_write(&cartridge, 0x2000, 0);
   TEST_ASSERT_EQUAL_HEX8(1, cartridge_read(&cartridge, 0x4000));

   cartridge_write(&cartridge, 0x0000, 0x0A);
   cartridge_write(&cartridge, 0x6000, 1);
   cartridge_write(&cartridge, 0x4000, 2);
   cartridge_ram_write(&cartridge, 0xA000, 0x5A);
   TEST_ASSERT_EQUAL_HEX8(0x5A, cartridge_ram_read(&cartridge, 0xA000));
   TEST_ASSERT_EQUAL_HEX8(0x00, test_ram[0]);
   TEST_ASSERT_EQUAL_HEX8(0x5A, test_ram[2 * 0x2000]);

   cartridge_write(&cartridge, 0x0000, 0);
   TEST_ASSERT_EQUAL_HEX8(0xFF, cartridge_ram_read(&cartridge, 0xA000));
}

void test_bus_echo_ram_mirrors_wram( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   bus_write(&emu.bus, 0xC123, 0x5A);
   TEST_ASSERT_EQUAL_HEX8(0x5A, bus_read(&emu.bus, 0xE123));

   bus_write(&emu.bus, 0xFDFF, 0xA5);
   TEST_ASSERT_EQUAL_HEX8(0xA5, bus_read(&emu.bus, 0xDDFF));

   emulator_unload_game_cartridge(&emu);
}

void test_bus_ignores_unusable_memory_writes( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");

   ppu_oam_write(&emu.ppu, 0x9F, 0x5A);
   bus_write(&emu.bus, 0xFEFF, 0xA5);
   TEST_ASSERT_EQUAL_HEX8(0x5A, ppu_oam_read(&emu.ppu, 0x9F));

   emulator_unload_game_cartridge(&emu);
}

void test_ppu_stops_timing_when_lcd_is_disabled( void )
{
   emulator_t emu = {0};

   emulator_init(&emu);
   emulator_load_game_cartridge(&emu, "");
   emu.io.io_ram[0x40] = 0;
   emu.io.io_ram[0x44] = 0x90;

   for (uint16_t ticks = 0; ticks < 1000; ticks += 100)
   {
      ppu_step(&emu.ppu, 100);
   }

   TEST_ASSERT_EQUAL_UINT8(0, emu.io.io_ram[0x44]);
   TEST_ASSERT_EQUAL_UINT8(STATE_0_HBLANK, emu.ppu.state);
   TEST_ASSERT_EQUAL_UINT16(0, emu.ppu.tick_count);
   TEST_ASSERT_EQUAL_HEX8(0, emu.io.io_ram[0x0F] & IF_REG_VBLANK_MASK);

   emu.io.io_ram[0x40] = 0x80;
   ppu_step(&emu.ppu, 1);
   TEST_ASSERT_EQUAL_UINT8(STATE_2_OAM_QUERY, emu.ppu.state);
   TEST_ASSERT_EQUAL_UINT8(0, emu.io.io_ram[0x44]);

   emulator_unload_game_cartridge(&emu);
}

int run_ppu_tests(void)
{
   UNITY_BEGIN();

   RUN_TEST(test_ppu_init);
   RUN_TEST(test_ppu_get_tile_index);
   RUN_TEST(test_ppu_get_tile_data_addr);
   RUN_TEST(test_ppu_get_tile_pixel_color_id);
   RUN_TEST(test_ppu_vram_oam_rw);
   RUN_TEST(test_ppu_tile_pixel_various_patterns);
   RUN_TEST(test_ppu_window_line_counter_pauses_when_hidden);
   RUN_TEST(test_ppu_renders_sprite_and_preserves_transparent_pixels);
   RUN_TEST(test_bus_boot_rom_overlay_disables_through_ff50);
   RUN_TEST(test_mbc1_rom_and_ram_banking);
   RUN_TEST(test_bus_echo_ram_mirrors_wram);
   RUN_TEST(test_bus_ignores_unusable_memory_writes);
   RUN_TEST(test_ppu_stops_timing_when_lcd_is_disabled);

   return UNITY_END();
}