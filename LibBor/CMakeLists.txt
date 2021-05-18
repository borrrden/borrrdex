project(libbor)
cmake_minimum_required(VERSION 3.10)
enable_language(ASM_NASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_FLAGS "-g -F dwarf")

set(
    BORRRDEX_C_ARGS
    -fPIC
    -m64
)

set(
    BORRRDEX_CXX_ARGS
    ${BORRRDEX_C_ARGS}
)

set(
    LIBBOR_SRC
    src/borrrdex/framebuffer.cpp
    src/gfx/sse2.asm
    src/gfx/graphics.cpp
    src/gfx/font.cpp
)

add_library(
    bor STATIC
    ${LIBBOR_SRC}
)

target_include_directories(
    bor PRIVATE
    include
    ../LibC/sysdeps/borrrdex/include
)

install(
    TARGETS bor
    LIBRARY DESTINATION lib
)

install(
    FILES 
    include/borrrdex/core/framebuffer.h
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/include/borrrdex/core
)