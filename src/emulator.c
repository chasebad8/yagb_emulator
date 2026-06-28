#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "emulator.h"
#include "common/logging.h"

SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Texture  *texture;

static int rgb_frame_buffer[FRAME_BUFFER_SIZE];

#define W 160
#define H 144

static void emulator_2bb_to_rgba(emulator_t *emulator)
{
   for (int pixel_index = 0; pixel_index < FRAME_BUFFER_SIZE; pixel_index++)
   {
      if (emulator->ppu.frame_buffer[pixel_index] == 0x00)
      {
         rgb_frame_buffer[pixel_index] = 0x214231;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0x01)
      {
         rgb_frame_buffer[pixel_index] = 0x426b29;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0x10)
      {
         rgb_frame_buffer[pixel_index] = 0x6c9421;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0x11)
      {
         rgb_frame_buffer[pixel_index] = 0x8cad28;
      }
   }
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

#ifndef DEBUG_MODE
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

#ifndef DEBUG_MODE
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