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
#include "elf64.h"
#include "elf64_ld.h"
#include "elf_info.h"
#include "elf_ld_env.h"

typedef struct {
	uint32_t lo;
	uint32_t hi;
} u64_t;

#define STINGS_ARE_EQUAL(__s1, __s2) (0 == strcmp(__s1, __s2))

void *mon_memset(void *dest, int val, size_t count);

/* prototypes of the real elf parsing functions */
static mon_status_t elf64_copy_sections(gen_image_access_t *image,
					elf_load_info_t *p_info);
static mon_status_t elf64_do_relocation(gen_image_access_t *image,
					elf_load_info_t *p_info,
					elf64_phdr_t *phdr_dyn);
static mon_status_t elf64_copy_section_header_table(gen_image_access_t *image,
						    elf_load_info_t *p_info);

/*
 *  FUNCTION  : elf64_get_load_info
 *  PURPOSE   : Get load-related information
 *  ARGUMENTS : gen_image_access_t *image - describes image to load
 *            : elf_load_info_t *p_info -
 *                      contains both input and output load-related data
 *  RETURNS   :
 */
mon_status_t
elf64_get_load_info(gen_image_access_t *image, elf_load_info_t *p_info)
{
	elf64_ehdr_t *ehdr;             /* ELF header */
	uint8_t *phdrtab;               /* Program Segment header Table */
	uint8_t *shdrtab;               /* sections header Table */
	elf64_addr_t low_addr = (elf64_addr_t) ~0;
	elf64_addr_t max_addr = 0;
	elf64_addr_t addr;
	uint32_t phsize;
	uint64_t memsz;
	int16_t i;
	mon_status_t status = MON_OK;
	uint32_t shsize;

	/* map ELF header to ehdr */
	if (sizeof(elf64_ehdr_t) !=
	    mem_image_map_to_mem(image, (void **)&ehdr, 0,
		    (size_t)sizeof(elf64_ehdr_t))) {
		status = MON_ERROR;
		goto quit;
	}

	if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
		ELF_PRINT_STRING("ELF file type not executable, type = 0x");
		ELF_PRINTLN_VALUE(ehdr->e_type);
		status = MON_ERROR;
		goto quit;
	}

	/* map Program Segment header Table to phdrtab */
	phsize = ehdr->e_phnum * ehdr->e_phentsize;
	if (mem_image_map_to_mem(image, (void **)&phdrtab, (size_t)ehdr->e_phoff,
		    (size_t)phsize) != phsize) {
		status = MON_ERROR;
		goto quit;
	}

	/* Calculate amount of memory required. First calculate size of all
	 * loadable segments */

	ELF_PRINT_STRING("addr_start..addr_end :offset :filesz "
		":memsz :bss_sz :p_type\n");
	for (i = 0; i < (int16_t)ehdr->e_phnum; ++i) {
		elf64_phdr_t *phdr = (elf64_phdr_t *)GET_PHDR(ehdr, phdrtab, i);

		addr = phdr->p_paddr;
		memsz = phdr->p_memsz;

		ELF_PRINT_VALUE(addr);
		ELF_PRINT_VALUE(addr + memsz);
		ELF_PRINT_VALUE(phdr->p_offset);
		ELF_PRINT_VALUE(phdr->p_filesz);
		ELF_PRINT_VALUE(memsz);
		ELF_PRINT_VALUE(memsz - phdr->p_filesz);
		ELF_PRINTLN_VALUE(phdr->p_type);

		if (PT_LOAD != phdr->p_type || 0 == phdr->p_memsz) {
			continue;
		}
		if (addr < low_addr) {
			low_addr = addr;
		}
		if (addr + memsz > max_addr) {
			max_addr = addr + memsz;
		}
	}

	if (0 != (low_addr & PAGE_4KB_MASK)) {
		ELF_PRINT_STRING ("failed because kernel low address "
			"not page aligned, low_addr = 0x");
		ELF_PRINTLN_VALUE(low_addr);
		status = MON_ERROR;
		goto quit;
	}

	/* now calculate amount of memory required for optional tables */
	if (p_info->copy_section_headers || p_info->copy_symbol_tables) {
		/* reserve place for section headers table */
		/* align sections table on 16-byte boundary */
		max_addr = (max_addr + 0xFull) & ~0xFull;
		/* store for further usage */
		p_info->sections_addr = max_addr;
		shsize = ehdr->e_shnum * ehdr->e_shentsize;
		/* add size of section headers table */
		max_addr += shsize;

		if (p_info->copy_symbol_tables) {
			/* map sections header Table to shdrtab */
			if (mem_image_map_to_mem
				    (image, (void **)&shdrtab, (size_t)ehdr->e_shoff,
				    (size_t)shsize) != shsize) {
				status = MON_ERROR;
				goto quit;
			}

			/* go through sections table, looking for non-empty sections */
			for (i = 0; i < ehdr->e_shnum; ++i) {
				elf64_shdr_t *shdr = (elf64_shdr_t *)GET_SHDR_LOADER(ehdr,
					shdrtab,
					i);
				if (0 == shdr->sh_size) {
					continue;
				}
				if (shdr->sh_addr != 0) {
					/* skip sections, which loaded as part of the program. */
					continue;
				}
				if (shdr->sh_addralign > 1) {
					max_addr =
						(max_addr +
						 (shdr->sh_addralign -
						  1)) & ~(shdr->sh_addralign - 1);
				}

				max_addr += shdr->sh_size;
			}
		}
	}

	p_info->start_addr = low_addr;
	p_info->end_addr = max_addr;
	p_info->entry_addr = ehdr->e_entry;

