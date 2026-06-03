# YAGB Emulator

(Thank you Copilot for generating the initial README) ...

A Game Boy emulator written in C.

## Project Overview

YAGB Emulator (Yet Another Game Boy Emulator) is a modular Game Boy emulator designed with a clean separation of concerns. The emulator is built around a memory bus that acts as a service broker, coordinating communication between different hardware components.

## Architecture

The emulator follows a modular architecture where:
- **Bus** - Central memory management and component communication
- **CPU** - Processor core execution
- **PPU** - Graphics and video processing
- **Timer** - System timing and clock management
- **Input** - Button and controller input handling
- **Cartridge** - Game ROM loading and memory mapping

## Project Structure

```
yagb_emulator/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
└── src/
    ├── main.c              # Application entry point
    ├── emulator.c/h        # Main emulator interface
    ├── bus/
    │   ├── bus.c           # Memory bus implementation
    │   └── bus.h           # Memory bus interface
    ├── cpu/
    │   ├── cpu.c           # CPU implementation
    │   └── cpu.h           # CPU interface
    ├── ppu/
    │   ├── ppu.c           # Graphics processing unit
    │   └── ppu.h           # PPU interface
    ├── timer/
    │   ├── timer.c         # Timer system
    │   └── timer.h         # Timer interface
    ├── input/
    │   ├── input.c         # Input handling
    │   └── input.h         # Input interface
    ├── cartridge/
    │   ├── cartridge.c     # ROM cartridge management
    │   └── cartridge.h     # Cartridge interface
    └── common/
        ├── logging.c       # Logging utilities
        └── logging.h       # Logging interface
```

## Memory Map

The Game Boy memory map is implemented across multiple components:

| Address Range | Size | Purpose | Handler |
|---|---|---|---|
| `0000-3FFF` | 16 KiB | ROM Bank 0 (Fixed) | Cartridge |
| `4000-7FFF` | 16 KiB | ROM Bank 1-NN (Switchable) | Cartridge |
| `8000-9FFF` | 8 KiB | Video RAM (VRAM) | PPU |
| `A000-BFFF` | 8 KiB | External RAM | Cartridge |
| `C000-DFFF` | 8 KiB | Work RAM (WRAM) | Bus |
| `E000-FDFF` | - | Echo RAM (Mirror) | Bus |
| `FE00-FE9F` | 160 B | Object Attribute Memory (OAM) | PPU |
| `FEA0-FEFF` | - | Unusable | - |
| `FF00-FF7F` | 128 B | I/O Registers | Bus |

## Building

```bash
cmake -B build
cmake --build build
```

## Components

### Bus (`src/bus/`)
The central hub for memory access and component communication. Manages work RAM, high RAM, and I/O register access while delegating cartridge and video memory operations to their respective components.

### CPU (`src/cpu/`)
Implements the Zilog Z80-based CPU core, responsible for instruction execution and program flow.

### PPU (`src/ppu/`)
Handles graphics rendering, including sprite processing, background mapping, and display output.

### Timer (`src/timer/`)
Manages the Game Boy's internal timing system and clock cycles.

### Input (`src/input/`)
Processes button input and controller state.

### Cartridge (`src/cartridge/`)
Handles ROM loading, memory mapping, and cartridge-specific features like bank switching.

### Common (`src/common/`)
Provides utility functions including logging and debugging support.

## References
instruction set and flags
https://gbdev.io/pandocs/CPU_Instruction_Set.html

optable
https://gbdev.io/gb-opcodes/optables/

opcode description
https://rgbds.gbdev.io/docs/v1.0.1/gbz80.7
