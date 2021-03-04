#pragma once

#include <cstdint>
#include <cstddef>

void heap_init(void* virtualAddress, size_t numPages);
void heap_expand(size_t length);

void* kmalloc(size_t size);
void kfree(void* address);