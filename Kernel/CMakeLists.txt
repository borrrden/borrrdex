project(Kernel)
cmake_minimum_required(VERSION 3.10)
enable_language(ASM_NASM)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(
    KERNEL_C_FLAGS
    -Wno-write-strings
    -Wno-unused-parameter
    -Wno-sign-compare
    -DBorrrdex64
    -ffreestanding
    -nostdlib
    -mcmodel=large
    -mno-red-zone
    -fno-pic
    -mno-mmx
    -mno-sse
    -mno-sse2
    -fno-stack-protector
    -fno-builtin 
    -finline-functions
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(
        KERNEL_C_FLAGS
        -g
        ${KERNEL_C_FLAGS}
    )
endif()

add_subdirectory(src/lai)
target_compile_options(
    lai PRIVATE
    ${KERNEL_C_FLAGS}
)

set(
    KERNEL_CXX_FLAGS
    ${KERNEL_C_FLAGS}
    -fno-exceptions
    -fno-rtti
    -Wno-deprecated-volatile
    -Wno-non-c-typedef-for-linkage
)

set(
    KERNEL_LINKER_FLAGS
    ${KERNEL_C_FLAGS}
    -m64
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linkscript-x86_64.ld
    -z max-page-size=0x1000
)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_FLAGS "-g -F dwarf")
set(CMAKE_EXECUTABLE_SUFFIX .elf)

add_executable(
    kernel
    src/arch/x86_64/entry.asm
    src/arch/x86_64/hal.cpp
    src/arch/x86_64/serial.cpp
    src/arch/x86_64/io.cpp
    src/arch/x86_64/idt.asm
    src/arch/x86_64/idt.cpp
    src/arch/x86_64/apic.cpp
    src/arch/x86_64/panic.cpp
    src/arch/x86_64/paging.cpp
    src/arch/x86_64/physical_allocator.cpp
    src/arch/x86_64/kpci.cpp
    src/arch/x86_64/timer.cpp
    src/arch/x86_64/debug.cpp
    src/arch/x86_64/acpi/acpi.cpp
    src/arch/x86_64/kcpuid.asm
    src/arch/x86_64/kcpuid.cpp
    src/video/video.cpp
    src/liballoc/liballoc.c
    src/liballoc/liballoc_internal.cpp
    src/logging.cpp
    src/kstring.cpp
    src/kassert.cpp
    src/kernel.cpp
)

target_compile_options(
    kernel PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${KERNEL_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${KERNEL_CXX_FLAGS}>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(
        kernel PRIVATE
        -DKERNEL_DEBUG
    )
endif()

target_link_options(
    kernel PRIVATE
    ${KERNEL_LINKER_FLAGS}
)

target_include_directories(
    kernel PRIVATE
    include
    include/arch/x86_64
)

target_link_libraries(
    kernel PRIVATE
    lai
)