#pragma once

struct keyboard_state {
    bool caps:1;
    bool control:1;
    bool shift:1;
    bool alt:1;
};

class input_manager {
public:
    typedef void(*callback_t)(int key);

    input_manager(callback_t cb)
        :_cb(cb)
    {
        
    }
    
    void poll();

private:
    keyboard_state _keyboard {0};
    callback_t _cb;
};