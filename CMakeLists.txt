cmake_minimum_required(VERSION 3.16)

project(yagb_emulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(Threads REQUIRED)

set(YAGB_SOURCES
   src/emulator.c

   src/bus/bus.c
   src/cpu/cpu.c
   src/cpu/cpu_opcodes.c
   src/ppu/ppu.c
   src/timer/timer.c
   src/cartridge/cartridge.c
   src/input_output/input_output.c

   src/common/logging.c
)

add_library(yagb_core STATIC ${YAGB_SOURCES})
add_library(yagb_core_test STATIC ${YAGB_SOURCES})

# ********************
#   Main Executable
# ********************
target_include_directories(yagb_core PUBLIC
   src
)

#yagb static library definitions
target_compile_definitions(yagb_core PRIVATE
   LOG_LEVEL=1
   #LOG_OPCODES
   EXIT_ON_BAD_OPCODE
   LOGFILE
)

target_compile_options(yagb_core PRIVATE
   -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
   -g
   -fsanitize=address
   #-fsanitize=thread
)

add_executable(yagb src/main.c)

target_link_libraries(yagb PRIVATE
   yagb_core
   SDL2::SDL2
   SDL2_ttf::SDL2_ttf
   Threads::Threads
)

target_compile_options(yagb PRIVATE
   -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
   -g
   -fsanitize=address
   #-fsanitize=thread
)

target_link_options(yagb PRIVATE
   -fsanitize=address
   #-fsanitize=thread
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
   LOG_LEVEL=0
   LOG_OPCODES
   DEBUG_MODE
)

add_executable(test_all
    test/run_tests.c
    test/test_opcodes.c
    test/test_ppu.c
    external/Unity/src/unity.c
)

target_link_libraries(test_all PRIVATE
   yagb_core_test
   SDL2::SDL2
   SDL2_ttf::SDL2_ttf
   Threads::Threads
)

add_test(NAME all_tests COMMAND test_all)
