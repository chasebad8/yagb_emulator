#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cartridge/cartridge.h"
#include "common/logging.h"

void cartridge_init(cartridge_t *cartridge_p)
{
   LOG_DEBUG("initializing cartridge ...");

   cartridge_p->rom      = NULL;
   cartridge_p->rom_size = 0;
   cartridge_p->rom_bank_count = 0;
   cartridge_p->ram = NULL;
   cartridge_p->ram_size = 0;
   cartridge_p->ram_bank_count = 0;
   cartridge_p->mapper = CARTRIDGE_MAPPER_ROM_ONLY;
   cartridge_p->mbc1_ram_enabled = 0;
   cartridge_p->mbc1_rom_bank_low5 = 1;
   cartridge_p->mbc1_bank_high2 = 0;
   cartridge_p->mbc1_banking_mode = 0;

   LOG_DEBUG("cartridge init success!");
}

void cartridge_load(cartridge_t *cartridge_p, const char* cartridge_path)
{
#ifdef DEBUG_MODE
   cartridge_p->rom_size = MAX_ROM_SIZE;
   cartridge_p->rom_bank_count = 2;

   if ((cartridge_p->rom = malloc(cartridge_p->rom_size)) == NULL)
   {
      LOG_ERROR("Failed to dynamically allocate memory for rom");
      exit(-1);
   }
   memset(cartridge_p->rom, 0, cartridge_p->rom_size);
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
      if (fseek(fptr, 0, SEEK_END) != 0 || (cartridge_size = ftell(fptr)) < 0)
      {
         LOG_ERROR("Failed to determine cartridge ROM size");
         fclose(fptr);
         exit(-1);
      }
      else if ((size_t)cartridge_size > MBC1_MAX_ROM_SIZE || cartridge_size < 0x150)
      {
         LOG_ERROR("Unsupported cartridge ROM size: %ld byte(s)", cartridge_size);
         fclose(fptr);
         exit(-1);
      }
      else if ((cartridge_p->rom = malloc((size_t)cartridge_size)) == NULL)
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
         cartridge_p->rom_size = (size_t)cartridge_size;
         if (fread(cartridge_p->rom, 1, cartridge_p->rom_size, fptr) != cartridge_p->rom_size)
         {
            LOG_ERROR("Failed to read cartridge ROM");
            free(cartridge_p->rom);
            cartridge_p->rom = NULL;
            fclose(fptr);
            exit(-1);
         }
         fclose(fptr);

         cartridge_p->rom_bank_count = (cartridge_p->rom_size + 0x3FFF) / 0x4000;
         uint8_t cartridge_type = cartridge_p->rom[0x0147];
         uint8_t ram_size_code = cartridge_p->rom[0x0149];

         if (cartridge_type >= 0x01 && cartridge_type <= 0x03)
         {
            cartridge_p->mapper = CARTRIDGE_MAPPER_MBC1;
         }

         switch (ram_size_code)
         {
            case 0x01: cartridge_p->ram_size = 0x0800; break;
            case 0x02: cartridge_p->ram_size = 0x2000; break;
            case 0x03: cartridge_p->ram_size = 0x8000; break;
            default: cartridge_p->ram_size = 0; break;
         }

         if (cartridge_p->mapper == CARTRIDGE_MAPPER_MBC1 && cartridge_p->ram_size > 0)
         {
            cartridge_p->ram = calloc(1, cartridge_p->ram_size);
            if (cartridge_p->ram == NULL)
            {
               LOG_ERROR("Failed to allocate cartridge RAM");
               free(cartridge_p->rom);
               cartridge_p->rom = NULL;
               exit(-1);
            }
            cartridge_p->ram_bank_count = (cartridge_p->ram_size + 0x1FFF) / 0x2000;
         }

         LOG_INFO("successfully loaded cartridge from %s. size: %lu bytes, mapper: %s\n",
                  cartridge_path,
                  (unsigned long)cartridge_p->rom_size,
                  cartridge_p->mapper == CARTRIDGE_MAPPER_MBC1 ? "MBC1" : "ROM-only");
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
      free(cartridge_p->ram);
      cartridge_p->rom = NULL;
      cartridge_p->ram = NULL;
      cartridge_p->rom_size = 0;
      cartridge_p->rom_bank_count = 0;
      cartridge_p->ram_size = 0;
      cartridge_p->ram_bank_count = 0;
      cartridge_p->mapper = CARTRIDGE_MAPPER_ROM_ONLY;
      cartridge_p->mbc1_ram_enabled = 0;
      cartridge_p->mbc1_rom_bank_low5 = 1;
      cartridge_p->mbc1_bank_high2 = 0;
      cartridge_p->mbc1_banking_mode = 0;
   }

   LOG_DEBUG("Successfully unloaded cartridge");
}

