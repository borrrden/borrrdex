#pragma once

#include <acpi/fadt.h>
#include <acpi/hpet.h>
#include <acpi/madt.h>
#include <acpi/rsdp.h>
#include <acpi/rsdt.h>
#include <acpi/xsdt.h>

#include <acpispec/tables.h>

namespace acpi {
    void set_rsdp(acpi_xsdp_t* p);
}