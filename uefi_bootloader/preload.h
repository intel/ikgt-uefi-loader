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

#ifndef __PRELOAD_H__
#define __PRELOAD_H__

/**
 * allocate_pages - Allocate memory pages from the system
 * @atype: type of allocation to perform
 * @mtype: type of memory to allocate
 * @num_pages: number of contiguous 4KB pages to allocate
 * @memory: used to return the address of allocated pages
 *
 * Allocate @num_pages physically contiguous pages from the system
 * memory and return a pointer to the base of the allocation in
 * @memory if the allocation succeeds. On success, the firmware memory
 * map is updated accordingly.
 *
 * If @atype is AllocateAddress then, on input, @memory specifies the
 * address at which to attempt to allocate the memory pages.
 */
static inline EFI_STATUS
allocate_pages(EFI_ALLOCATE_TYPE atype,
		EFI_MEMORY_TYPE mtype,
		UINTN num_pages,
		EFI_PHYSICAL_ADDRESS *memory)
{
	return uefi_call_wrapper(BS->AllocatePages, 4,
			atype,
			mtype,
			num_pages,
			memory);
}

/**
 * free_pages - Return memory allocated by allocate_pages() to the firmware
 * @memory: physical base address of the page range to be freed
 * @num_pages: number of contiguous 4KB pages to free
 *
 * On success, the firmware memory map is updated accordingly.
 */
static inline EFI_STATUS
free_pages(EFI_PHYSICAL_ADDRESS memory, UINTN num_pages)
{
	return uefi_call_wrapper(BS->FreePages, 2, memory, num_pages);
}

/**
 * allocate_pool - Allocate pool memory
 * @type: the type of pool to allocate
 * @size: number of bytes to allocate from pool of @type
 * @buffer: used to return the address of allocated memory
 *
 * Allocate memory from pool of @type. If the pool needs more memory
 * pages are allocated from EfiConventionalMemory in order to grow the
 * pool.
 *
 * All allocations are eight-byte aligned.
 */
static inline EFI_STATUS
allocate_pool(EFI_MEMORY_TYPE type, UINTN size, void **buffer)
{
	return uefi_call_wrapper(BS->AllocatePool, 3, type, size, buffer);
}

/**
 * free_pool - Return pool memory to the system
 * @buffer: the buffer to free
 *
 * Return @buffer to the system. The returned memory is marked as
 * EfiConventionalMemory.
 */
static inline EFI_STATUS free_pool(void *buffer)
{
	return uefi_call_wrapper(BS->FreePool, 1, buffer);
}

static inline EFI_STATUS open_protocol(EFI_HANDLE ImageHandle,
			EFI_LOADED_IMAGE **efi_loaded_image)
{
	 return uefi_call_wrapper(BS->OpenProtocol,
			6,
			ImageHandle,
			&LoadedImageProtocol,
			(VOID **)efi_loaded_image,
			ImageHandle,
			NULL,
			EFI_OPEN_PROTOCOL_GET_PROTOCOL);
}

static inline EFI_STATUS close_protocol(EFI_HANDLE ImageHandle)
{
	return uefi_call_wrapper(BS->CloseProtocol,
			4,
			ImageHandle,
			&LoadedImageProtocol,
			ImageHandle, NULL);
}


static inline EFI_STATUS open_file(EFI_FILE_HANDLE Dir,
			EFI_FILE_HANDLE *FileHandle,
			VOID *FileName,
			UINTN Mode)
{
	return uefi_call_wrapper(Dir->Open,
			5,
			Dir,
			FileHandle,
			FileName,
			Mode, 0ULL);
}

static inline EFI_STATUS read_file(EFI_FILE_HANDLE FileHandle,
			UINTN buflen,
			VOID *buf)
{
	return uefi_call_wrapper(FileHandle->Read, 3, FileHandle, &buflen, buf);
}

static inline EFI_STATUS close_file(EFI_FILE_HANDLE FileHandle)
{
	return uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
}

#endif
