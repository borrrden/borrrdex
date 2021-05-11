#include <fs/fs_volume.h>

namespace fs {
    void fs_volume::set_volume_id(volume_id_t id) {
        _volume_id = id;
    }
}