#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include "common/logging.h"
#include "emulator.h"

#if BYTE_ORDER != LITTLE_ENDIAN
#error "!! this emulator requires a little-endian system !!"
#endif

void handle_sigint(int sig) {
   LOG_INFO("SIGINT received, exiting...");
   //emulator_unload_game_cartridge(&emulator);
   exit(0);
}

/**
 * this is the entry point to the program. It will call public emulator functions.
 * effectively it will progress the emulator.
 */
int main(int argc, char *argv[])
{
   signal(SIGINT, handle_sigint);

   emulator_t emulator;
   const char *boot_rom_path = NULL;
   const char *game_cartridge_path = NULL;

   LOG_INFO("Welcome to Yet Another GameBoy Emulator!");
   LOG_INFO("----------------------------------------");

   if (argc == 2)
   {
      game_cartridge_path = argv[1];
   }
   else if (argc == 4 && strcmp(argv[1], "--boot") == 0)
   {
      boot_rom_path = argv[2];
      game_cartridge_path = argv[3];
   }
   else
   {
      fprintf(stderr, "Usage: %s [--boot <dmg-boot-rom.bin>] <game.gb>\n", argv[0]);
      return EXIT_FAILURE;
   }

   {
      emulator_init(&emulator);

      emulator_load_game_cartridge(&emulator, game_cartridge_path);
      if (boot_rom_path != NULL)
      {
         emulator_load_boot_rom(&emulator, boot_rom_path);
      }

      emulator_run(&emulator);
   }

   return 1;
}