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

#include "elf_env.h"
#include "elf32_ld.h"
#include "elf64.h"
#include "elf64_ld.h"
#include "elf_ld.h"
#include "elf_info.h"
#include "image_access_mem.h"
#include "image_loader.h"
#include "elf_ld_env.h"
void_t print_string(uint8_t *string);
/*
 *  FUNCTION  : elf_get_load_info
 *  PURPOSE   : Return ELF image size and machine type
 *  ARGUMENTS : image - describes image and access class
 *            : LOAD_ELF_PARAMS_T *params
 *    RETURNS : MON_OK if success
 */
static mon_status_t
elf_get_load_info(gen_image_access_t *image, elf_load_info_t *p_info)
{
	uint8_t *p_buffer;
	mon_status_t status;

	if (sizeof(elf64_ehdr_t) !=
	    mem_image_map_to_mem(image, (void **)&p_buffer, 0,
		    sizeof(elf64_ehdr_t))) {
		ELF_PRINT_STRING("failed to read file's header\n");
		return MON_ERROR;
	}

	if (elf32_header_is_valid(p_buffer)) {
		p_info->machine_type = EM_386;
		status = elf32_get_load_info(image, p_info);
	} else if (elf64_header_is_valid(p_buffer)) {
		p_info->machine_type = EM_X86_64;
		status = elf64_get_load_info(image, p_info);
	} else {
		status = MON_ERROR; /* not ELF */
	}

	return status;
}

/*
 *  FUNCTION  : elf_load_executable
 *  PURPOSE   : Load and relocate ELF-executable to memory
 *  ARGUMENTS : gen_image_access_t *image - describes image to load
 *            : elf_load_info_t *p_info - contains load-related data
 *            : uint8_t    *p_dest    - where to load
 *  RETURNS   :
 *  NOTES     : elf_get_load_info() must be called prior this function
 *            : p_dest assumed is pointing on preallocated memory buffer,
 *            : sufficient to contain loaded executable binary
 */
static mon_status_t
elf_load_executable(gen_image_access_t *image,
		    elf_load_info_t *p_info, uint8_t *p_dest)
{
	mon_status_t status;

	/* calculate relocation offset */
	p_info->relocation_offset =
		(int64_t)((uint64_t)(size_t)p_dest - p_info->start_addr);

	/* and update addresses accordingly */
	p_info->start_addr += p_info->relocation_offset;
	p_info->end_addr += p_info->relocation_offset;
	p_info->entry_addr += p_info->relocation_offset;
	p_info->sections_addr += p_info->relocation_offset;

	switch (p_info->machine_type) {
	case EM_386:
		status = elf32_load_executable(image, p_info);
		break;
	case EM_X86_64:
		status = elf64_load_executable(image, p_info);
		break;
	default:
		status = MON_ERROR;
		break;
	}

	return status;
}

static boolean_t
load_elf_image(char *p_image,
	       char *p_target,
	       size_t image_size, uint64_t *p_entry_point_address)
{
	mon_status_t status;
	gen_image_access_t *image = NULL;
	elf_load_info_t load_info;
	mem_image_access_t tmp;

	image = mem_image_create_ex(p_image, image_size, (void *)&tmp);
	if (NULL != image) {
		load_info.copy_section_headers = FALSE;
		load_info.copy_symbol_tables = FALSE;

		status = elf_get_load_info(image, &load_info);
		if (MON_OK != status) {
			print_string("elf_get_load_info() failed\n");
			return FALSE;
		}

		status = elf_load_executable(image, &load_info, p_target);
		if (MON_OK != status) {
			print_string("elf_load_executable() failed\n");
			return FALSE;
		}
	} else {
		return FALSE;
	}

	*p_entry_point_address = load_info.entry_addr;

	return TRUE;
}

/*------------------------- Exported Interface --------------------------*/

/*----------------------------------------------------------------------
 *
 * Get info required for image loading
 *
 * Input:
 * void* file_mapped_into_memory - file directly read or mapped in RAM
 *
 * Output:
 * image_info_t - fills the structure
 *
 * Return value - image_info_status_t
 *---------------------------------------------------------------------- */
image_info_status_t get_image_info(const void *file_mapped_into_memory,
				   uint32_t allocated_size,
				   image_info_t *p_image_info)
{
	mon_status_t status = 0;
	gen_image_access_t *image = NULL;
	elf_load_info_t load_info;
	mem_image_access_t tmp;

	image =
		mem_image_create_ex((char *)file_mapped_into_memory, allocated_size,
			(void *)&tmp);
	if (NULL == image) {
		return IMAGE_INFO_WRONG_PARAMS;
	}

	load_info.copy_section_headers = FALSE;
	load_info.copy_symbol_tables = FALSE;

	status = elf_get_load_info(image, &load_info);
	if (MON_OK != status) {
		return IMAGE_INFO_WRONG_FORMAT;
	}

	p_image_info->load_size = load_info.end_addr - load_info.start_addr;

	if (EM_386 == load_info.machine_type) {
		p_image_info->machine_type = IMAGE_MACHINE_X86;
	}

	if (EM_X86_64 == load_info.machine_type) {
		p_image_info->machine_type = IMAGE_MACHINE_EM64T;
	}

	return IMAGE_INFO_OK;
}

/*----------------------------------------------------------------------
 *
 * load image into memory
 *
 * Input:
 * void* file_mapped_into_memory - file directly read or mapped in RAM
 * void* image_base_address - load image to this address. Must be alined
 * on 4K.
 * uint32_t allocated_size - buffer size for image
 * uint64_t* p_entry_point_address - address of the uint64_t that will be filled
 * with the address of image entry point if
 * all is ok
 *
 * Output:
 * Return value - FALSE on any error
 *---------------------------------------------------------------------- */
boolean_t
load_image(const void *file_mapped_into_memory,
	   void *image_base_address,
	   uint32_t allocated_size, uint64_t *p_entry_point_address)
{
	return load_elf_image((char *)file_mapped_into_memory,
		(char *)image_base_address, (size_t)allocated_size,
		p_entry_point_address);
}
