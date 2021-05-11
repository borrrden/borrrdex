#include <symbols.h>
#include <kstring.h>
#include <logging.h>
#include <debug.h>
#include <kassert.h>

#include <frg/hash_map.hpp>
#include <frg/allocation.hpp>
#include <frg/std_compat.hpp>
#include <frg/string.hpp>

static frg::hash_map<frg::string_view, kernel_symbol, frg::hash<frg::string_view>, frg::stl_allocator> symbol_map(frg::hash<frg::string_view>{});

void load_symbols(fs::fs_node* node) {
    constexpr unsigned buffer_size = 4096;
    char buffer[buffer_size];
    unsigned buffer_pos = 0;

    ssize_t off = 0;
    ssize_t read = 0;
    char* saved, *address_end;;
    while((read = fs::read(node, off, buffer_size, buffer)) > 0) {
        buffer_pos = 0;
        strtok_r(buffer, "\n", &saved);
        while(saved) {
            char* line = buffer + buffer_pos;
            *(saved - 1) = 0; // replace line end with null
            if(strnlen(line, 1024) < sizeof(uintptr_t) * 2 + 3) {
                // Too short, invalid line
                goto end;
            }

            address_end = strchr(line, ' ');
            if(!address_end) {
                // Invalid? Empty?
                goto end;
            }

            kernel_symbol sym;
            if(hex_to_pointer(line, sizeof(uintptr_t) * 2, sym.address)) {
                // Invalid address string
                goto end;
            }

            sym.mangled_name = strdup(address_end + 3);
            log::debug(debug_level_symbols, debug::LEVEL_VERBOSE, "Found kernel symbol: %llx, name: '%s'",
                sym.address, sym.mangled_name);
            symbol_map.insert(frg::string_view(sym.mangled_name), sym);

        end:
            buffer_pos += saved - line;
            strtok_r(nullptr, "\n", &saved);
        }

        if(!buffer_pos) break;
        off += buffer_pos;
    }

    assert(read >= 0);
}

int resolve_kernel_symbol(const char* mangled_name, kernel_symbol* sym) {
    auto existing = symbol_map.find(mangled_name);
    if(existing == symbol_map.end()) {
        return -1;
    }
    
    *sym = existing->get<1>();
    return 0;
}

void add_kernel_symbol(kernel_symbol* sym) {
    assert(!symbol_map.find(sym->mangled_name));

    symbol_map.insert(frg::string_view(sym->mangled_name), *sym);
}

void remove_kernel_symbol(const char* mangled_name) {
    symbol_map.remove(frg::string_view(mangled_name));
}