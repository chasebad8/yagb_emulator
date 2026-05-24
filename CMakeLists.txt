cmake_minimum_required(VERSION 3.16)

project(yagb_emulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.c
    src/emulator.c

    src/bus/bus.c
    src/cpu/cpu.c
    src/ppu/ppu.c
    src/timer/timer.c
    src/input/input.c
    src/cartridge/cartridge.c

    src/common/logging.c
)

add_executable(yagb ${SOURCES})

#compiler option to use relative file path for logging
target_compile_options(yagb PRIVATE
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
)

#compiler definitions for logging
# target_compile_definitions(yagb PRIVATE
#     LOGTIME
#     LOGFILE
# )

target_include_directories(yagb PRIVATE src)
