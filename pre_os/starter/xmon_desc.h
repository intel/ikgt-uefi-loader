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

#ifndef __XMON_DESC_H
#define __XMON_DESC_H

/*
 * Current design: xmon_pkg.bin will be loaded to 0x10000000.
 * User must update this address in available space of E820
 * if this default value address doesn't work.
 * And it points to the structure xmon_memory_layout_t
 */
#define STARTER_DEFAULT_LOAD_ADDR 0x10000000  /* @256MB  */



/* The final package is packed with xmon.bin,
 * startap.bin, xmon_loader.bin, and
 * the .text section of starter.bin
 */
#ifdef DEBUG
#define LOADER_BIN_SIZE    0xE1000 /* 900k */
#else
#define LOADER_BIN_SIZE    0x7D000 /* 500k */
#endif

/* header magics */
#define XMON_LOADERBIN_FILE_MAPPING_HEADER_MAGIC0 0x1B3D5F78
#define XMON_LOADERBIN_FILE_MAPPING_HEADER_MAGIC1 0x2A4C6E89



/* The size of our stack (8KB) for starter/loader */
#define STARTER_STACK_SIZE             0x2000

#ifndef ASM_FILE


#include "common_types.h"
#include "mon_startup.h"
#include "image_loader.h"
#include "x32_init64.h"


/* file layout in this order (no starter.bin)*/
enum {
	XMON_LOADER_BIN_INDEX   = 0,
	STARTAP_BIN_INDEX = 1,
	XMON_BIN_INDEX  = 2,
	/* add others if any */

	/* keep this as the last one */
	MAX_BIN_COUNT
} file_pack_index_t;

/* File offset mapping header flags
 *  actually, just convert order index (file_pack_index_t) to bit-map.
 */
#define INDEX_TO_BITMAP_FLAG(_index_)   (1 << (_index_))

typedef struct {
	/* address offset: offset to the _start of xmon_pkg.bin
	 *  packer will update these fields, and starter_main()
	 *  then uses them to get the binaries' location info.
	 */
	unsigned int offset;
	unsigned int size;
} file_bin_info_t;

/* layout header for files (xmon.bin,xmon_loader.bin,startap.bin) mapped in RAM
 *  by search this header(in starter.bin file, see staretr.S file), to get the address of these
 *  file mapping memory location. 4 byte aligned.
 *  the xmonPacker must search this header, and update the offset
 *  and size for each component/file.
 */
typedef struct {
	/* must be XMON_BINARY_FILE_MAPPING_HEADER_MAGIC0/1*/
	unsigned int magic0;
	unsigned int magic1;

	/* flags (bit-map) to indicate if file exists,
	 *  see macro: INDEX_TO_BITMAP_FLAG(_index_)
	 */
	unsigned int flags;


	/* valid only if the corresponding flag bit-map is set */
	file_bin_info_t files[MAX_BIN_COUNT];
} xmon_loaderbin_file_mapping_header_t;




/* The size of our heap (32MB) for starter/loader(xmon_loader) */
#define LOADER_HEAP_SIZE       0x2000000

/* startap size 128KB (must be 4KB aligned), otherwise, need to
 *  use paged_buffer_t to force page alignment
 */
#define STARTAP_IMG_SIZE                0x20000

/* xmon_loader image size running in RAM, 48KB */
#define XMON_LOADER_IMG_SIZE            0xC000

/* secondary guest runtime footprint size */
#define SG_RUNTIME_SIZE                     0x800000

/* startap AP startup stack: 1024 bytes
 *  refer to the function start_application() in startap.c file.
 */
#define STARTUP_AP_STACK_SIZE           0x400

/* AP startup Code size
 *  refer to APStartUpCode[] in file ap_procs_init.c
 */
#define STARTUP_AP_WAKEUP_CODE          120

