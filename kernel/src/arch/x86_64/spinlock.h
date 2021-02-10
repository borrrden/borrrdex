#pragma once

class Spinlock {
public:
    Spinlock() {}

    void acquire();
    void release();
    
private:
    int _handle{0};
};