#include <pty.h>
#include <kstring.h>
#include <fs/fs_blocker.h>
#include <scheduler.h>
#include <thread.h>
#include <abi-bits/errno.h>

static cc_t c_cc_default[pty::NCCS]{
    4,          // VEOF
    '\n',       // VEOL
    '\b',       // VERASE
    0,          // VINTR
    0,          // VKILL
    0,          // VMIN
    0,          // VQUIT
    0,          // VSTART
    0,          // VSTOP
    0,          // VSUSP
    0,          // VTIME
};

namespace pty {
    list<pty *> ptys;
    char next_pty = '0';

    pty::pty(char* name) 
        :_name(name)
        ,_controller_file(name, device_type::controller)
        ,_worker_file(name, device_type::worker)
    {
        _worker_file.flags = fs::FS_NODE_CHARDEVICE;
        _controller.set_ignore_backspace(true);
        _worker.set_ignore_backspace(false);
        _tios.c_lflag = ECHO | ICANON;

        _controller_file.set_pty(this);
        _worker_file.set_pty(this);

        for(int i = 0; i < NCCS; i++) {
            _tios.c_cc[i] = c_cc_default[i];
        }

        ptys.add(this);
    }

    pty::~pty() {
        if(_name) {
            free(_name);
        }
    }

    ssize_t pty::controller_read(char* b, size_t size) {
        return _controller.read(b, size);
    }

    ssize_t pty::controller_write(const char* b, size_t size) {
        ssize_t ret = _worker.write(b, size);
        if(_worker_file.blocked()->size() > 0) {
            if(is_canonical()) {
                _worker_file.lock();
                while(_worker.has_lines() && _worker_file.blocked()->size() > 0) {
                    _worker_file.blocked()->get(0)->unblock();
                }
                _worker_file.unlock();
            } else {
                _worker_file.lock();
                while(_worker.has_data() && _worker_file.blocked()->size() > 0) {
                    _worker_file.blocked()->get(0)->unblock();
                }
                _worker_file.unlock();
            }
        }

        if(echo() && ret) {
            for(unsigned i = 0; i < size; i++) {
                if(b[i] == '\e') {
                    _controller.write("^[", 2);
                } else {
                    _controller.write(&b[i], 1);
                }
            }
        }

        if(is_canonical()) {
            if(_worker.has_lines()) {
                while(_watching_worker.size() > 0) {
                    _watching_worker.remove_at(0)->signal();
                }
            }
        } else {
            if(_worker.has_data()) {
                while(_watching_worker.size() > 0) {
                    _watching_worker.remove_at(0)->signal();
                }
            }
        }

        return ret;
    }

    ssize_t pty::worker_read(char* b, size_t size) {
        threading::thread* t = scheduler::get_current_thread();
        while(is_canonical() && _worker.has_lines()) {
            fs::fs_blocker blocker(&_worker_file);
            if(!t->block(&blocker)) {
                return -EINTR;
            }
        }

        while(!is_canonical() && _worker.has_data()) {
            fs::fs_blocker blocker(&_worker_file);
            if(!t->block(&blocker)) {
                return -EINTR;
            }
        }

        return _worker.read(b, size);
    }

    ssize_t pty::worker_write(const char* b, size_t size) {
        ssize_t ret = _controller.write(b, size);
        _controller_file.lock();
        while(_controller.has_data() && _controller_file.blocked()->size() > 0) {
            _controller_file.blocked()->get(0)->unblock();
        }
        _controller_file.unlock();

        if(_controller.has_data()) {
            while(_watching_controller.size() > 0) {
                _watching_controller.remove_at(0)->signal();
            }
        }

        return ret;
    }

    void pty::watch_controller(fs::fs_watcher& watcher, int events) {
        if(!(events & fs::POLLIN) || _controller_file.can_read()) {
            watcher.signal();
            return;
        }

        _watching_controller.add(&watcher);
    }

    void pty::watch_worker(fs::fs_watcher& watcher, int events) {
        if(!(events & fs::POLLIN) || _worker_file.can_read()) {
            watcher.signal();
            return;
        }

        _watching_worker.add(&watcher);
    }

