#include <stdlib.h>
#include <stdio.h>

#include "cartridge/cartridge.h"
#include "common/logging.h"

void cartridge_init(cartridge_t *cartridge_p)
{
   LOG_DEBUG("initializing cartridge ...");

   cartridge_p->rom      = NULL;
   cartridge_p->rom_size = 0;

   LOG_DEBUG("cartridge init success!");
}

void cartridge_load(cartridge_t *cartridge_p, const char* cartridge_path)
{
#ifdef DEBUG_MODE
   cartridge_p->rom_size = MAX_ROM_SIZE;

   if ((cartridge_p->rom = malloc(cartridge_p->rom_size)) == NULL)
   {
      LOG_ERROR("Failed to dynamically allocate memory for rom");
      exit(-1);
   }
#else
   FILE *fptr = NULL;
   long cartridge_size = 0;

   if (cartridge_p->rom != NULL)
   {
      LOG_ERROR("Another cartridge is currently loaded into memory. Please unload it first");
      exit(-1);
   }
   else if ((fptr = fopen(cartridge_path, "rb")) == NULL)
   {
      LOG_ERROR("Failed to open cartridge %s", cartridge_path);
      exit(-1);
   }
   else
   {
      /* go to the end of the file to get the size */
      fseek(fptr, 0, SEEK_END);
      //cartridge_p->rom_size = ftell(fptr);
      cartridge_p->rom_size = MAX_ROM_SIZE;


      //if ((cartridge_p->rom_size <= 0) || (cartridge_p->rom_size > MAX_ROM_SIZE))
      if (cartridge_p->rom_size <= 0)
      {
         LOG_ERROR("Cartridge size out or ROM limit bounds: %lu byte(s)", cartridge_p->rom_size);
         fclose(fptr);
         exit(-1);
      }
      else if ((cartridge_p->rom = malloc(cartridge_p->rom_size)) == NULL)
      {
         LOG_ERROR("Failed to dynamically allocate memory for rom");
         fclose(fptr);
         exit(-1);
      }
      else
      {
         /* set file pointer back to the start of file */
         rewind(fptr);

         /* copy the cartridge into memory */
         fread(cartridge_p->rom, 1, cartridge_p->rom_size, fptr);
         fclose(fptr);

         LOG_INFO("successfully loaded cartridge from %s. size: %lu bytes\n", cartridge_path, cartridge_p->rom_size);
      }
   }
#endif
}

void cartridge_unload(cartridge_t *cartridge_p)
{
   if (cartridge_p->rom == NULL)
   {
      ;
   }
   else
   {
      free(cartridge_p->rom);
      cartridge_p->rom_size = 0;
   }

   LOG_DEBUG("Successfully unloaded cartridge");
}

uint8_t cartridge_read(cartridge_t *cartridge_p, uint16_t addr)
{
#ifndef DEBUG_MODE
   /* assume addr is valid */
   if (cartridge_p->rom == NULL)
   {
      LOG_ERROR("Cartridge not loaded");
      exit(-1);
   }
   else
   {
      return cartridge_p->rom[addr];
   }
#else
   return cartridge_p->rom[addr];
#endif
}

void cartridge_write(cartridge_t *cartridge_p, uint16_t addr, uint8_t value)
{
#ifndef DEBUG_MODE
   /* assume addr is valid */
   if (cartridge_p->rom == NULL)
   {
      LOG_ERROR("Cartridge not loaded");
      exit(-1);
   }
   else
   {
      cartridge_p->rom[addr] = value;
   }
#else
   cartridge_p->rom[addr] = value;
#endif
}
