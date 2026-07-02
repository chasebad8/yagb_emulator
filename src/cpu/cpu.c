/**
 * @file cpu.c
 * @brief Game Boy CPU emulation implementation
 *
 * Implements the central processing unit (CPU) for the YAGB Game Boy emulator.
 * Contains opcode handlers for 8-bit and 16-bit arithmetic operations, load/store
 * instructions, and increment/decrement operations. Provides helper functions for
 * reading and writing 8-bit and 16-bit registers with proper memory address handling.
 * Includes proper flag handling (Z, N, H, C) for ALU operations.
 *
 * @author Chase Badalato
 * @date 2026-05-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cpu.h"
#include "cpu_opcodes.h"
#include "common/logging.h"

/**
 * @brief
 *
 * @param cpu
 * @param bus
 */
void cpu_init(cpu_t *cpu, bus_t *bus)
{
   LOG_DEBUG("initializing cpu ...");

   cpu->A  = 0x0;
   cpu->B  = 0x0;
   cpu->C  = 0x0;
   cpu->D  = 0x0;
   cpu->E  = 0x0;
   cpu->F  = 0x0;
   cpu->H  = 0x0;
   cpu->L  = 0x0;
   cpu->PC = 0x100;
   cpu->SP = 0x0;

   cpu->bus = bus;

   LOG_DEBUG("cpu init success!");
}

/**
 * @brief push PC to the stack and jump to
 *        interrupt handler address
 *
 * @param cpu
 * @param addr
 * @return uint8_t
 */
static uint8_t cpu_call(cpu_t *cpu, uint16_t addr)
{
   bus_write(cpu->bus, --cpu->SP, ((cpu->PC >> 8) & 0xFF));
   bus_write(cpu->bus, --cpu->SP, (cpu->PC & 0xFF));
   cpu->PC = addr;

   return 24;
}

/**
 * @brief
 *
 * @param cpu
 */
uint8_t cpu_process_interrupts(cpu_t *cpu)
{
   if (cpu == NULL || cpu->bus == NULL)
   {
      LOG_ERROR("null pointer!");
      exit(-1);
   }
   else if (cpu->IME == 0)
   {
      return 0;
   }
   else
   {
      uint8_t if_reg  = bus_read(cpu->bus, IF_REG);
      uint8_t ie_reg  = bus_read(cpu->bus, IE_REG);
      uint8_t pending = if_reg & ie_reg & 0x1F;

      /* no interrupts to service */
      if (pending == 0)
      {
         return 0;
      }

      /* reset IME flag */
      cpu->IME = 0;

      if (pending & IF_REG_VBLANK_MASK)
      {
         LOG_DEBUG("processing vblank interrupt");

         /* clear interrupt flag */
         bus_write(cpu->bus, IF_REG, if_reg & ~IF_REG_VBLANK_MASK);
         return cpu_call(cpu, 0x0040);;
      }

      if (pending & IF_REG_LCD_MASK)
      {
         LOG_DEBUG("processing lcd interrupt");

         bus_write(cpu->bus, IF_REG, if_reg & ~IF_REG_LCD_MASK);
         return cpu_call(cpu, 0x0048);
      }

      if (pending & IF_REG_TIMER_MASK)
      {
         LOG_DEBUG("processing timer interrupt");

         bus_write(cpu->bus, IF_REG, if_reg & ~IF_REG_TIMER_MASK);
         return cpu_call(cpu, 0x0050);
      }

      if (pending & IF_REG_SERIAL_MASK)
      {
         LOG_DEBUG("processing serial interrupt");

         bus_write(cpu->bus, IF_REG, if_reg & ~IF_REG_SERIAL_MASK);
         return cpu_call(cpu, 0x0058);
      }

      if (pending & IF_REG_JOYPAD_MASK)
      {
         LOG_DEBUG("processing joypad interrupt");

         bus_write(cpu->bus, IF_REG, if_reg & ~IF_REG_JOYPAD_MASK);
         return cpu_call(cpu, 0x0060);
      }
   }
}

/**
 * @brief
 *
 * @param cpu
 */
uint8_t cpu_step(cpu_t *cpu)
{
   uint8_t t_cycles = 0;

   /* fetch, decode, execute baby */
   if(cpu->bus == NULL)
   {
      LOG_ERROR("cpu not connected to bus");
      exit(-1);
   }
   else
   {
      uint8_t opcode = bus_read(cpu->bus, cpu->PC++);
      t_cycles = cpu_run_opcode(cpu, opcode);
   }

   return t_cycles;
}