#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "emulator.h"
#include "common/logging.h"

typedef struct
{
    TTF_Font *font;
    SDL_Color color;
} debug_renderer_t;

debug_renderer_t debug;
debug_renderer_t debug_bold;

void debug_draw_text(SDL_Renderer *renderer,
                     debug_renderer_t *debug,
                     int x,
                     int y,
                     const char *text);

#define DRAW_LINE(fmt, ...) do { \
    snprintf(line, sizeof(line), fmt, ##__VA_ARGS__); \
    debug_draw_text(renderer, &debug, 10, y, line); \
    y += 16; \
} while (0)

#define DRAW_BOLD_LINE(fmt, ...) do { \
    snprintf(line, sizeof(line), fmt, ##__VA_ARGS__); \
    debug_draw_text(renderer, &debug_bold, 10, y, line); \
    y += 16; \
} while (0)
int y = 10;
char line[64];


SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Texture  *texture;
SDL_Texture  *vram_texture;

static int rgb_frame_buffer[FRAME_BUFFER_SIZE];
static uint32_t vram_preview_buffer[128 * 128];

pthread_t thread;
int value = 42;

#define WIDTH 160
#define HEIGHT 144
#define DEBUG_COL_WIDTH 240
#define GAME_SCALE 4
#define GAME_SCREEN_WIDTH (WIDTH * GAME_SCALE)
#define GAME_SCREEN_HEIGHT (HEIGHT * GAME_SCALE)
#define VRAM_PREVIEW_WIDTH 128
#define VRAM_PREVIEW_HEIGHT 128
#define VRAM_PREVIEW_SCALE 2
#define VRAM_VIEWPORT_GAP 20
#define WINDOW_WIDTH (DEBUG_COL_WIDTH + GAME_SCREEN_WIDTH + VRAM_VIEWPORT_GAP + (VRAM_PREVIEW_WIDTH * VRAM_PREVIEW_SCALE) + VRAM_VIEWPORT_GAP)
#define WINDOW_HEIGHT GAME_SCREEN_HEIGHT

void debug_draw_cpu(SDL_Renderer *renderer, cpu_t *cpu)
{
    int y = 10;
    char line[128];

    DRAW_BOLD_LINE("CPU REGISTERS");

    DRAW_LINE("PC: 0x%04X  SP: 0x%04X", cpu->PC, cpu->SP);

    DRAW_LINE("AF: 0x%02X%02X", cpu->A, cpu->F);
    DRAW_LINE("BC: 0x%02X%02X", cpu->B, cpu->C);
    DRAW_LINE("DE: 0x%02X%02X", cpu->D, cpu->E);
    DRAW_LINE("HL: 0x%02X%02X", cpu->H, cpu->L);

    DRAW_LINE(" ");

    DRAW_LINE("A: 0x%02X F: 0x%02X", cpu->A, cpu->F);
    DRAW_LINE("B: 0x%02X C: 0x%02X", cpu->B, cpu->C);
    DRAW_LINE("D: 0x%02X E: 0x%02X", cpu->D, cpu->E);
    DRAW_LINE("H: 0x%02X L: 0x%02X", cpu->H, cpu->L);
}

void debug_draw_io(SDL_Renderer *renderer, io_t *io)
{
   int y = 200;
   char line[128];

   DRAW_BOLD_LINE("IO REGISTERS");

   DRAW_LINE("IF:  %5s 0x%02X", "", io->io_ram[0x0F]);
   DRAW_LINE("LCDC: 0x%02X",        io->io_ram[0x40]);
   DRAW_LINE("STAT:%s 0x%02X",  "", io->io_ram[0x41]);
   DRAW_LINE("SCY: %2s 0x%02X", "", io->io_ram[0x42]);
   DRAW_LINE("SCX: %2s 0x%02X", "", io->io_ram[0x43]);
   DRAW_LINE("LY:  %4s 0x%02X", "", io->io_ram[0x44]);
   DRAW_LINE("LYC: %2s 0x%02X", "", io->io_ram[0x45]);
}

void debug_draw_text(SDL_Renderer *renderer,
                     debug_renderer_t *debug,
                     int x,
                     int y,
                     const char *text)
{
    SDL_Surface *surface =
        TTF_RenderText_Blended(debug->font,
                               text,
                               debug->color);

    SDL_Texture *texture =
        SDL_CreateTextureFromSurface(renderer,
                                     surface);

    SDL_Rect dst =
    {
        x,
        y,
        surface->w,
        surface->h
    };

    SDL_RenderCopy(renderer,
                   texture,
                   NULL,
                   &dst);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

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
      if (emulator->ppu.frame_buffer[pixel_index] == 0b00)
      {
         rgb_frame_buffer[pixel_index] = 0x8cad28;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0b01)
      {
         rgb_frame_buffer[pixel_index] = 0x6c9421;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0b10)
      {
         rgb_frame_buffer[pixel_index] = 0x426b29;
      }
      else if (emulator->ppu.frame_buffer[pixel_index] == 0b11)
      {
         rgb_frame_buffer[pixel_index] = 0x214231;
      }
   }
}

static void emulator_update_vram_preview(const uint32_t *pixels)
{
   if (vram_texture == NULL)
   {
      return;
   }

   if (pixels != NULL)
   {
      for (int i = 0; i < (VRAM_PREVIEW_WIDTH * VRAM_PREVIEW_HEIGHT); i++)
      {
         vram_preview_buffer[i] = pixels[i];
      }
   }

   SDL_UpdateTexture(vram_texture,
                     NULL,
                     vram_preview_buffer,
                     VRAM_PREVIEW_WIDTH * sizeof(uint32_t));
}