quit:
	return status;
}

/*
 *  FUNCTION  : elf64_load_executable
 *  PURPOSE   : Load and relocate ELF x86-64 executable to memory
 *  ARGUMENTS : gen_image_access_t *image - describes image to load
 *            : elf_load_info_t *p_info - contains load-related data
 *  RETURNS   :
 *  NOTES     : Load map (addresses grow from up to bottom)
 *            :        elf header
 *            :        loadable program segments
 *            :        section headers table (optional)
 *            :        loaded sections        (optional)
 */
mon_status_t
elf64_load_executable(gen_image_access_t *image, elf_load_info_t *p_info)
{
	mon_status_t status = MON_OK;
	elf64_ehdr_t *ehdr;
	uint8_t *phdrtab;
	elf64_word_t phsize;
	elf64_addr_t addr;
	elf64_xword_t memsz;
	elf64_xword_t filesz;
	int16_t i;
	elf64_phdr_t *phdr_dyn = NULL;

	ELF_CLEAR_SCREEN();

	/* map ELF header to ehdr */
	if (sizeof(elf64_ehdr_t) !=
	    mem_image_map_to_mem(image, (void **)&ehdr, 0, sizeof(elf64_ehdr_t))) {
		status = MON_ERROR;
		goto quit;
	}

	/* map Program Segment header Table to phdrtab */
	phsize = ehdr->e_phnum * ehdr->e_phentsize;
	if (mem_image_map_to_mem(image, (void **)&phdrtab, (size_t)ehdr->e_phoff,
		    (size_t)phsize) != phsize) {
		status = MON_ERROR;
		goto quit;
	}

	ELF_PRINT_STRING
		("p_type :p_flags :p_offset:p_vaddr :p_paddr "
		":p_filesz:p_memsz :p_align\n");

	/* now actually copy image to its target destination */
	for (i = 0; i < (int16_t)ehdr->e_phnum; ++i) {
		elf64_phdr_t *phdr = (elf64_phdr_t *)GET_PHDR(ehdr, phdrtab, i);

		ELF_PRINT_VALUE(phdr->p_type);
		ELF_PRINT_VALUE(phdr->p_flags);
		ELF_PRINT_VALUE(phdr->p_offset);
		ELF_PRINT_VALUE(phdr->p_vaddr);
		ELF_PRINT_VALUE(phdr->p_paddr);
		ELF_PRINT_VALUE(phdr->p_filesz);
		ELF_PRINT_VALUE(phdr->p_memsz);
		ELF_PRINTLN_VALUE(phdr->p_align);

		if (PT_DYNAMIC == phdr->p_type) {
			phdr_dyn = phdr;
			continue;
		}

		if (PT_LOAD != phdr->p_type || 0 == phdr->p_memsz) {
			continue;
		}

		filesz = phdr->p_filesz;
		addr = phdr->p_paddr;
		memsz = phdr->p_memsz;

		/* make sure we only load what we're supposed to! */
		if (filesz > memsz) {
			filesz = memsz;
		}

		if (mem_image_read
			    (image,
			    (void *)(size_t)(addr + p_info->relocation_offset),
			    (size_t)phdr->p_offset, (size_t)filesz)
		    != (size_t)filesz) {
			status = MON_ERROR;
			ELF_PRINT_STRING("failed to read segment from file\n");
			goto quit;
		}

		if (filesz < memsz) {
			/* zero BSS if exists */
			mon_memset((void *)(size_t)(addr + filesz +
						    p_info->relocation_offset), 0,
				(size_t)(memsz - filesz));
		}
	}

	/* Update copied segments addresses */
	ehdr = (elf64_ehdr_t *)(size_t)p_info->start_addr;
	phdrtab = (uint8_t *)(size_t)(p_info->start_addr + ehdr->e_phoff);

	for (i = 0; i < (int16_t)ehdr->e_phnum; ++i) {
		elf64_phdr_t *phdr = (elf64_phdr_t *)GET_PHDR(ehdr, phdrtab, i);

		if (0 != phdr->p_memsz) {
			phdr->p_paddr += p_info->relocation_offset;
			phdr->p_vaddr += p_info->relocation_offset;
		}
	}

	if (NULL != phdr_dyn) {
		status = elf64_do_relocation(image, p_info, phdr_dyn);
		if (MON_OK != status) {
			goto quit;
		}
	}

	/* optionally copy sections table and sections */
	if (p_info->copy_section_headers || p_info->copy_symbol_tables) {
		status = elf64_copy_section_header_table(image, p_info);
	}

	if (p_info->copy_symbol_tables) {
		status = elf64_copy_sections(image, p_info);
	}

quit:
	return status;
}

