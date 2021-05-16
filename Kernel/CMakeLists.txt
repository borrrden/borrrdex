project(Kernel)
cmake_minimum_required(VERSION 3.10)
enable_language(ASM_NASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(
    KERNEL_C_FLAGS
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
    src/arch/x86_64/elf.cpp
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
    src/arch/x86_64/symbols.cpp
    src/arch/x86_64/scheduler.cpp
    src/arch/x86_64/scheduler.asm
    src/arch/x86_64/syscalls.cpp
    src/arch/x86_64/smp.cpp
    src/arch/x86_64/tss.cpp
    src/arch/x86_64/tss.asm
    src/arch/x86_64/thread.cpp
    src/video/video.cpp
    src/liballoc/liballoc.c
    src/liballoc/liballoc_internal.cpp
    src/mm/address_space.cpp
    src/mm/vm_object.cpp
    src/fs/filesystem.cpp
    src/fs/directory_entry.cpp
    src/fs/fs_node.cpp
    src/fs/fs_volume.cpp
    src/fs/tar.cpp
    src/logging.cpp
    src/kstring.cpp
    src/kassert.cpp
    src/kernel.cpp
    src/device.cpp
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
    ../LibC/subprojects/frigg/include
)

set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/smptrampoline.bin.o PROPERTIES
    EXTERNAL_OBJECT TRUE
    GENERATED TRUE
)

target_link_libraries(
    kernel PRIVATE
    lai
    ${CMAKE_CURRENT_BINARY_DIR}/smptrampoline.bin.o
)

add_custom_command(
    TARGET kernel
    PRE_BUILD
    COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/bintoelf.sh ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/x86_64/smptrampoline.asm ${CMAKE_CURRENT_BINARY_DIR}/smptrampoline.bin.o ${CMAKE_CURRENT_SOURCE_DIR}/include/arch/x86_64/ smptrampoline
)