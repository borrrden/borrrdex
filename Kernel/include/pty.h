#pragma once

#include <stdint.h>
#include <device.h>
#include <klist.hpp>
#include <char_buffer.h>
#include <fs/fs_watcher.h>
#include <fs/directory_entry.h>

typedef unsigned int cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

namespace pty {
    constexpr uint16_t TIOCGATTR     = 0xB301;
    constexpr uint16_t TIOCSATTR     = 0xB302;
    constexpr uint16_t TIOCFLUSH     = 0xB303;
    constexpr uint16_t TIOCGWINSZ    = 0x5413;
    constexpr uint16_t TIOCSWINSZ    = 0x5414;

    constexpr uint8_t NCCS          = 11;

    // c_lflag
    constexpr uint16_t ECHO     = 0x0001;
    constexpr uint16_t ECHOE    = 0x0002;
    constexpr uint16_t ECHOK    = 0x0004;
    constexpr uint16_t ECHONL   = 0x0008;
    constexpr uint16_t ICANON   = 0x0010;
    constexpr uint16_t IEXTEN   = 0x0020;
    constexpr uint16_t ISIG     = 0x0040;
    constexpr uint16_t NOFLSH   = 0x0080;
    constexpr uint16_t TOSTOP   = 0x0100;

    // Flush constants
    constexpr uint8_t TCIFLUSH  = 1;
    constexpr uint8_t TCIOFLUSH = 2;
    constexpr uint8_t TCOFLUSH  = 3;

    enum device_type {
        worker,
        controller
    };
}

struct termios {
    tcflag_t c_ifflag;
    tcflag_t c_offlag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[pty::NCCS];
    speed_t ibaud;
    speed_t obaud;
};

struct winsz {
    unsigned short row_count;   // char
    unsigned short col_count;   // char
    unsigned short width;       // pixel
    unsigned short height;      // pixel
};

namespace pty {
    class pty;

    class pty_device : public devices::device {
    public:
        pty_device(const char* name, device_type type);

        ssize_t read(size_t, size_t, uint8_t*) override;
        ssize_t write(size_t, size_t, uint8_t*) override;

        int ioctl(uint64_t, uint64_t) override;

        void watch(fs::fs_watcher& watcher, int events) override;
        void unwatch(fs::fs_watcher& watcher) override;

        bool can_read() const override;

        inline void set_pty(pty* pty) { _pty = pty; }
    private:
        pty* _pty;
        device_type _type;
        fs::directory_entry dirent;
    };

    #define PTY_DELEGATE(type, x) if(type == device_type::worker) _worker.x; else _controller.x
    #define PTY_DELEGATE_RET(type, x) (type == device_type::worker) ? _worker.x : _controller.x

    class pty {
    public:
        pty(char* name);
        ~pty();

        bool echo() const { return _tios.c_lflag & ::pty::ECHO; }
        bool is_canonical() const { return _tios.c_lflag & ::pty::ICANON; }

        inline winsz get_winsz() const { return _winsz; }
        inline void set_winsz(winsz w) { _winsz = w; }

        inline termios get_attr() const { return _tios; }
        inline void set_attr(termios t) { _tios = t; }

        void set_ignore_backspace(device_type t, bool ignore) { PTY_DELEGATE(t, set_ignore_backspace(ignore)); }
        void flush(device_type t) { PTY_DELEGATE(t, flush()); }
        bool has_lines(device_type t) const { return PTY_DELEGATE_RET(t, has_lines()); }
        bool has_data(device_type t) const { return PTY_DELEGATE_RET(t, has_data()); }

        ssize_t controller_read(char* buffer, size_t size);
        ssize_t controller_write(const char* buffer, size_t size);
        ssize_t worker_read(char* buffer, size_t size);
        ssize_t worker_write(const char* buffer, size_t size);

        void watch_controller(fs::fs_watcher& watcher, int events);
        void watch_worker(fs::fs_watcher& watcher, int events);
        void unwatch_controller(fs::fs_watcher& watcher);
        void unwatch_worker(fs::fs_watcher& watcher);

        fs::fs_fd_t* open_worker() { return fs::open(&_worker_file, 0); }
        fs::fs_fd_t* open_controller() { return fs::open(&_controller_file, 0); }
    private:
        termios _tios;
        winsz _winsz;
        char* _name {nullptr};
        pty_device _controller_file;
        pty_device _worker_file;
        kstd::char_buffer _worker;
        kstd::char_buffer _controller;
        list<fs::fs_watcher *> _watching_worker;
        list<fs::fs_watcher *> _watching_controller;
    };

    pty* grant_pty();
}