#pragma once

#include <thread.h>
#include <frg/intrusive.hpp>
#include <lock.h>

namespace kstd {
    constexpr uint16_t DATASTREAM_BUFSIZE_DEFAULT = 1024;

    typedef struct {
        uint8_t* data;
        size_t len;
    } stream_packet_t;

    class stream {
    public:
        virtual ~stream() {}

        virtual void wait() = 0;

        virtual int64_t read(void* buffer, size_t len) = 0;
        virtual int64_t write(void* buffer, size_t len) = 0;

        virtual int64_t pos() { return 0; }
        virtual bool empty() { return true; }
    protected:
        struct thread_item {
            threading::thread* thread;
            frg::default_list_hook<thread_item> hook;
        };

        frg::intrusive_list<thread_item, frg::locate_member<thread_item, frg::default_list_hook<thread_item>, &thread_item::hook>> _waiting;
    };

    class data_stream final : public stream {
    public:
        data_stream(size_t buf_size);
        ~data_stream();

        void wait() override;

        int64_t read(void* buffer, size_t len) override;
        int64_t write(void* buffer, size_t len) override;

        int64_t pos() override { return _buffer_pos; }
        bool empty() override;
    private:
        lock_t _stream_lock;
        size_t _buffer_size;
        size_t _buffer_pos;
        uint8_t* _buffer {nullptr};
    };

    class packet_stream final : public stream {
    public:
        void wait() override;

        int64_t read(void* buffer, size_t len) override;
        int64_t write(void* buffer, size_t len) override;

        bool empty() override;
    private:
        struct packet_item {
            stream_packet_t packet;
            frg::default_list_hook<packet_item> hook;
        };

        frg::intrusive_list<packet_item, frg::locate_member<packet_item, frg::default_list_hook<packet_item>, &packet_item::hook>> _packets;
    };
}