cmake_minimum_required(VERSION 3.16)

project(yagb_emulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(YAGB_SOURCES
   src/emulator.c

   src/bus/bus.c
   src/cpu/cpu.c
   src/cpu/cpu_opcodes.c
   src/ppu/ppu.c
   src/timer/timer.c
   src/input/input.c
   src/cartridge/cartridge.c

   src/common/logging.c
)

add_library(yagb_core STATIC ${YAGB_SOURCES})
add_library(yagb_core_test STATIC ${YAGB_SOURCES})

# ********************
#   Main Executable
# ********************
target_include_directories(yagb_core PUBLIC
   src)

#yagb static library definitions
target_compile_definitions(yagb_core PRIVATE
   LOG_LEVEL=0
   LOG_OPCODES
   EXIT_ON_BAD_OPCODE
)

add_executable(yagb src/main.c)

target_link_libraries(yagb PRIVATE
   yagb_core)

target_compile_options(yagb PRIVATE
   -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
)

# ********************
# Unity Test Framework
# ********************
enable_testing()

target_include_directories(yagb_core_test PUBLIC
   src
   external/Unity/src/)

#yagb test static library definitions
target_compile_definitions(yagb_core_test PRIVATE
   LOG_LEVEL=3
   LOG_OPCODES
   DEBUG_MODE
)

add_executable(test_opcodes
    test/test_opcodes.c
    external/Unity/src/unity.c
)

target_link_libraries(test_opcodes PRIVATE yagb_core_test)

add_test(NAME opcode_tests COMMAND test_opcodes)