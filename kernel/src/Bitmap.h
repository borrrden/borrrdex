#pragma once

#include <stddef.h>
#include <stdint.h>

class Bitmap {
public:
    Bitmap() : Bitmap(0, NULL) {}
    Bitmap(size_t size, uint8_t* buffer);

    bool operator[](uint64_t index) const;
    bool Set(uint64_t index, bool value);

    size_t GetSize() const;
private:
    size_t _size;
    uint8_t* _buffer;
};