#include <fs/pipe.h>
#include <fs/fs_blocker.h>
#include <scheduler.h>
#include <kmath.h>
#include <errno.h>
using namespace kstd;

namespace fs {
    unix_pipe::unix_pipe(int end, ref_counted<data_stream> stream)
        :_end(static_cast<decltype(_end)>(end))
        ,_stream(stream)
    {

    }

    ssize_t unix_pipe::read(size_t off, size_t size, uint8_t* buffer) {
        if(_end != ReadEnd) {
            return -ESPIPE;
        }

        if(!_widowed && _stream->empty()) {
            fs::fs_blocker bl(this, size);
            if(!scheduler::get_current_thread()->block(&bl)) {
                return -EINTR;
            }
        }

        size = min(size, (size_t)_stream->pos());
        return _stream->read(buffer, size);
    }

    ssize_t unix_pipe::write(size_t off, size_t size, uint8_t* buffer) {
        if(_end != WriteEnd) {
            return -ESPIPE;
        }

        if(_widowed) {
            return -EPIPE;
        }

        ssize_t ret = _stream->write(buffer, size);

        {
            kstd::lock l(_watching_lock);
            for(auto w : _other_end->_watching) {
                w->watcher.signal();
                delete w;
            }

            _other_end->_watching.clear();
        }

        return ret;
    }

    void unix_pipe::watch(fs::fs_watcher& watcher, int events) {
        // Not implemented
    }

    void unix_pipe::unwatch(fs::fs_watcher& watcher) {
        // Not implemented
    }

    void unix_pipe::close() {
        _handle_count--;

        if(_handle_count <= 0) {
            if(_other_end) {
                _other_end->_widowed = true;
            }

            delete this;
        }
    }
    
    void unix_pipe::create_pipe(unix_pipe*& read, unix_pipe*& write) {
        ref_counted<data_stream> stream = new data_stream(kstd::DATASTREAM_BUFSIZE_DEFAULT);
        read = new unix_pipe(unix_pipe::ReadEnd, stream);
        write = new unix_pipe(unix_pipe::WriteEnd, stream);

        read->_other_end = write;
        write->_other_end = read;
    }
}