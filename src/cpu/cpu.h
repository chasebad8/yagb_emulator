#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "bus/bus.h"

/**
 * @brief index for the 8 bit cpu registers.
 *        these index's need to be passed to
 *        write_r8 or read_r8 to access the
 *        actual cpu registers.
 *
 */
typedef enum
{
   REG_B = 0,
   REG_C,
   REG_D,
   REG_E,
   REG_H,
   REG_L,
   REG_HL_MEM,
   REG_A,

   REG_8_MAX = REG_A

} r8_idx_e;

/**
 * @brief index for the 16 bit cpu registers.
 *        these index's need to be passed to
 *        write_r16 or read_r16 to access the
 *        actual cpu registers.
 *
 */
typedef enum
{
   REG_BC = 0,
   REG_DE,
   REG_HL,
   REG_SP,

   REG_16_MAX

} r16_idx_e;

/**
 * @brief index for the 16 bit cpu registers
 *        whose value points to a mem addr.
 *        these index's need to be passed to
 *        write_m16 or read_m16 to access the
 *        actual cpu register pointer values.
 *
 */
typedef enum
{
   MEM_BC = 0,
   MEM_DE,
   MEM_HL_ADD,
   MEM_HL_SUB,

   MEM_16_MAX

} m16_idx_e;

typedef struct
{
   /*
      note: the gameboy is little endian (lsb at lowest addr)
            this is why F byte comes before A byte
   */
   union
   {
      struct
      {
         uint8_t F; /* also flag register  */
         uint8_t A;
      };
      uint16_t AF;
   };

   union
   {
      struct
      {
         uint8_t C;
         uint8_t B;
      };
      uint16_t BC;
   };

   union
   {
      struct
      {
         uint8_t E;
         uint8_t D;
      };
      uint16_t DE;
   };

   union
   {
      struct
      {
         uint8_t L;
         uint8_t H;
      };
      uint16_t HL;
   };

   uint16_t SP; /* stack pointer */
   uint16_t PC; /* program counter */

   uint8_t IME; /* interrupt master enable flag */

   bus_t *bus; /* pointer to bus instance */

} cpu_t;

typedef uint8_t (*opcode_handler_t)(cpu_t *cpu, uint8_t opcode);

void cpu_init(cpu_t *cpu, bus_t *bus);

uint8_t cpu_step(cpu_t *cpu);

/**
 * @brief
 *
 * @param cpu
 * @param opcode
 */
void cpu_debug_run_opcode(cpu_t* cpu, uint8_t opcode);

#endif // CPU_H
