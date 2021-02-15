#include "Bitmap.h"

Bitmap::Bitmap(size_t size, uint8_t* buffer)
    :_size(size)
    ,_buffer(buffer)
    {
        
        for(size_t i = 0; i < size; i++) {
            buffer[i] = 0;
        }
    }

bool Bitmap::operator[](uint64_t index) const {
    if(index > _size * 8) {
        return false;
    }

    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;

    return _buffer[byteIndex] & bitIndexer;
}

bool Bitmap::Set(uint64_t index, bool value) {
    if(index > _size * 8) {
        return false;
    }

    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;

    if(value) {
        _buffer[byteIndex] |= bitIndexer;
    } else {
        _buffer[byteIndex] &= ~bitIndexer;
    }

    return true;
}

size_t Bitmap::GetSize() const {
    return _size;
}