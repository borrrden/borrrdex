#pragma once

#ifndef __cplusplus
#error C++ only
#endif

#include <cstdint>

constexpr int8_t VFS_OK             = 0;
constexpr int8_t VFS_NOT_SUPPORTED  = -1;
constexpr int8_t VFS_ERROR          = -2;
constexpr int8_t VFS_INVALID_PARAMS = -3;
constexpr int8_t VFS_NOT_OPEN       = -4;
constexpr int8_t VFS_NOT_FOUND      = -5;
constexpr int8_t VFS_NO_SUCH_FS     = -6;
constexpr int8_t VFS_LIMIT          = -7;
constexpr int8_t VFS_IN_USE         = -8;
constexpr int8_t VFS_UNUSABLE       = -9;

constexpr uint8_t VFS_NAME_LENGTH = 16;
constexpr uint16_t VFS_PATH_LENGTH = 256;

typedef int openfile_t;

struct fs_struct {
    void* internal;

    int (*unmount)(fs_struct*);

    int (*open)(fs_struct*, const char*);

    int (*close)(fs_struct*, int);

    int (*read)(fs_struct*, int, void*, int, int);

    int (*write)(fs_struct*, int, void*, int, int);

    int (*create)(fs_struct*, const char*, int, int);

    int (*remove)(fs_struct*, const char*);

    int (*getfree)(fs_struct*);

    int (*filecount)(fs_struct*, const char*);

    int (*file)(fs_struct*, const char*, int, char *);
};

class VirtualFilesystem {
    
};