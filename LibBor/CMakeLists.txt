project(libbor)
cmake_minimum_required(VERSION 3.10)

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