#include "Clock.h"
#include "../cstr.h"

const char* dow[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void print_padded(BasicRenderer* r, uint64_t val) {
    if(val < 10) {
        r->Print("0");
    }

    r->Print(to_string(val));
}

void Clock::tick(datetime_t* dt) {
    Point prev = _renderer->CursorPosition;
    _renderer->CursorPosition.x = _renderer->Width() - 8 * 23;
    _renderer->CursorPosition.y = 0;
    _renderer->Print(dow[dt->weekday - 1]);
    _renderer->Print(" ");
    _renderer->Print(to_string((uint64_t)dt->year));
    _renderer->Print("/");
    print_padded(_renderer, dt->month);
    _renderer->Print("/");
    print_padded(_renderer, dt->day);
    _renderer->Print(" ");
    print_padded(_renderer, dt->hours);
    _renderer->Print(":");
    print_padded(_renderer, dt->minutes);
    _renderer->Print(":");
    print_padded(_renderer, dt->seconds);

    _renderer->CursorPosition = prev;
}