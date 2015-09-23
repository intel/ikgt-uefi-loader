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

#include <efi.h>
#include <efilib.h>

#include <preload.h>

#define IKGT_BOOT_HEADER_MAGIC        0x6d6d76656967616d
#define HIGH_ADDR                     0x3fffffff
#define IMAGE_NAME                    L"ikgt_pkg.bin"

#define DEBUG_MSG

#ifdef DEBUG_MSG

#define debug(fmt, ...) do { \
	Print(fmt, ##__VA_ARGS__); \
} while(0)

#else
#define debug(fmt, ...) (void)0
#endif

/*
*  ikgt image header
*  contains addresses at which load-time and run-time memory
*  regions need to be allocaed, their sizes, and the entry point
*  offset
*/

typedef struct {
	/* a 64bit magic value */
	UINT64  magic;

	/* size of this structure */
	UINT32  size;
	UINT32  reserved;

	/* 32bit entry offset */
	UINT32  entry32_offset;
	/* 64bit entry offset */
	UINT32  entry64_offset;

	/* reserved */
	UINT32  reserved1;
	UINT32  reserved2;

	/* rt_mem_base is a prefered runtime memory address for bootloader
	 * to allocate. If failed, the bootloader can allocate any address
	 * below 1G. */
	UINT32  rt_mem_base;
	/* rt_mem_size is sizeof(xmon_runtime_memory_layout_t) */
	UINT32  rt_mem_size;

	/* ldr_mem_base is a prefered loadtime memory address for bootloader
	 * to allocate. If failed, the bootloader can allocate any address
	 * below 1G. */
	UINT32  ldr_mem_base;
	/* ldr_mem_size is sizeof(xmon_loadtime_memory_layout_t) */
	UINT32  ldr_mem_size;

	/* size of image package after 0-padding to 4k aligned */
	UINT32  image_size;
	/* anything else?*/
} ikgt_loader_boot_header_t;

/*
 *  Platform info structure to store the EFI memory map and all
 *  other info that needs to be passed to ikgt loader
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

static uint64_t __readmsr(uint64_t msr_id)
{
	uint64_t ret;
	__asm__ __volatile__ (
	"mov %0, %%rcx\n"
	"xor %%rax, %%rax\n"
	"rdmsr\n"
	"mov %%rax, %1"
	:
	: "g" (msr_id), "m" (ret)
	: "%rcx", "%rax", "%rdx", "memory"
	);

	return ret;
}

static void __cpuid(uint64_t cpu_info[4], uint64_t info_type)
{
	__asm__ __volatile__ (
		"push %%rbx      \n\t" /* save %rbx */
		"cpuid           \n\t"
		"mov %%rbx, %1   \n\t" /* save what cpuid just put in %rbx */
		"pop %%rbx       \n\t" /* restore the old %rbx */
		:
		"=a" (cpu_info[0]),
		"=r" (cpu_info[1]),
		"=c" (cpu_info[2]),
		"=d" (cpu_info[3])
		: "a" (info_type)
		: "cc"
		);
}
#define IA32_MSR_FEATURE_CONTROL        ((uint32_t)0x03A)

static int check_vmx_support(void)
{
	uint64_t info[4];
	uint64_t u;

	/* CPUID: input in rax = 1. */
	__cpuid(info, 1);

	/* CPUID: output in rcx, VT available? */
	if ((info[2] & 0x00000020) == 0) {
		debug(L"VT not available\n");
		return -1;
	}

	/* Fail if feature is locked and vmx is off. */
	u = __readmsr(IA32_MSR_FEATURE_CONTROL);

	if (((u & 0x01) != 0) && ((u & 0x04) == 0)) {
		debug(L"VMX is off!\n");
		return -1;
	}
	debug(L"VT is available and VMX is ON!\n");
	return 0;
}


static EFI_STATUS load_image(EFI_FILE_HANDLE dir,
			const CHAR16 *name,
			EFI_PHYSICAL_ADDRESS *image_addr,
			UINT32 *image_size)
{
	EFI_STATUS err;
	EFI_FILE_HANDLE handle = NULL;
	EFI_FILE_INFO *info;
	CHAR8 *buf;
	UINTN buflen;
	EFI_PHYSICAL_ADDRESS buf_phy_addr = HIGH_ADDR;

	err = open_file(dir, &handle, (VOID *)name, EFI_FILE_MODE_READ);
	if (EFI_ERROR(err) || handle == NULL) {
		debug(L"open file error: %r\n", err);
		goto out;
	}

	/* allocate memory used for load ikgt_pkg.bin file into memory */
	info = LibFileInfo(handle);
	buflen = info->FileSize+1;
	err = allocate_pages(
			AllocateMaxAddress,
			EfiLoaderData,
			EFI_SIZE_TO_PAGES(buflen),
			(EFI_PHYSICAL_ADDRESS *)&buf_phy_addr);
	if (EFI_ERROR(err) != EFI_SUCCESS) {
		debug(L"alloc mem has failed\n");
		goto out;
	}
	buf = (UINT8 *)(UINTN)buf_phy_addr;

	err = read_file(handle, buflen, buf);
	if (EFI_ERROR(err) == EFI_SUCCESS) {
		buf[buflen] = '\0';
		*image_addr = buf_phy_addr;
		*image_size = buflen;
		debug(L"read file into buffer succeed! file size = %d\n", buflen);
	} else {
		debug(L"read file into buffer failed: error=%r\n", err);
		free_pages(buf_phy_addr, EFI_SIZE_TO_PAGES(buflen));
	}

out:
	close_file(handle);
	return err;
}

