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
   cpu->PC = 0x0;
   cpu->SP = 0x0;

   cpu->bus = bus;

   LOG_DEBUG("cpu init success!");
}

void cpu_process_interrupts(cpu_t *cpu)
{
   /* if IE is true, IME is true and the flag is set, jump PC */
   if (cpu->IME == 1)
   {

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
