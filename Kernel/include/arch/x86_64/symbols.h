#pragma once

#include <stdint.h>
#include <fs/fs_node.h>

struct kernel_symbol {
    uintptr_t address;
    const char* mangled_name;
};

void load_symbols(fs::fs_node* node);

int resolve_kernel_symbol(const char* mangled_name, kernel_symbol* symbol);

void add_kernel_symbol(kernel_symbol* sym);
void remove_kernel_symbol(const char* mangled_name);