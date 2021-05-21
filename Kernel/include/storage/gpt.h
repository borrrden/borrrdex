#pragma once

#include <stdint.h>
#include <device.h>
#include <kguid.h>

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t size;
    uint32_t crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_lba;
    uint64_t last_lba;
    guid_t disk_guid;
    uint64_t partition_table_lba;
    uint32_t part_num;
    uint32_t part_entry_size;
    uint32_t part_entry_crc;
} __attribute__((packed)) gpt_header_t;

typedef struct {
    guid_t type_guid;
    guid_t partition_guid;
    uint64_t start_lba;
    uint64_t end_lba;
    uint64_t flags;
    char name[72];
} __attribute__((packed)) gpt_entry_t;

namespace gpt {
    constexpr uint64_t GPT_HEADER_SIGNATURE = 0x5452415020494645ULL;

    constexpr guid_t GUID_LINUX_FS_DATA =
        { 0x0FC63DAF, 0x8483, 0x4772, { 0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4 } };

    int parse(devices::disk_device* disk);
}