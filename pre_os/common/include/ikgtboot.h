/****************************************************************************
* Copyright (c) 2015 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
****************************************************************************/

#ifndef _IKGT_BOOT_H_
#define _IKGT_BOOT_H_

#define IKGT_BOOT_HEADER_MAGIC        0x6d6d76656967616d
#define RT_MEM_BASE                   0x12C00000 /*Hardcoded address for runtime address:300 MB*/
#define LDR_MEM_BASE                  0x10000000 /*Hardcoded address for load address:256 MB*/
#define SCAN_MAX_IMAGE_SIZE           0x100000   /*Scan Max image size assumed to be 1 MB*/
#define BOOT_HDR_VERSION              1
#define IKGT_BOOTLOADER_MAGIC         0x4857b815
#define FLASH_PAGE_SIZE_EFI           2048 /*page size for flashing blocks for flashing images */
#define __KERNEL_32_CS                0x10

#ifndef ASM_FILE

/* common types */
#define CONST
#define IN
/*  Ikgt boot header:
 *  This header is used to send in components needed by EFI bootloader
 *  to load EVMM.
 *  NOTE:
 *  1. The header address is 8-byte aligned in starter.
 *  2. Boot loader, EFI loader or kernelflinger  searches
 *     this header with a 64bit magic value.
 *  3. CONST - the field is populated by packer or during compilation.
 *  4. Boot loader should copy the whole image package to the
 *     address of ldr_mem_base, and then call into
 *     the entry of entry64_offset+ldr_mem_base.
 */
typedef struct {
    /* a 64bit magic value */
    CONST uint64_t magic;

    /* size of this structure */
    CONST uint32_t size;
    /* To be used later for version info */
    CONST uint32_t version;

    /* reserved to be cleaned out */
    CONST uint32_t reserved1;

    /* 64bit entry offset */
    CONST uint32_t entry64_offset;

    /* No longer being used, reserved to be cleaned out */
    CONST uint32_t reserved2;
    CONST uint32_t reserved3;

    /* boot loader will allocate it with this size,
    and populate rt_mem_base (make sure it < 4G),
    the rt_mem_size is sizeof(xmon_runtime_memory_layout_t)
    */
    CONST uint32_t rt_mem_base;
    CONST uint32_t rt_mem_size;

    /* boot loader will allocate it with this size,
    and populate ldr_mem_base (make sure it < 4G),
    the ldr_mem_size is sizeof(xmon_loader_memory_layout_t)
    */
    CONST uint32_t ldr_mem_base;
    CONST uint32_t ldr_mem_size;

    /* size of image package after 0-padding to 4k aligned */
    CONST uint32_t image_size;

} ikgt_loader_boot_header_t;

/*   Platform info structure to store the EFI memory map and any future platform info
 *   used for launching trusty
 */
typedef struct {
    /* EFI memory map address */
    uint32_t   memmap_addr;
    /* EFI memory map size */
    uint32_t   memmap_size;
    /* Address of load-time region where image is actually loaded */
    uint32_t   load_addr;
    /* Address of allocated runtime memory region */
    uint32_t   run_addr;
} ikgt_platform_info_t;


#undef CONST
#undef IN

#endif
#endif