static ikgt_loader_boot_header_t *find_header(UINTN start_addr, UINT32 size)
{
	ikgt_loader_boot_header_t *ikgt_hdr = NULL;
	UINT64  *magic;

	/* one time scan 8bytes */
	for (magic = (UINT64 *)start_addr; (UINTN)magic < (UINTN)start_addr+size; magic++) {
		if (*magic == IKGT_BOOT_HEADER_MAGIC) {
			debug(L"find the the specified headers\n");
			ikgt_hdr = (ikgt_loader_boot_header_t *) magic;
			break;
		}
	}

	if (*magic != IKGT_BOOT_HEADER_MAGIC)
		debug(L"cannot find the the sepecified heades\n");

	if (ikgt_hdr != NULL) {
		debug(L"ikgt_header->magic = 0x%llx\n", ikgt_hdr->magic);
		debug(L"ikgt_header->size = %d\n", ikgt_hdr->size);
		debug(L"ikgt_header->entry32_offset = 0x%x\n", ikgt_hdr->entry32_offset);
		debug(L"ikgt_header->entry64_offset = 0x%x\n", ikgt_hdr->entry64_offset);
		debug(L"ikgt_header->rt_mem_size = 0x%x\n", ikgt_hdr->rt_mem_size);
		debug(L"ikgt_header->ldr_mem_size = 0x%x\n", ikgt_hdr->ldr_mem_size);
	}

	return ikgt_hdr;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_LOADED_IMAGE     *efi_loaded_image = NULL;
	EFI_FILE_HANDLE      root_dir;
	EFI_STATUS           err = EFI_SUCCESS;

	VOID (*call_loader)  (ikgt_platform_info_t *) = NULL;

	EFI_PHYSICAL_ADDRESS image_addr = HIGH_ADDR;
	EFI_PHYSICAL_ADDRESS platform_addr = HIGH_ADDR;
	UINTN                map_key;
	UINTN                desc_size;
	UINT32               desc_ver;
	UINTN                nr_entries;
	UINT32               image_size = 0;
	BOOLEAN              alloc_flag = FALSE;

	ikgt_platform_info_t      *platform_info;
	ikgt_loader_boot_header_t *ikgt_header;

	InitializeLib(ImageHandle, SystemTable);

	err = open_protocol(ImageHandle, &efi_loaded_image);
	if (EFI_ERROR(err) || efi_loaded_image == NULL) {
		debug(L"Error getting LoadedImageProtocol handle %d", err);
		return err;
	}

	root_dir = LibOpenRoot(efi_loaded_image->DeviceHandle);
	if (!root_dir) {
		debug(L"Unable to open root directory %d", err);
		goto out;
	}

	/* load the ikgt_pkg.bin into memory */
	err = load_image(root_dir, IMAGE_NAME, &image_addr, &image_size);
	if (EFI_ERROR(err)) {
		debug(L"read file failed\n");
		goto out;
	}
	debug(L"Image Load Addr = %x\n", (UINTN)image_addr);
	debug(L"Image Size = %d\n", image_size);

	/* find the ikgt's private header */
	ikgt_header = find_header((UINTN)image_addr, image_size);
	if (ikgt_header == NULL) {
		debug(L"get ikgt file header failed\n");
		goto out;
	}

	/* ldr_mem_base is a prefered loadtime memory address for bootloader
	* to allocate. If failed, the bootloader can allocate any address
	* below 1G. */
	err = allocate_pages(
			AllocateAddress,
			EfiLoaderData,
			EFI_SIZE_TO_PAGES(ikgt_header->ldr_mem_size),
			(EFI_PHYSICAL_ADDRESS *)&ikgt_header->ldr_mem_base);
	if (EFI_ERROR(err) != EFI_SUCCESS) {
		debug(L"allocating loadtime mem at the fixed address failed, ");
		debug(L"try to allocate it at any address below 1G\n");
		ikgt_header->ldr_mem_base = HIGH_ADDR;
		err = allocate_pages(
			AllocateMaxAddress,
			EfiLoaderData,
			EFI_SIZE_TO_PAGES(ikgt_header->ldr_mem_size),
			(EFI_PHYSICAL_ADDRESS *)&ikgt_header->ldr_mem_base);
	}
	if (EFI_ERROR(err) != EFI_SUCCESS) {
		debug(L"allocate loadtime memory has failed\n");
		goto out;
	}

	/* rt_mem_base is a prefered runtime memory address for bootloader
	* to allocate. If failed, the bootloader can allocate any address
	* below 1G. */
	err = allocate_pages(
			AllocateAddress,
			EfiReservedMemoryType,
			EFI_SIZE_TO_PAGES(ikgt_header->rt_mem_size),
			(EFI_PHYSICAL_ADDRESS *)&ikgt_header->rt_mem_base);
	if (EFI_ERROR(err) != EFI_SUCCESS) {
		debug(L"allocating runtime mem at the fixed address failed, ");
		debug(L"try to allocate it at any address below 1G\n");
		ikgt_header->rt_mem_base = HIGH_ADDR;
		err = allocate_pages(
			AllocateMaxAddress,
			EfiReservedMemoryType,
			EFI_SIZE_TO_PAGES(ikgt_header->rt_mem_size),
			(EFI_PHYSICAL_ADDRESS *)&ikgt_header->rt_mem_base);
	}
	if (EFI_ERROR(err) != EFI_SUCCESS) {
		debug(L"allocate runtime memory has failed\n");
		goto out;
	}
	alloc_flag = TRUE;
	debug(L"allocation of ldr/rt memory for ikgt succeed!\n");
	debug(L"load-time memory addr = 0x%x\n", ikgt_header->rt_mem_base);
	debug(L"run-time memory addr = 0x%x\n", ikgt_header->ldr_mem_base);

	/* copy the ikgt_pkg.bin into the load time memory */
	CopyMem((VOID *)(UINTN)ikgt_header->ldr_mem_base,
			(VOID *)(UINTN)image_addr,
			image_size);

	/* allocate memory for platform_info structure */
	err = allocate_pages(
			AllocateMaxAddress,
			EfiLoaderData,
			EFI_SIZE_TO_PAGES(sizeof(ikgt_platform_info_t)),
			(EFI_PHYSICAL_ADDRESS *)&platform_addr);

	if (EFI_ERROR(err) != EFI_SUCCESS) {
		debug(L"alloc mem for platform_info has failed\n");
		goto out;
	}
	platform_info = (ikgt_platform_info_t *)platform_addr;

	/* initialize the platform_info */
	platform_info->memmap_addr = (UINT32)(UINTN)LibMemoryMap(&nr_entries,
								&map_key,
								&desc_size,
								&desc_ver);
	platform_info->memmap_size = desc_size * nr_entries;
	platform_info->load_addr = ikgt_header->ldr_mem_base;
	platform_info->run_addr = ikgt_header->rt_mem_base;

	debug(L"platform_info->memmap_addr = 0x%x\n", platform_info->memmap_addr);
	debug(L"platform_info->memmap_size = 0x%x\n", platform_info->memmap_size);

	/* set the call address */
	call_loader = (VOID(*)(ikgt_platform_info_t *))(UINTN)(ikgt_header->ldr_mem_base
				+ ikgt_header->entry64_offset);

	debug(L"call ikgt loader entry_addr = 0x%x\n", call_loader);
	debug(L"loading ikgt...\n");

	if (0 != check_vmx_support()) {
		debug(L"No VTx support. will not load ikgt!\n");
		goto out;
	}

	/* call the entry point of ikgt loader */
	if (call_loader != NULL)
		call_loader(platform_info);

	check_vmx_support();

	debug(L"loading ikgt done!\n");
out:
	if (image_addr != HIGH_ADDR)
		free_pages(image_addr, EFI_SIZE_TO_PAGES(image_size));
	if (platform_addr != HIGH_ADDR)
		free_pages(platform_addr, EFI_SIZE_TO_PAGES(sizeof(ikgt_platform_info_t)));

	if (alloc_flag == TRUE)
		free_pages(ikgt_header->ldr_mem_base, EFI_SIZE_TO_PAGES(ikgt_header->ldr_mem_size));
	/* must not to free the runtime memory, it's will be used by ikgt at runtime. */

	close_file(root_dir);
	close_protocol(ImageHandle);

	/* this allow us to configure EFI to chain multiple bootloaders.
	* EFI will load us firstly, and because we return EFI_NOT_FOUND,
	* it will try the next bootloader which is the OS bootloader.
	*/
	if (EFI_ERROR(err) == EFI_SUCCESS)
		return EFI_NOT_FOUND;

	return err;
}

