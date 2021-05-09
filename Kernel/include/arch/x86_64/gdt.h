#pragma once

#include <stdint.h>

/**
 * Accessed bit. Just set to 0. The CPU sets this to 1 when the segment is accessed. 
 */
constexpr uint8_t GDT_DESC_ACCESS       = 0x01;

/**
 * Code Segments - 1 for read access, 0 for no access
 * Data Segments - 1 for write access, 0 for no access
 */
constexpr uint8_t GDT_DESC_READWRITE    = 0x02;

/**
 * Code Segments [Conformance]:
 *      0 means this segment can only be jumped into from the same privilege level
 *      1 means *lower* privilege levels are also ok (i.e. 3 -> 2 is ok, but 0 -> 2 is not)
 * 
 * Data Segments [Direction]:
 *      0 means The segment grows upwards, 1 means it grows downwards
 */
constexpr uint8_t GDT_DESC_DC           = 0x04;

/**
 * If 1, this is a code segment, 0 means data segment
 */
constexpr uint8_t GDT_DESC_EXECUTABLE   = 0x08;

/**
 * If 1, this is either a code or data segment, 0 means system segment
 */
constexpr uint8_t GDT_DESC_CODEDATA     = 0x10;

/**
 * If this value is set, the segment is ring level 3 (0 if not set)
 */
constexpr uint8_t GDT_DESC_DPL          = 0x60;

/**
 * This value must be 1 on all valid selectors
 */
constexpr uint8_t GDT_DESC_PRESENT      = 0x80;

/**
 * When this mask is applied, the hi portion of limit is filtered out,
 * and only the leftover flags remains set
 */
constexpr uint8_t GDT_GRAN_LIMITHIMAST  = 0x0F;

/**
 * This bit is OS specific, and has no inherent meaning
 */
constexpr uint8_t GDT_GRAN_OS           = 0x10;

/**
 * If set, this is a 64-bit segment.  Otherwise it could be 32-bit
 * if the 32-bit bit is set, or 16-bit if neither are set
 */
constexpr uint8_t GDT_GRAN_64BIT        = 0x20;

/**
 * If set, this is a 32-bit segment.  Otherwise it could be 64-bit
 * if the 64-bit bit is set, or 16-bit if neither are set
 */
constexpr uint8_t GDT_GRAN_32BIT        = 0x40;

/**
 * If set, the granularity of the limit is measured in [4 KiB] pages, 
 * otherwise it is measured in bytes
 */
constexpr uint8_t GDT_GRAN_4K           = 0x80;

// Predefined selectors
constexpr uint8_t GDT_SELECTOR_KERNEL_CODE  = (0x01 << 3);
constexpr uint8_t GDT_SELECTOR_KERNEL_DATA  = (0x02 << 3);
constexpr uint8_t GDT_SELECTOR_USER_CODE    = (0x05 << 3);
constexpr uint8_t GDT_SELECTOR_USER_DATA    = (0x04 << 3);