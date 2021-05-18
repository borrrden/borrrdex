#include <char_buffer.h>
#include <liballoc/liballoc.h>
#include <lock.h>

namespace kstd {
    char_buffer::char_buffer() 
        :_buffer((char *)malloc(_size))
    {

    }

    char_buffer::~char_buffer() {
        free(_buffer);
    }

    ssize_t char_buffer::write(const char* b, size_t size) {
        if(_position + _size > _max_size) {
            size = _max_size - _position;
        }

        if(size == 0) {
            return 0;
        }

        lock l(_lock);
        if((_position + size) > _size) {
            _buffer = (char *)realloc(_buffer, _position + _size + 128);
        }

        ssize_t written = 0;
        for(unsigned i = 0; i < size; i++) {
            if(b[i] == '\b' && !_ignore_bs) {
                if(_position > 0) {
                    _position--;
                    written++;
                }

                continue;
            } else {
                _buffer[_position++] = b[i];
                written++;
            }

            if(!b[i] || b[i] == '\n') {
                _lines++;
            }
        }

        return written;
    }

    ssize_t char_buffer::read(char* b, size_t size) {
        lock l(_lock);
        if(size > _position) {
            size = _position;
        }

        if(size == 0) {
            return 0;
        }

        for(unsigned i = 0; i < size; i++) {
            if(_buffer[i] == 0) {
                _lines--;
                continue;
            }

            b[i] = _buffer[i];
            if(_buffer[i] == '\n') {
                _lines--;
            }
        }

        for(unsigned i = 0; i < _size - size; i++) {
            _buffer[i] = _buffer[size + 1];
        }

        _position -= size;
        return size;
    }

    void char_buffer::flush() {
        lock l(_lock);
        _position = 0;
        _lines = 0;
    }
}