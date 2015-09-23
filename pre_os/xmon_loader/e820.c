/*******************************************************************************
* Copyright (c) 2015 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include "mon_defs.h"
#include "mon_arch_defs.h"
#include "mon_startup.h"
#include "xmon_desc.h"
#include "common.h"
#include "screen.h"
#include "memory.h"
#include "e820.h"
#include "ikgtboot.h"
/* data struct definition for EFI memory map 
   xmon loader parses the e820 from EFI mmap struct */
#define EFI_E820_SIZE_PADDING           8
typedef enum {
    EfiAcpiAddressRangeMemory   = 1,
    EfiAcpiAddressRangeReserved = 2,
    EfiAcpiAddressRangeACPI     = 3,
    EfiAcpiAddressRangeNVS      = 4
} efi_acpi_memory_t;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct efi_memory_desc {
    uint32_t type;
    uint32_t pad;
    uint64_t phys_addr;
    uint64_t virt_addr;
    uint64_t num_pages;
    uint64_t attribute;
} efi_memory_desc_t;

extern void *CDECL mon_page_alloc(uint64_t pages);

/* Convert EFI acpi memory defs to E820 type defs */
efi_acpi_memory_t EfiTypeToE820Type(uint32_t Type)
{
	switch(Type)
	{
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			return EfiAcpiAddressRangeMemory;

		case EfiACPIReclaimMemory:
			return EfiAcpiAddressRangeACPI;

		case EfiACPIMemoryNVS:
			return EfiAcpiAddressRangeNVS;

		default:
			return EfiAcpiAddressRangeReserved;
    }
}


boolean_t get_e820_table_from_ibh(xmon_desc_t *xd, uint64_t *e820_addr)
{
	int15_e820_memory_map_t      *e820 = NULL;
	ikgt_platform_info_t         *platform_info = NULL;
	efi_memory_desc_t            *efi_map;
	uint32_t                     desc_size, map_size;
	uint32_t                     count, i;

	platform_info = (ikgt_platform_info_t *)(xd->initial_state.rbx);

	if (platform_info == NULL)
		return FALSE;

	map_size = platform_info->memmap_size;
	desc_size = sizeof(efi_memory_desc_t)+EFI_E820_SIZE_PADDING;
	count = map_size/desc_size;
	efi_map = (efi_memory_desc_t *) (uint64_t)platform_info->memmap_addr;

	e820 = (int15_e820_memory_map_t *)allocate_memory(PAGE_ALIGN_4K(map_size));
	if (e820 == NULL)
		return FALSE;

	for (i = 0; i < count; i++)
	{
		efi_memory_desc_t *next = (efi_memory_desc_t*)((uint64_t)efi_map+desc_size*i);

		e820->memory_map_entry[i].basic_entry.base_address = next->phys_addr;
		e820->memory_map_entry[i].basic_entry.length = next->num_pages * 0x1000;
		e820->memory_map_entry[i].basic_entry.address_range_type = EfiTypeToE820Type(next->type);
		e820->memory_map_entry[i].extended_attributes.bits.enabled = 1;
	}

	e820->memory_map_size = count * sizeof(int15_e820_memory_map_entry_ext_t);

	*e820_addr = (uint64_t)e820;

	return TRUE;
}

/* End of file */
