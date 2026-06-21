#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "emulator.h"
#include "common/logging.h"

SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Texture  *texture;

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

#if 1
      SDL_Init(SDL_INIT_VIDEO);

      window = SDL_CreateWindow(
         "DMG Emulator",
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

#define W 160
#define H 144

      uint32_t framebuffer[W * H];

      // clear screen (black)
      for (int i = 0; i < W * H; i++)
         framebuffer[i] = 0x0f380f;

      // draw ONE white pixel (dot)
      framebuffer[10 + 10 * W] = 0x306230;
      framebuffer[20 + 20 * W] = 0x8bac0f;
      framebuffer[30 + 30 * W] = 0x9bbc0f;

      SDL_UpdateTexture(texture, NULL, framebuffer, W * sizeof(uint32_t));

      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);

      SDL_Delay(100);
#endif

      LOG_INFO("emulator init success!\n");
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

   while (1)
   {
      cycle_count = cpu_step(&emulator->cpu);
      ppu_step(&emulator->ppu, cycle_count);

#if 1
      /* cpu_process_interrupts(); */
      SDL_UpdateTexture(texture, NULL, emulator->ppu.frame_buffer, W * sizeof(uint32_t));

      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);
#endif

      SDL_Delay(1);
   }
}