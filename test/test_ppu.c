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
   if (pixel == 0x00)
   {
      return 0x214231;
   }
   else if (pixel == 0x01)
   {
      return 0x426b29;
   }
   else if (pixel == 0x10)
   {
      return 0x6c9421;
   }
   else if (pixel == 0x11)
   {
      return 0x8cad28;
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

   emu.io.io_ram[0x40] = 0;

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

   emu.io.io_ram[0x40] = 1 << 4;

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
   addr = ppu_get_tile_data_addr(&emu.ppu, -128, TILE_SOURCE_BG);
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

   uint32_t pixel_arr[128] = { 0x214231 };
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

int run_ppu_tests(void)
{
   UNITY_BEGIN();

   RUN_TEST(test_ppu_init);
   RUN_TEST(test_ppu_get_tile_index);
   RUN_TEST(test_ppu_get_tile_data_addr);
   RUN_TEST(test_ppu_get_tile_pixel_color_id);

   return UNITY_END();
}