/*
 * According to the Linux boot protocol (linux: Documentation\x86\boot.txt)
 * this address is used by legacy kernel real mode part, assume it is not
 * used now. But recommend the startap code should backup the content before
 * using it for AP wakeup bootstrap code.
 *
 * Note1:If this address doesn't work, e.g. in SMP mode, system cannot start
 *       but UP mode is OK, then change to another address 0x70000. This
 *       address is used as temp disk buffer during Grub startup stage, so
 *       assume it won't used any more once Grub is up.
 * Note2:Besides, there are some more choices:
 *       1) 0x8a000 -- used by tboot for S3 wakeup vector if xmon doesn't co-work
 *                     with tboot;
 *       2) 0x8c000 -- used by Xen trampoline, we can use it if no co-working.
 */
#define AP_WAKEUP_BOOTSTRAP_CODE_ADDR 0x90000



/* default xmon total size (depens on CPU count, total RAM size, and xmon view count)
 *  improvement: 1) caculate it at runtime, or; 2) passed on from command line
 */
#ifdef DEBUG
#define XMON_DEFAULT_TOTAL_SIZE  0xD00000 /* include xmon img/stack/heap */
#else
#define XMON_DEFAULT_TOTAL_SIZE  0xA00000
#endif



/* dummy page buffer definition:
 *  used as a place holder to force page alignment, e.g. paged_buffer_t[x]
 */
typedef struct _XMON_PAGED_BUFFER {
	uint8_t buf[4096];
} paged_buffer_t;

/* Num/Len of Page array requred for size n bytes */
#define NUM_OF_PAGE(n)  (((n) + 4095) >> 12)



/* add more if there are new modules loaded by grub */
typedef enum  {
	MFIRST_MODULE = 0,

	/* primary guest */
	MVMLINUZ = MFIRST_MODULE,       /* linux kernel */
	MINITRD,                        /* initrd */

	GRUB_MODULE_COUNT
} grub_module_index_t;


typedef struct {
	uint64_t img_base;
	uint64_t total_size;       /* must be >= hdr_info.load_size */

	image_info_t hdr_info;
	uint64_t entry_point;

	init32_struct_t init32;         /* passed on to start ap for AP wakeup */
	init64_struct_t init64;         /* required for switching to x64 bit */
} startap_info_t;


typedef struct {
	uint64_t img_base;

	/* may be not fixed, it is variable (not include startap size)
	 *  see XMON_DEFAULT_TOTAL_SIZE
	 */
	uint64_t total_size;


	image_info_t hdr_info;
	uint64_t entry_point;
} xmon_info_t;


/* struct for secondary guest support */
typedef struct {
	uint64_t img_base;
	uint64_t total_size;
	uint64_t entry_point;
	uint64_t p_mbi; /* multiboot info address */
} sguest_info_t;


/* file modules (raw binary) mapped in memory/RAM */
typedef struct {
	uint64_t addr;
	uint64_t size;
} module_file_info_t;


typedef struct {
	uint64_t rax;   /* MB magic */
	uint64_t rbx;   /* MB info passed on from loader */
	uint64_t rcx;
	uint64_t rdx;

	uint64_t rdi;
	uint64_t rsi;

	uint64_t rbp;
	uint64_t rsp;

	uint64_t rip;
	uint64_t rflags;
} initial_state_t;


typedef struct {
	/* save initial mb boot cpu state when bootstub passed on to us */
	initial_state_t initial_state;


	/* starter fills these below */
	uint64_t loader_mem_addr;
	uint64_t runtime_mem_addr;
	module_file_info_t xmon_loader_file;
	module_file_info_t startap_file;
	module_file_info_t xmon_file;
	module_file_info_t sguest_file;


	/* xmon_loader fills these below to prepare loading xmon */
	mon_startup_struct_t mon_env;  /* passed on to startap/xmon */
	startap_info_t startap;
	xmon_info_t xmon;
	sguest_info_t sguest;

} xmon_desc_t;

/*
 * use page place holder to force memory page-aligned
 */