    void pty::unwatch_controller(fs::fs_watcher& watcher) {
        _watching_controller.remove(&watcher);
    }

    void pty::unwatch_worker(fs::fs_watcher& watcher) {
        _watching_worker.remove(&watcher);
    }

    pty_device::pty_device(const char* name, device_type type) 
        :devices::device(devices::device_type::unix_pseudo_terminal, name)
        ,_type(type)
    {
        dirent.node = this;
        _device_name = "UNIX Pseudoterminal Device";
    }

    ssize_t pty_device::read(size_t offset, size_t size, uint8_t* buffer) {
        assert(_pty);
        assert(_type == device_type::controller || _type == device_type::worker);

        if(_type == device_type::worker) {
            return _pty->worker_read((char *)buffer, size);
        }

        if(__builtin_expect(_type == device_type::controller, 1)) {
            return _pty->controller_read((char *)buffer, size);
        }

        return -EBADF;
    }

    using pty_write_func = ssize_t(pty::pty::*)(const char*, size_t);
    ssize_t pty_device::write(size_t offset, size_t size, uint8_t* buffer) {
        assert(_pty);
        assert(_type == device_type::controller || _type == device_type::worker);

        pty_write_func write_func;

        if(_type == device_type::worker) {
            write_func = &pty::worker_write;
        } else if(__builtin_expect(_type == device_type::controller, 1)) {
            write_func = &pty::controller_write;
        } else {
            return -EBADF;
        }

        ssize_t written = (_pty->*(write_func))((const char*)buffer, size);
        if(written < 0 || written == size) {
            return written;
        }

        // Buffer was full, keep trying
        buffer += written;
        while(written < size) {
            ssize_t ret = (_pty->*(write_func))((const char*)buffer, size);
            if(ret < 0) {
                return written;
            }

            written += ret;
            buffer += ret;
        }

        return written;
    }

    int pty_device::ioctl(uint64_t cmd, uint64_t arg) {
        assert(_pty);

        switch(cmd) {
            case TIOCGWINSZ:
                *((winsz*)arg) = _pty->get_winsz();
                break;
            case TIOCSWINSZ:
                _pty->set_winsz(*((winsz*)arg));
                break;
            case TIOCGATTR:
                *((termios*)arg) = _pty->get_attr();
                break;
            case TIOCSATTR:
                _pty->set_attr(*((termios *)arg));
                _pty->set_ignore_backspace(device_type::worker, !_pty->is_canonical());
                break;
            case TIOCFLUSH:
                if(arg == TCIFLUSH || arg == TCIOFLUSH) {
                    _pty->flush(device_type::worker);
                }

                if(arg == TCOFLUSH || arg == TCIOFLUSH) {
                    _pty->flush(device_type::controller);
                }

                break;
        }

        return 0;
    }

    void pty_device::watch(fs::fs_watcher& watcher, int events) {
        if(_type == device_type::controller) {
            _pty->watch_controller(watcher, events);
        } else if(__builtin_expect(_type == device_type::worker, 1)) {
            _pty->watch_worker(watcher, events);
        }

        assert(!"Invalid PTY device type");
    }

    void pty_device::unwatch(fs::fs_watcher& watcher) {
        if(_type == device_type::controller) {
            _pty->unwatch_controller(watcher);
        } else if(__builtin_expect(_type == device_type::worker, 1)) {
            _pty->unwatch_worker(watcher);
        }

        assert(!"Invalid PTY device type");
    }

    bool pty_device::can_read() const {
        if(_type == device_type::controller) {
            return _pty->has_data(_type);
        } 
        
        if(__builtin_expect(_type == device_type::worker, 1)) {
            return _pty->is_canonical() ? _pty->has_lines(_type) : _pty->has_data(_type);
        }

        assert(!"Invalid PTY device type");
    }

    pty* grant_pty() {
        char* name = (char *)malloc(5);
        name[3] = next_pty++;
        name[4] = 0;
        strncpy(name, "pty", 3);
        return new pty(name);
    }
}