mon_status_t
elf64_do_relocation(gen_image_access_t *image,
		    elf_load_info_t *p_info, elf64_phdr_t *phdr_dyn)
{
	elf64_dyn_t *dyn_section;
	elf64_sword_t dyn_section_sz = (elf64_sword_t)phdr_dyn->p_filesz;
	elf64_rela_t *rela = NULL;
	elf64_sword_t rela_sz = 0;
	elf64_sword_t rela_entsz = 0;
	elf64_sym_t *symtab = NULL;
	elf64_sword_t symtab_entsz = 0;
	elf64_sword_t i;
	elf64_off_t relocation_offset = p_info->relocation_offset;

	if (mem_image_map_to_mem
		    (image, (void **)&dyn_section, (size_t)phdr_dyn->p_offset,
		    (size_t)dyn_section_sz) != dyn_section_sz) {
		ELF_PRINT_STRING("failed to read dynamic section from file\n");
		return MON_ERROR;
	}

	/* locate rela address, size, entry size */
	for (i = 0; i < dyn_section_sz / sizeof(elf64_dyn_t); ++i) {
		switch (dyn_section[i].d_tag) {
		case DT_RELA:
			rela =
				(elf64_rela_t *)(size_t)(dyn_section[i].d_un.d_ptr +
							 p_info->start_addr);
			break;
		case DT_RELASZ:
			rela_sz = (elf64_sword_t)dyn_section[i].d_un.d_val;
			break;
		case DT_RELAENT:
			rela_entsz = (elf64_sword_t)dyn_section[i].d_un.d_val;
			break;
		case DT_SYMTAB:
			symtab =
				(elf64_sym_t *)(size_t)(dyn_section[i].d_un.d_ptr +
							p_info->start_addr);
			break;
		case DT_SYMENT:
			symtab_entsz = (elf64_sword_t)dyn_section[i].d_un.d_val;
			break;
		default:
			break;
		}
	}
#if 0 /* TODO: enable the checks. */
	if (NULL == rela
	    || 0 == rela_sz
	    || NULL == symtab
	    || sizeof(elf64_rela_t) != rela_entsz
	    || sizeof(elf64_sym_t) != symtab_entsz) {
		ELF_PRINT_STRING("missed mandatory dynamic information\n");
		return MON_ERROR;
	}
#endif
	for (i = 0; i < rela_sz / rela_entsz; ++i) {
		elf64_addr_t *target_addr =
			(elf64_addr_t *)(size_t)(rela[i].r_offset +
						 relocation_offset);
		elf64_sword_t symtab_idx;

		switch (rela[i].r_info & 0xFF) {
		case R_X86_64_32:
			*target_addr = rela[i].r_addend + relocation_offset;
			symtab_idx = ((u64_t *)&rela[i].r_info)->hi;
			*target_addr += symtab[symtab_idx].st_value;
			break;
		case R_X86_64_RELATIVE:
			*target_addr = rela[i].r_addend + relocation_offset;
			break;
		case 0:        /* do nothing */
			break;
		default:
			ELF_PRINT_STRING("Unsupported Relocation 0x");
			ELF_PRINTLN_VALUE(rela[i].r_info & 0xFF);
			return MON_ERROR;
		}
	}

	return MON_OK;
}

