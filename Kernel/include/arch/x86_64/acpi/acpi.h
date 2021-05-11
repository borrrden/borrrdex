#pragma once

#include <acpi/fadt.h>
#include <acpi/hpet.h>
#include <acpi/madt.h>
#include <acpi/mcfg.h>
#include <acpi/rsdp.h>
#include <acpi/rsdt.h>
#include <acpi/xsdt.h>

#include <acpispec/tables.h>
#include <klist.hpp>

namespace acpi {
    void initialize();
    
    void set_rsdp(acpi_xsdp_t* p);

    const mcfg_t* get_mcfg();
    const list<int_source_override_t *>* int_source_overrides();

    int get_proc_count();
    const uint8_t* get_processors();
    void disable_smp();
}