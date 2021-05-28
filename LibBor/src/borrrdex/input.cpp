#include <borrrdex/core/input.h>

#include <fcntl.h>

namespace borrrdex {
    static int keyboard_fd = 0;

    ssize_t poll_keyboard(uint8_t* buffer, size_t count) {
        if(!keyboard_fd) {
            keyboard_fd = open("/dev/kbd0", O_RDONLY);
        }

        return read(keyboard_fd, buffer, count);
    }
}