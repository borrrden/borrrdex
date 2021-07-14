#include <stream.h>
#include <liballoc/liballoc.h>
#include <kmath.h>
#include <kstring.h>
#include <scheduler.h>

namespace kstd {
    data_stream::data_stream(size_t buf_size) 
        :_buffer_size(buf_size)
        ,_buffer_pos(0)
        ,_buffer((uint8_t *)malloc(buf_size))
    {

    }

    data_stream::~data_stream() {
        free(_buffer);
    }

    int64_t data_stream::read(void* data, size_t len) {
        kstd::lock l(_stream_lock);
        len = kstd::min(len, _buffer_pos);
        if(len == 0) {
            return 0;
        }

        memcpy(data, _buffer, len);
        memcpy(_buffer, _buffer + len, _buffer_pos - len);
        _buffer_pos -= len;
        return len;
    }

    int64_t data_stream::write(void* data, size_t len) {
        kstd::lock l(_stream_lock);
        if(_buffer_pos + len >= _buffer_size) {
            while(_buffer_pos + len >= _buffer_size) {
                _buffer_size *= 2;
            }

            _buffer = (uint8_t *)realloc(_buffer, _buffer_size);
        }

        memcpy(_buffer + _buffer_pos, data, len);
        _buffer_pos += len;
        return len;
    }

    bool data_stream::empty() {
        return !_buffer_pos;
    }

    void data_stream::wait() {
        while(empty()) {
            scheduler::yield();
        }
    }

    int64_t packet_stream::read(void* data, size_t len) {
        if(_packets.empty()) {
            return 0;
        }

        auto pkt = _packets.pop_front();
        if(len > pkt->packet.len) {
            len = pkt->packet.len;
        }

        memcpy(data, pkt->packet.data, len);
        free(pkt->packet.data);
        free(pkt);
        return len;
    }

    int64_t packet_stream::write(void* data, size_t len) {
        stream_packet_t pkt;
        pkt.len = len;
        pkt.data = (uint8_t *)malloc(len);
        memcpy(pkt.data, data, len);

        _packets.push_back(new packet_item {
            pkt
        });

        return pkt.len;
    }

    bool packet_stream::empty() {
        return _packets.empty();
    }

    void packet_stream::wait() {
        while(empty()) {}
    }
}