uint8_t cartridge_read(cartridge_t *cartridge_p, uint16_t addr)
{
   if (cartridge_p->rom == NULL)
   {
      LOG_ERROR("Cartridge not loaded");
      exit(-1);
   }

   size_t bank = addr / 0x4000;
   if (cartridge_p->mapper == CARTRIDGE_MAPPER_MBC1)
   {
      if (addr < 0x4000)
      {
         if (cartridge_p->mbc1_banking_mode != 0)
         {
            bank = (size_t)cartridge_p->mbc1_bank_high2 << 5;
         }
      }
      else
      {
         bank = ((size_t)cartridge_p->mbc1_bank_high2 << 5) |
                cartridge_p->mbc1_rom_bank_low5;
      }
   }

   if (cartridge_p->rom_bank_count > 0)
   {
      bank %= cartridge_p->rom_bank_count;
   }
   size_t offset = bank * 0x4000 + (addr & 0x3FFF);
   return offset < cartridge_p->rom_size ? cartridge_p->rom[offset] : 0xFF;
}

void cartridge_write(cartridge_t *cartridge_p, uint16_t addr, uint8_t value)
{
   if (cartridge_p->mapper != CARTRIDGE_MAPPER_MBC1)
   {
#ifdef DEBUG_MODE
      if (cartridge_p->rom != NULL && addr < cartridge_p->rom_size)
      {
         cartridge_p->rom[addr] = value;
      }
#endif
      return;
   }

   if (addr < 0x2000)
   {
      cartridge_p->mbc1_ram_enabled = (value & 0x0F) == 0x0A;
   }
   else if (addr < 0x4000)
   {
      cartridge_p->mbc1_rom_bank_low5 = value & 0x1F;
      if (cartridge_p->mbc1_rom_bank_low5 == 0)
      {
         cartridge_p->mbc1_rom_bank_low5 = 1;
      }
   }
   else if (addr < 0x6000)
   {
      cartridge_p->mbc1_bank_high2 = value & 0x03;
   }
   else if (addr < 0x8000)
   {
      cartridge_p->mbc1_banking_mode = value & 0x01;
   }
}

uint8_t cartridge_ram_read(cartridge_t *cartridge_p, uint16_t addr)
{
   if (cartridge_p->mapper != CARTRIDGE_MAPPER_MBC1 ||
       cartridge_p->mbc1_ram_enabled == 0 || cartridge_p->ram == NULL)
   {
      return 0xFF;
   }

   size_t bank = cartridge_p->mbc1_banking_mode ? cartridge_p->mbc1_bank_high2 : 0;
   size_t offset = bank * 0x2000 + (addr - 0xA000);
   return offset < cartridge_p->ram_size ? cartridge_p->ram[offset] : 0xFF;
}

void cartridge_ram_write(cartridge_t *cartridge_p, uint16_t addr, uint8_t value)
{
   if (cartridge_p->mapper != CARTRIDGE_MAPPER_MBC1 ||
       cartridge_p->mbc1_ram_enabled == 0 || cartridge_p->ram == NULL)
   {
      return;
   }

   size_t bank = cartridge_p->mbc1_banking_mode ? cartridge_p->mbc1_bank_high2 : 0;
   size_t offset = bank * 0x2000 + (addr - 0xA000);
   if (offset < cartridge_p->ram_size)
   {
      cartridge_p->ram[offset] = value;
   }
}
