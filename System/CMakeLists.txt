project(borrrdex-system)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

set(
    FTERM_SRC
    fterm/main.cpp
    fterm/input.cpp
)

set(
    SYSTEM_CXX_FLAGS
    -Werror
)

add_executable(
    fterm
    ${FTERM_SRC}
)

target_include_directories(
    fterm PRIVATE
    ../LibC/options/posix/include
    ../LibC/sysdeps/borrrdex/include
    ../LibBor/include
)

target_compile_options(
    fterm PRIVATE
    ${SYSTEM_CXX_FLAGS}
)

target_link_libraries(
    fterm PRIVATE
    -lfreetype
    -lbor
)

install(
    TARGETS fterm
    RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../Initrd
)