#pragma once

#include <stdint.h>
#include <stddef.h>
#include <spinlock.h>
#include <types.h>

namespace kstd {
    class char_buffer {
        static constexpr uint16_t START_SIZE = 1024;
    public:
        char_buffer();
        ~char_buffer();

        inline void set_ignore_backspace(bool ignore) { _ignore_bs = ignore; }

        inline bool has_lines() const { return _lines > 0; }
        inline bool has_data() const { return _position > 0; }

        ssize_t write(const char* buffer, size_t size);
        ssize_t read(char* buffer, size_t size);

        void flush();
    private:
        lock_t _lock;
        size_t _position;
        size_t _size {START_SIZE};
        size_t _max_size = 0x200000;
        int _lines;
        char* _buffer;
        bool _ignore_bs {false};
    };
}