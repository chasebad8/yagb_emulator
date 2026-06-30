#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <SDL2/SDL.h>

#include "emulator.h"
#include "common/logging.h"

SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Texture  *texture;

static int rgb_frame_buffer[FRAME_BUFFER_SIZE];

pthread_t thread;
int value = 42;

#define W 160
#define H 144

static uint32_t emulator_2bb_to_rgba_test(uint8_t pixel)
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

static void emulator_2bb_to_rgba(emulator_t *emulator)
{
   for (int pixel_index = 0; pixel_index < FRAME_BUFFER_SIZE; pixel_index++)
   {
      if (emulator->ppu.frame_buffer[pixel_index] == 0x00)
      {
         rgb_frame_buffer[pixel_index] = 0x8cad28;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0x01)
      {
         rgb_frame_buffer[pixel_index] = 0x6c9421;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0x10)
      {
         rgb_frame_buffer[pixel_index] = 0x426b29;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0x11)
      {
         rgb_frame_buffer[pixel_index] = 0x214231;
      }
   }
}

void *vram_display(void *arg)
{
   emulator_t *emu = (emulator_t *)arg;

   SDL_Init(SDL_INIT_VIDEO);

   SDL_Window *window_l = SDL_CreateWindow(
      "VRAM DUMP",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      160 * 4,
      144 * 4,
      SDL_WINDOW_SHOWN);

   SDL_Renderer *renderer_l = SDL_CreateRenderer(
      window,
      -1,
      SDL_RENDERER_ACCELERATED);

   SDL_Texture *texture_l = SDL_CreateTexture(
      renderer,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,
      160,
      144);

   uint8_t pixel_arr[255*64] = { 0 };
   uint32_t pixel_col = 0;

   LOG_INFO("bus = %p, ppu addr = %p", emu->bus, &emu->ppu);

   while(1)
   {
      for(uint8_t tile_index = 0; tile_index < 255; tile_index++)
      {
         for(uint8_t pixel = 0; pixel < 64; pixel++)
         {
            // pixel_col = ppu_get_tile_pixel_color_id(&emu->ppu,
            //                                        ppu_get_tile_data_addr(&emu->ppu, tile_index, TILE_SOURCE_BG) + ((pixel / 8) * 2),
            //                                        pixel);
            pixel_col = ppu_get_tile_pixel_color_id(&emu->ppu,
                                                   0,
                                                   0);

            //pixel_arr[pixel] = emulator_2bb_to_rgba_test(pixel_col);
         }
      }

      // SDL_UpdateTexture(texture_l, NULL, pixel_arr, W * sizeof(uint32_t));

      // SDL_RenderClear(renderer_l);
      // SDL_RenderCopy(renderer_l, texture_l, NULL, NULL);
      // SDL_RenderPresent(renderer_l);

   }

   return NULL;
}

/* an emulator instance will be created for 1 emulator. Technically we can support multiple emulators at once. */
void emulator_init(emulator_t *emulator)
{
   LOG_INFO("initializing emulator ...");

   if(emulator == NULL)
   {
      LOG_ERROR("emulator pointer is NULL");
      exit(-1);
   }
   else
   {
      /* memory broker */
      bus_init(&emulator->bus, &emulator->ppu, &emulator->rom, &emulator->io);

      /* central processing unit */
      cpu_init(&emulator->cpu, &emulator->bus);

      /* pixel processing unit */
      ppu_init(&emulator->ppu, &emulator->bus);

      /* game cartridge handling unit */
      cartridge_init(&emulator->rom);

      io_init(&emulator->io);

      if (pthread_create(&thread, NULL, vram_display, &emulator) != 0)
      {
         LOG_ERROR("pthread_create failed");
      }

#ifdef DEBUG_MODE
      SDL_Init(SDL_INIT_VIDEO);

      window = SDL_CreateWindow(
         "Yet Another GameBoy",
         SDL_WINDOWPOS_CENTERED,
         SDL_WINDOWPOS_CENTERED,
         160 * 4,
         144 * 4,
         SDL_WINDOW_SHOWN);

      renderer = SDL_CreateRenderer(
         window,
         -1,
         SDL_RENDERER_ACCELERATED);

      texture = SDL_CreateTexture(
         renderer,
         SDL_PIXELFORMAT_ARGB8888,
         SDL_TEXTUREACCESS_STREAMING,
         160,
         144);

      /* cpu_process_interrupts(); */
      SDL_UpdateTexture(texture, NULL, emulator->ppu.frame_buffer, W * sizeof(uint32_t));

      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);

      //SDL_Delay(1000);
#endif
   }
}

void emulator_load_game_cartridge(emulator_t *emulator, const char *game_cartridge_path)
{
   if(emulator == NULL)
   {
      LOG_ERROR("emulator pointer is NULL");
      exit(-1);
   }
   else if (game_cartridge_path == NULL)
   {
      LOG_ERROR("invalid path to game cartridge");
      exit(-1);
   }
   else
   {
      cartridge_load(&emulator->rom, game_cartridge_path);
   }
}

void emulator_unload_game_cartridge(emulator_t *emulator)
{
   if(emulator == NULL)
   {
      LOG_ERROR("emulator pointer is NULL");
      exit(-1);
   }
   else
   {
      cartridge_unload(&emulator->rom);
   }
}

void emulator_run(emulator_t *emulator)
{
   /* temporary */
   uint8_t cycle_count = 0;

   static uint8_t wait = 0;

   emulator_2bb_to_rgba(emulator);

   SDL_UpdateTexture(texture, NULL, rgb_frame_buffer, W * sizeof(uint32_t));

   SDL_RenderClear(renderer);
   SDL_RenderCopy(renderer, texture, NULL, NULL);
   SDL_RenderPresent(renderer);

   while (1)
   {
      cycle_count = cpu_step(&emulator->cpu);
      ppu_step(&emulator->ppu, cycle_count);

      if(bus_read(&emulator->bus, LY_REG) == 144)
      {
         emulator_2bb_to_rgba(emulator);

#ifdef DEBUG_MODE
         /* cpu_process_interrupts(); */
         SDL_UpdateTexture(texture, NULL, rgb_frame_buffer, W * sizeof(uint32_t));

         SDL_RenderClear(renderer);
         SDL_RenderCopy(renderer, texture, NULL, NULL);
         SDL_RenderPresent(renderer);
#endif
         if(wait == 0)
         {
            SDL_Delay(50);
            wait = 1;
         }
      }
      else
      {
         wait = 0;
      }
   }
}