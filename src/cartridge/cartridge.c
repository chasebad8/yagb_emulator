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
   FILE *fptr = NULL;
   size_t cartridge_size = 0;

   if (cartridge_path == NULL)
   {
      LOG_ERROR("Invalid path to cartridge");
      exit(-1);
   }
   else if (cartridge_p->rom != NULL)
   {
      LOG_ERROR("Another cartridge is currently loaded into memory. Please unload it first");
      exit(-1);
   }
   else if ((fptr = fopen(cartridge_path, "r")) == NULL)
   {
      LOG_ERROR("Failed to open cartridge %s", cartridge_path);
      exit(-1);
   }
   else
   {
      /* go to the end of the file to get the size */
      fseek(fptr, 0, SEEK_END);
      cartridge_p->rom_size = ftell(fptr);

      if ((cartridge_p->rom_size == 0) || (cartridge_p->rom_size > MAX_ROM_SIZE))
      {
         LOG_ERROR("Cartridge size out or ROM limit bounds: %d byte(s)", cartridge_p->rom_size);
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
         /* copy the cartridge into memory */
         fread(cartridge_p->rom, 1, cartridge_p->rom_size, fptr);
         fclose(fptr);

         LOG_DEBUG("successfully loaded cartridge from %s. size: %d bytes\n", cartridge_path, cartridge_p->rom_size);
      }
   }
}

void cartridge_unload(cartridge_t *cartridge_p)
{
   if (cartridge_p->rom == NULL)
   {
      return;
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
}