typedef struct {
	/* xmon_pkg.bin image loaded by bootstub, including
	 *  raw xmon.bin, startap.bin, xmon_loader.bin,
	 *  !! must be located at first one to make sure
	 *  !! xmon_pkg.bin image can be correctly loaded by bootstub.
	 */
	union {
		uint8_t img_base[LOADER_BIN_SIZE];
		paged_buffer_t holder[NUM_OF_PAGE(LOADER_BIN_SIZE)];
	} u_ldr_bin;

	/* page aligned, xmon_loader image in RAM */
	union {
		uint8_t img_base[XMON_LOADER_IMG_SIZE];
		paged_buffer_t holder[NUM_OF_PAGE(XMON_LOADER_IMG_SIZE)];
	} u_xmon_loader;


	/* page aligned, heap memory in RAM */
	union {
		uint8_t base[LOADER_HEAP_SIZE];
		paged_buffer_t holder[NUM_OF_PAGE(LOADER_HEAP_SIZE)];
	} u_ldr_heap;


	union {
		xmon_desc_t ed;
		paged_buffer_t holder[NUM_OF_PAGE(sizeof(xmon_desc_t))];
	} u_desc;


	/* add more if any */
} xmon_loader_memory_layout_t;



/*
 *   xmon runtime memory layout
 *  +-----------+\
 *  |           | \
 *  |           |  \
 *  |           |   \
 *  |   heap    |    \
 *  |           |     \
 *  |           |      \
 *  |           |       |
 *  +-----------+       |->- mon_memory_layout[mon_image].total_size (XMON_DEFAULT_TOTAL_SIZE)
 *  |           |       |
 *  |  stack    |      /
 *  |           |     /
 *  +-----------+    /
 *  |           |   /
 *  |  xmon img |  /  <--- mon_memory_layout[mon_image].image_size
 *  |           | /
 *  +-----------+/    <--- mon_memory_layout[mon_image].base_address
 *  |           |
 *  | otherguest|
 *  |   imgs(if |
 *  |   any)    |
 *  |           |
 *  +-----------+
 *  |           |
 *  |startap img|     <--- STARTAP_IMG_SIZE
 *  +-----------+
 *
 */

/* memory layout at runtime
 * startap, xmon and secondary guest runtime memory layout
 */
typedef struct {
	/* startap runtime image in RAM */
	union {
		uint8_t base[STARTAP_IMG_SIZE];
		paged_buffer_t holder[NUM_OF_PAGE(STARTAP_IMG_SIZE)];
	} u_startap_img;

	union {
		uint8_t base[SG_RUNTIME_SIZE];
		paged_buffer_t holder[NUM_OF_PAGE(SG_RUNTIME_SIZE)];
	} u_sguest_img;

	/* add more if any */


	/* currently using default size.
	 *  xmon(code, stack, heap) at runtime.
	 */
	union {
		uint8_t img_base[XMON_DEFAULT_TOTAL_SIZE];
		paged_buffer_t holder[NUM_OF_PAGE(XMON_DEFAULT_TOTAL_SIZE)];
	} u_xmon;
} xmon_runtime_memory_layout_t;

#define FIELD_OFFSET_TO_HEAD(__struct, __member) OFFSET_OF(__struct, __member)

/*
 *  this struct is just used as memory layout definition, and for
 *  easy memory base address caculation.
 *  this structure must be pointered by STARTER_DEFAULT_LOAD_ADDR
 */
typedef struct _XMON_MEMORY_LAYOUT_ {
	/* this memory used at loader time only, they will be
	 *  re-claim by guest OS after xmon starts.
	 */
	xmon_loader_memory_layout_t ldr_mem_base;      /* must be located at first one */


	/* this memory used at RUNTIME , it is bootstub or xmon_loader's
	 *  responsibility to hide it from E820 table, and eventually
	 *  hiden from guest
	 *  by xmon code with ept settings.
	 */
	xmon_runtime_memory_layout_t rt_mem_base;
} xmon_memory_layout_t;


/* must hide this memory range below by xmon_loader or bootstub */
#define XMON_RUNTIME_MEMORY_BASE  (STARTER_DEFAULT_LOAD_ADDR + \
				   FIELD_OFFSET_TO_HEAD(xmon_memory_layout_t, \
					   rt_mem_base))
#define XMON_RUNTIME_MEMORY_SIZE  (sizeof(xmon_runtime_memory_layout_t))


/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

#endif

/* End of file */
#endif
