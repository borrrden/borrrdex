#pragma once

class cpu;
namespace smp {
    void initialize();

    unsigned get_proc_count();
    cpu* get_cpu(unsigned index);
}