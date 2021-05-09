#include <acpi/acpi.h>
#include <panic.h>
#include <vector>

namespace acpi {
    acpi_xsdp_t* desc;
    std::vector<int_source_override_t *> isos;

    void set_rsdp(acpi_xsdp_t* p) {
        desc = p;
    }

    void initialize() {
        if(!desc) {
            // TODO: Search in low memory
            const char* panicReasons[]{"System not ACPI Compliant."};
            kernel_panic(panicReasons, 1);
            __builtin_unreachable();
        }

        
    }
}