/*
 *  FUNCTION  : elf64_copy_section_header_table
 *  PURPOSE   : Copy section header table at the end of the image,
 *            : update addresses of loaded sections and also update ELF header,
 *            : so it will point to section header table new location
 *  ARGUMENTS : image - describes image and access class
 *            : LOAD_ELF_PARAMS_T *params
 *    RETURNS : MON_OK if success
 *      NOTES : Should be called after the image is already loaded
 */
mon_status_t
elf64_copy_section_header_table(gen_image_access_t *image,
				elf_load_info_t *p_info)
{
	elf64_ehdr_t *ehdr = (elf64_ehdr_t *)(size_t)p_info->start_addr;
	uint8_t *shdrtab = (uint8_t *)(size_t)p_info->sections_addr;
	uint32_t section_hdr_tabsiz = ehdr->e_shentsize * ehdr->e_shnum;

	/* check to see if the elf header was copied to the target location
	 * normally this is the case, but not always.  If it is not copied then the
	 * program will not be able to find the section table in any event, and
	 * abort this copying of the section header table. */
	if (TRUE != elf64_header_is_valid(ehdr)) {
		ELF_PRINT_STRING("ELF header not present in target\n");
		return MON_ERROR;
	}

	/* copy sections headers table as is to new location (following program
	 * segments) */
	if (mem_image_read(image, (void *)shdrtab, (size_t)ehdr->e_shoff,
		    (size_t)section_hdr_tabsiz) != section_hdr_tabsiz) {
		ELF_PRINT_STRING("failed to copy section header table\n");
		return MON_ERROR;
	}

	ELF_PRINT_STRING("elf64 shtab =0x");
	ELF_PRINTLN_VALUE(ehdr);

	/* From now on we use new section headers table, so we can adjust the
	 * section header table offset to the new location. */
	ehdr->e_shoff = (elf64_off_t)(p_info->sections_addr - p_info->start_addr);

	return MON_OK;
}

mon_status_t
elf64_copy_sections(gen_image_access_t *image, elf_load_info_t *p_info)
{
	/* mon_status_t status = MON_OK; */
	elf64_ehdr_t *ehdr = (elf64_ehdr_t *)(size_t)p_info->start_addr;
	elf64_addr_t curr_addr;
	uint32_t section_hdr_tabsiz = ehdr->e_shentsize * ehdr->e_shnum;
	int16_t i;

	/* go through sections table, looking for non-empty sections */
	curr_addr = p_info->sections_addr + section_hdr_tabsiz;

	for (i = 0; i < ehdr->e_shnum; ++i) {
		elf64_shdr_t *shdr =
			(elf64_shdr_t *)(size_t)GET_SHDR_LOADER(ehdr,
				p_info->sections_addr,
				i);
		if (shdr->sh_addr != 0) {
			/* section was loaded as part of the segment loading. just relocate
			 * its address and continue */
			shdr->sh_addr += p_info->relocation_offset;
			continue;
		}

		if (0 == shdr->sh_size) {
			continue;
		}

		if (shdr->sh_addralign > 1) {
			curr_addr =
				(curr_addr +
				 (shdr->sh_addralign - 1)) & ~(shdr->sh_addralign -
							       1);
		}

		if (mem_image_read
			    (image, (void *)(size_t)curr_addr,
			    (size_t)shdr->sh_offset,
			    (size_t)shdr->sh_size) != shdr->sh_size) {
			ELF_PRINT_STRING("failed to read section from file\n");
			return MON_ERROR;
		}
		/* update section address */
		shdr->sh_addr = curr_addr;

		curr_addr += shdr->sh_size;
	}

	return MON_OK;
}
