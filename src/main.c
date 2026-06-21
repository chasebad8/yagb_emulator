#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "common/logging.h"
#include "emulator.h"

#if BYTE_ORDER != LITTLE_ENDIAN
#error "!! this emulator requires a little-endian system !!"
#endif
/**
 * this is the entry point to the program. It will call public emulator functions.
 * effectively it will progress the emulator.
 */
int main(int argc, char *argv[])
{
   emulator_t emulator;

   LOG_INFO("Welcome to Yet Another GameBoy Emulator!");
   LOG_INFO("----------------------------------------");

   if (argc == 1)
   {
      LOG_ERROR("no cartridge passed as argument");
      exit(-1);
   }
   else if(argc > 2)
   {
      LOG_ERROR("too many arguments, expecting one");
      exit(-1);
   }
   else
   {
      emulator_init(&emulator);

      emulator_load_game_cartridge(&emulator, argv[1]);

      emulator_run(&emulator);
   }

   return 1;
}