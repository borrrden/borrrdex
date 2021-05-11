#include <fs/directory_entry.h>
#include <fs/fs_node.h>
#include <kstring.h>
#include <kassert.h>

namespace fs {
    directory_entry::directory_entry(fs_node* node, const char* name) {
        strncpy(_name, name, NAME_MAX);

        flags = file_to_dirent_flags(node->flags);
    }

    mode_t directory_entry::file_to_dirent_flags(mode_t flags) {
        switch(flags & FS_NODE_TYPE) {
            case FS_NODE_FILE:
                flags = DT_REG;
                break;
            case FS_NODE_DIRECTORY:
                flags = DT_DIR;
                break;
            case FS_NODE_CHARDEVICE:
                flags = DT_CHR;
                break;
            case FS_NODE_BLKDEVICE:
                flags = DT_BLK;
                break;
            case FS_NODE_SOCKET:
                flags = DT_SOCK;
                break;
            case FS_NODE_SYMLINK:
                flags = DT_LNK;
                break;
            default:
                assert(!"Invalid file flags!");
                break;
        }

        return flags;
    }
}