static void emulator_refresh_vram_preview(emulator_t *emulator)
{
   if (emulator == NULL)
   {
      return;
   }

   uint32_t pixel_arr[VRAM_PREVIEW_WIDTH * VRAM_PREVIEW_HEIGHT] = { 0 };

   for (uint16_t tile_index = 0; tile_index < 256; tile_index++)
   {
      uint16_t tile_addr = ppu_get_tile_data_addr(&emulator->ppu, tile_index, TILE_SOURCE_BG);

      for (uint8_t pixel = 0; pixel < 64; pixel++)
      {
         uint8_t tile_x = tile_index % 16;
         uint8_t tile_y = tile_index / 16;

         uint8_t px = pixel % 8;
         uint8_t py = pixel / 8;

         uint8_t x = tile_x * 8 + px;
         uint8_t y = tile_y * 8 + py;

         uint8_t pixel_col = ppu_get_tile_pixel_color_id(&emulator->ppu, tile_addr + ((pixel / 8) * 2), pixel);

         if (x < VRAM_PREVIEW_WIDTH && y < VRAM_PREVIEW_HEIGHT)
         {
            pixel_arr[y * VRAM_PREVIEW_WIDTH + x] = emulator_2bb_to_rgba_test(pixel_col);
         }
      }
   }

   emulator_update_vram_preview(pixel_arr);
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

      TTF_Init();

      debug.font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);

      if (!debug.font)
      {
         LOG_ERROR("Font load failed: %s\n", TTF_GetError());
         exit(1);
      }

      debug.color = (SDL_Color)
      {
         255, 255, 255, 255
      };

      debug_bold.font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14);

      if (!debug_bold.font)
      {
         LOG_ERROR("Font load failed: %s\n", TTF_GetError());
         exit(1);
      }

      debug_bold.color = (SDL_Color)
      {
         255, 255, 255, 255
      };

#if 0
      if (pthread_create(&thread, NULL, vram_display, emulator) != 0)
      {
         LOG_ERROR("pthread_create failed");
      }
#endif

#ifndef DEBUG_MODE
      SDL_Init(SDL_INIT_VIDEO);

      window = SDL_CreateWindow(
         "Yet Another GameBoy",
         SDL_WINDOWPOS_CENTERED,
         SDL_WINDOWPOS_CENTERED,
         WINDOW_WIDTH,
         WINDOW_HEIGHT,
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

      vram_texture = SDL_CreateTexture(
         renderer,
         SDL_PIXELFORMAT_ARGB8888,
         SDL_TEXTUREACCESS_STREAMING,
         VRAM_PREVIEW_WIDTH,
         VRAM_PREVIEW_HEIGHT);

      SDL_UpdateTexture(texture, NULL, emulator->ppu.frame_buffer, WIDTH * sizeof(uint32_t));
      SDL_UpdateTexture(vram_texture, NULL, vram_preview_buffer, VRAM_PREVIEW_WIDTH * sizeof(uint32_t));

      SDL_RenderClear(renderer);

      SDL_Rect game_rect = { DEBUG_COL_WIDTH, 0, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT };
      SDL_Rect vram_rect = { DEBUG_COL_WIDTH + GAME_SCREEN_WIDTH + VRAM_VIEWPORT_GAP, 0, VRAM_PREVIEW_WIDTH * VRAM_PREVIEW_SCALE, VRAM_PREVIEW_HEIGHT * VRAM_PREVIEW_SCALE };
      SDL_RenderCopy(renderer, texture, NULL, &game_rect);
      SDL_RenderCopy(renderer, vram_texture, NULL, &vram_rect);
      SDL_RenderPresent(renderer);
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

   static uint8_t curr = 0;

   emulator_2bb_to_rgba(emulator);
   emulator_refresh_vram_preview(emulator);

   SDL_UpdateTexture(texture, NULL, rgb_frame_buffer, WIDTH * sizeof(uint32_t));

   SDL_RenderClear(renderer);
   SDL_RenderCopy(renderer, texture, NULL, NULL);
   SDL_RenderPresent(renderer);

   while (1)
   {
      cycle_count  = cpu_step(&emulator->cpu);
      cycle_count += cpu_process_interrupts(&emulator->cpu);

      ppu_step(&emulator->ppu, cycle_count);

      //SDL_Delay(100);
      if(bus_read(&emulator->bus, LY_REG) != curr)
      {
         emulator_2bb_to_rgba(emulator);
         emulator_refresh_vram_preview(emulator);

#ifndef DEBUG_MODE
         /* cpu_process_interrupts(); */
         SDL_UpdateTexture(texture, NULL, rgb_frame_buffer, WIDTH * sizeof(uint32_t));

         SDL_RenderClear(renderer);

         SDL_Rect game_rect = { DEBUG_COL_WIDTH, 0, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT };
         SDL_Rect vram_rect = { DEBUG_COL_WIDTH + GAME_SCREEN_WIDTH + VRAM_VIEWPORT_GAP, 0, VRAM_PREVIEW_WIDTH * VRAM_PREVIEW_SCALE, VRAM_PREVIEW_HEIGHT * VRAM_PREVIEW_SCALE };
         SDL_RenderCopy(renderer, texture, NULL, &game_rect);
         SDL_RenderCopy(renderer, vram_texture, NULL, &vram_rect);
         debug_draw_cpu(renderer, &emulator->cpu);
         debug_draw_io(renderer, &emulator->io);

         SDL_RenderPresent(renderer);
         //SDL_Delay(10);

#endif
         curr = bus_read(&emulator->bus, LY_REG);
         // if(wait == 0)
         // {
         //    SDL_Delay(50);
         //    wait = 1;
         // }
         // }
         // else
         // {
         //    wait = 0;
         // }
      }
   }
}