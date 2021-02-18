#include "Process.h"
#include "thread.h"

static int setup_new_process(tid_t thread, const char* executable, const char**argv_src,
    uint64_t* entry_point, uint64_t* stack_top) {
    tid_t current_thread = thread_get_current();
    thread_table_t* thread_entry = thread_get_thread_entry(thread);
    PageTableManager* current_ptm = thread_get_thread_entry(current_thread)->pageTableManager;
    return 0;
}

Process::Process(const char* executable, const char** argv) {
    
}