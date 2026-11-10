cmake_minimum_required(VERSION 3.16)
set(CH32_TARGET_TRIPLET riscv32-wch-elf CACHE STRING "")
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wch_cmake/cmake/wch_gcc.cmake)
set(CMAKE_CXX_STANDARD 23)

project(bomzhur VERSION 1.0.0 LANGUAGES CXX C ASM)

ch32_fetch_cmsis(V0)
find_package(CMSIS COMPONENTS CH32V0 REQUIRED)

add_subdirectory(3rdparty/Zhele)

set(COMMON_COMPILE_OPTIONS
    -Os -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections
)

# --- Bootloader -------------------------------------------------------------
# CMSIS::CH32::V003 (the type target) brings the startup code, system_ch32v00x.c
# and the headers, but no linker script — every target below supplies its own.
# No Zhele here: the bootloader only needs src/utils/crc32.h and CMSIS.
add_executable(${CMAKE_PROJECT_NAME}_bootloader bootloader_src/main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME}_bootloader CMSIS::CH32::V003 CH32::NoSys CH32::Nano)
target_include_directories(${CMAKE_PROJECT_NAME}_bootloader PRIVATE .)
target_compile_options(${CMAKE_PROJECT_NAME}_bootloader PRIVATE ${COMMON_COMPILE_OPTIONS})
ch32_add_linker_script(${CMAKE_PROJECT_NAME}_bootloader PRIVATE src/V003F4_boot.ld)

ch32_print_size_of_target(${CMAKE_PROJECT_NAME}_bootloader)
ch32_generate_binary_file(${CMAKE_PROJECT_NAME}_bootloader)

# --- Application ------------------------------------------------------------
set(PROJECT_SOURCES
    src/main.cpp
)

# Declare common part as library
add_library(${CMAKE_PROJECT_NAME}_common INTERFACE)
target_sources(${CMAKE_PROJECT_NAME}_common INTERFACE ${PROJECT_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME}_common INTERFACE zhele::zhele CMSIS::CH32::V003 CH32::NoSys CH32::Nano)
# The firmware version is a property of the build, not of a source file: it comes
# from project(VERSION) and is the only thing sys.getinfo reports.
target_compile_definitions(${CMAKE_PROJECT_NAME}_common INTERFACE
    HSE_VALUE=0 HSI_VALUE=24000000 F_CPU=48000000
    FW_VERSION="${PROJECT_VERSION}")
target_compile_options(${CMAKE_PROJECT_NAME}_common INTERFACE ${COMMON_COMPILE_OPTIONS} -flto)
target_include_directories(${CMAKE_PROJECT_NAME}_common INTERFACE 3rdparty/include .)

# Add "usual" target for debug purpose: whole flash, no bootloader, no A/B
add_executable(${CMAKE_PROJECT_NAME})
target_link_libraries(${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}_common)
ch32_add_linker_script(${CMAKE_PROJECT_NAME} PRIVATE src/V003F4.ld)

ch32_print_size_of_target(${CMAKE_PROJECT_NAME})
ch32_generate_binary_file(${CMAKE_PROJECT_NAME})
ch32_generate_hex_file(${CMAKE_PROJECT_NAME})

# Add app0 target
add_executable(${CMAKE_PROJECT_NAME}_app0)
target_link_libraries(${CMAKE_PROJECT_NAME}_app0 ${CMAKE_PROJECT_NAME}_common)
ch32_add_linker_script(${CMAKE_PROJECT_NAME}_app0 PRIVATE src/V003F4_app0.ld)
ch32_print_size_of_target(${CMAKE_PROJECT_NAME}_app0)
ch32_generate_binary_file(${CMAKE_PROJECT_NAME}_app0)

# Add app1 target
add_executable(${CMAKE_PROJECT_NAME}_app1)
target_link_libraries(${CMAKE_PROJECT_NAME}_app1 ${CMAKE_PROJECT_NAME}_common)
ch32_add_linker_script(${CMAKE_PROJECT_NAME}_app1 PRIVATE src/V003F4_app1.ld)
ch32_print_size_of_target(${CMAKE_PROJECT_NAME}_app1)
ch32_generate_binary_file(${CMAKE_PROJECT_NAME}_app1)

add_dependencies(${CMAKE_PROJECT_NAME}_app1 ${CMAKE_PROJECT_NAME}_app0 ${CMAKE_PROJECT_NAME}_bootloader)

# --- Postbuild --------------------------------------------------------------
# Stamps CRC32 + update counter into both slots and glues boot/app0/app1 into
# the update file and the full first-flash image, next to the other build outputs.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}_app1 POST_BUILD
    COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/postbuild/postbuild.py"
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}"
    "${CMAKE_CURRENT_SOURCE_DIR}/postbuild/cfg.json"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
