#include <storage/gpt.h>
#include <liballoc/liballoc.h>
#include <logging.h>
#include <debug.h>
#include <kmath.h>

namespace gpt {
    int parse(devices::disk_device* disk) {
        gpt_header_t* header = (gpt_header_t *)malloc(disk->block_size());
        if(int read = disk->read_disk_block(1, 1, header) < 0) {
            log::info("[gpt] Disk error %d", read);
            return -1;
        }

        if(header->signature != GPT_HEADER_SIGNATURE || header->part_num == 0) {
            return 0;
        }

        IF_DEBUG(debug_level_partitions >= debug::LEVEL_NORMAL, {
            log::info("Found GPT header partitions: %d, Entry Size: %d", header->part_num, header->part_entry_size);
        })

        uint64_t table_lba = header->partition_table_lba;
        int part_num = header->part_num;
        gpt_entry_t* partition_table = (gpt_entry_t *)malloc(kstd::round_up(part_num * header->part_entry_size, disk->block_size()));
        uint32_t sectors = kstd::intervals_needed(part_num * header->part_entry_size, disk->block_size());
        if(int read = disk->read_disk_block(table_lba, sectors, partition_table) < 0) {
            return -1;
        }

        for(int i = 0; i < part_num; i++) {
            gpt_entry_t* entry = &partition_table[i];
            IF_DEBUG(debug_level_partitions >= debug::LEVEL_NORMAL, {
                log::info("Found GPT Partition of size %d MB", (entry->end_lba - entry->start_lba) * 512 / 1024 / 1024);
            })

            if((entry->end_lba - entry->start_lba)) {
                auto* part = new devices::partition_device(entry->start_lba, entry->end_lba, disk, entry->type_guid);
                disk->add_partition(part);
            }
        }

        free(partition_table);
        free(header);
        return 1;
    }
}