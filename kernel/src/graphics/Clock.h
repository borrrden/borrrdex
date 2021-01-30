#pragma once

#include "BasicRenderer.h"
#include "../io/rtc.h"

class Clock {
    public:
        Clock() : Clock(GlobalRenderer) { }
        Clock(BasicRenderer* renderer)
            :_renderer(renderer)
        {

        }

        void tick(datetime_t* dt);

        uint16_t get_update_ticks() const { return rtc_get_interrupt_frequency_hz() / 4; }
    private:
        BasicRenderer* _renderer;
};