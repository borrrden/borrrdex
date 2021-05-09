#include <acpi/common.h>

// The Extended System Descriptor Table, which holds the addreses
// of all other system descriptor tables on the system (Root
// System Descriptor Table also exists, for 32-bit implementations)
// ACPI 6.4 p.146
typedef struct {
	acpi_desc_header_t h;
	void* entries[0];
} __attribute__((packed)) xsdt_t;