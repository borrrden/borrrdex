#pragma once

#include <stdint.h>
#include <stddef.h>
#include "uefi/EfiMemory.h"

uint64_t GetMemorySize(EFI_MEMORY_DESCRIPTOR* mMap, uint64_t mMapEntries, uint64_t mDescriptorSize);
void memset(void* start, uint8_t value, uint64_t c);
int memcmp(const void* aptr, const void* bptr, size_t n);