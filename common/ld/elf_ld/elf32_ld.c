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
#include "elf32.h"
#include "elf32_ld.h"
#include "elf_info.h"
#include "image_access_mem.h"
#include "elf_ld_env.h"

#define STINGS_ARE_EQUAL(__s1, __s2) (0 == strcmp(__s1, __s2))

void *mon_memset(void *dest, int val, size_t count);

/* prototypes of the real elf parsing functions */
static mon_status_t elf32_copy_sections(gen_image_access_t *image,
					elf_load_info_t *p_info);
static mon_status_t elf32_do_relocation(gen_image_access_t *image,
					elf_load_info_t *p_info,
					elf32_phdr_t *phdr_dyn);
static mon_status_t elf32_copy_section_header_table(gen_image_access_t *image,
						    elf_load_info_t *p_info);

/*
 *  FUNCTION  : elf32_get_load_info
 *  PURPOSE   : Get load-related information
 *  ARGUMENTS : gen_image_access_t *image - describes image to load
 *            : elf_load_info_t *p_info -
 *            :          contains both input and output load-related data
 *    RETURNS :
 */
mon_status_t
elf32_get_load_info(gen_image_access_t *image, elf_load_info_t *p_info)
{
	elf32_ehdr_t *ehdr;             /* ELF header */
	uint8_t *phdrtab;               /* Program Segment header Table */
	uint8_t *shdrtab;               /* sections header Table */
	elf32_addr_t low_addr = (elf32_addr_t) ~0;
	elf32_addr_t max_addr = 0;
	elf32_addr_t addr;
	uint32_t phsize;
	uint32_t memsz;
	int16_t i;
	mon_status_t status = MON_OK;
	uint32_t shsize;

	ELF_CLEAR_SCREEN();

	/* map ELF header to ehdr. */
	if (sizeof(elf32_ehdr_t) !=
	    mem_image_map_to_mem(image, (void **)&ehdr, (size_t)0,
		    (size_t)sizeof(elf32_ehdr_t))) {
		status = MON_ERROR;
		goto quit;
	}

	if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
		ELF_PRINT_STRING("ELF file type not executable. Type = 0x");
		ELF_PRINTLN_VALUE(ehdr->e_type);
		status = MON_ERROR;
		goto quit;
	}

	/* map Program Segment header Table to phdrtab. */
	phsize = ehdr->e_phnum * ehdr->e_phentsize;
	if (mem_image_map_to_mem
		    (image, (void **)&phdrtab, (size_t)ehdr->e_phoff,
		    (size_t)phsize) != phsize) {
		status = MON_ERROR;
		goto quit;
	}

	/* Calculate amount of memory required. First calculate size of all
	 * loadable segments */

	ELF_PRINT_STRING
		("addr_start.addr_end :offset :filesz "
		":memsz :bss_sz :p_type\n");
	for (i = 0; i < (int16_t)ehdr->e_phnum; ++i) {
		elf32_phdr_t *phdr = (elf32_phdr_t *)GET_PHDR(ehdr, phdrtab, i);

		addr = phdr->p_paddr;
		memsz = phdr->p_memsz;

		ELF_PRINT_VALUE(addr);
		ELF_PRINT_VALUE(addr + memsz);
		ELF_PRINT_VALUE(phdr->p_offset);
		ELF_PRINT_VALUE(phdr->p_filesz);
		ELF_PRINT_VALUE(memsz);
		ELF_PRINT_VALUE(memsz - phdr->p_filesz);
		ELF_PRINT_VALUE(phdr->p_type);

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
		ELF_PRINT_STRING(
			"Failed because kernel low address is not page aligned, low_addr = 0x");
		ELF_PRINTLN_VALUE(low_addr);
		status = MON_ERROR;
		goto quit;
	}

	/* now calculate amount of memory required for optional tables */
	if (p_info->copy_section_headers || p_info->copy_symbol_tables) {
		/* reserve place for section headers table */
		/* align sections table on 16-byte boundary */
		max_addr = (max_addr + 0xF) & ~0xF;
		/* store for further usage */
		p_info->sections_addr = max_addr;
		/* size of header sections table */
		shsize = ehdr->e_shnum * ehdr->e_shentsize;
		/* add size of sections table */
		max_addr += shsize;

		if (p_info->copy_symbol_tables) {
			/* read sections header Table */
			if (mem_image_map_to_mem
				    (image, (void **)&shdrtab, (size_t)ehdr->e_shoff,
				    (size_t)shsize) != shsize) {
				status = MON_ERROR;
				goto quit;
			}

			/* go through sections table, looking for non-empty sections */
			for (i = 0; i < ehdr->e_shnum; ++i) {
				elf32_shdr_t *shdr = (elf32_shdr_t *)GET_SHDR_LOADER(ehdr,
					shdrtab,
					i);
				if (0 == shdr->sh_size) {
					/* skip empty sections */
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
 *  FUNCTION  : elf32_load_executable
 *  PURPOSE   : Load and relocate ELF-x32 executable to memory
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
elf32_load_executable(gen_image_access_t *image, elf_load_info_t *p_info)
{
	mon_status_t status = MON_OK;
	elf32_ehdr_t *ehdr;             /* ELF header */
	uint8_t *phdrtab;               /* Program Segment header Table */
	elf32_word_t phsize;            /* Program Segment header Table size */
	elf32_addr_t addr;
	elf32_word_t memsz;
	elf32_word_t filesz;
	int16_t i;
	elf32_phdr_t *phdr_dyn = NULL;

	ELF_CLEAR_SCREEN();

	/* map ELF header to ehdr */
	if (sizeof(elf32_ehdr_t) !=
	    mem_image_map_to_mem(image, (void **)&ehdr, (size_t)0,
		    (size_t)sizeof(elf32_ehdr_t))) {
		status = MON_ERROR;
		goto quit;
	}

	/* map Program Segment header Table to phdrtab */
	phsize = ehdr->e_phnum * ehdr->e_phentsize;
	if (mem_image_map_to_mem
		    (image, (void **)&phdrtab, (size_t)ehdr->e_phoff,
		    (size_t)phsize) != phsize) {
		status = MON_ERROR;
		goto quit;
	}

	ELF_PRINT_STRING
		("p_type :p_flags :p_offset:p_vaddr :p_paddr "
		":p_filesz:p_memsz :p_align\n");
	/* now actually copy image to its target destination */
	for (i = 0; i < (int16_t)ehdr->e_phnum; ++i) {
		elf32_phdr_t *phdr = (elf32_phdr_t *)GET_PHDR(ehdr, phdrtab, i);
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
			    (size_t)phdr->p_offset, (size_t)filesz) != filesz) {
			status = MON_ERROR;
			ELF_PRINT_STRING("failed to read segment from file.\n");
			goto quit;
		}

		if (filesz < memsz) { /* zero BSS if exists */
			mon_memset((void *)(size_t)(addr + filesz +
						    p_info->relocation_offset), 0,
				(size_t)(memsz - filesz));
		}
	}

	/* Update copied segments addresses */
	/* now ehdr points to the new, copied ELF header */
	ehdr = (elf32_ehdr_t *)(size_t)p_info->start_addr;
	phdrtab = (uint8_t *)(size_t)(p_info->start_addr + ehdr->e_phoff);

	for (i = 0; i < (int16_t)ehdr->e_phnum; ++i) {
		elf32_phdr_t *phdr = (elf32_phdr_t *)GET_PHDR(ehdr, phdrtab, i);

		if (0 != phdr->p_memsz) {
			phdr->p_paddr += (elf32_addr_t)p_info->relocation_offset;
			phdr->p_vaddr += (elf32_addr_t)p_info->relocation_offset;
		}
	}

	if (NULL != phdr_dyn) {
		status = elf32_do_relocation(image, p_info, phdr_dyn);
		if (MON_OK != status) {
			goto quit;
		}
	}

	/* optionally copy sections table and sections */

	if (p_info->copy_section_headers || p_info->copy_symbol_tables) {
		status = elf32_copy_section_header_table(image, p_info);
	}

	if (p_info->copy_symbol_tables) {
		status = elf32_copy_sections(image, p_info);
	}

quit:
	return status;
}

mon_status_t
elf32_do_relocation(gen_image_access_t *image,
		    elf_load_info_t *p_info, elf32_phdr_t *phdr_dyn)
{
	elf32_dyn_t *dyn_section;
	elf32_sword_t dyn_section_sz = phdr_dyn->p_filesz;
	elf32_rela_t *rela = NULL;
	elf32_sword_t rela_sz = 0;
	elf32_sword_t rela_entsz = 0;
	elf32_sym_t *symtab = NULL;
	elf32_sword_t symtab_entsz = 0;
	elf32_sword_t i;
	elf32_rel_t *rel = NULL;
	elf32_sword_t rel_sz = 0;
	elf32_sword_t rel_entsz = 0;

	elf32_off_t relocation_offset = (elf32_off_t)p_info->relocation_offset;

	if (mem_image_map_to_mem
		    (image, (void **)&dyn_section, (size_t)phdr_dyn->p_offset,
		    (size_t)dyn_section_sz) != dyn_section_sz) {
		ELF_PRINT_STRING("failed to read dynamic section from file.\n");
		return MON_ERROR;
	}

	/* locate rela address, size, entry size */
	for (i = 0; i < dyn_section_sz / sizeof(elf32_dyn_t); ++i) {
		if (DT_RELA == dyn_section[i].d_tag) {
			rela =
				(elf32_rela_t *)(size_t)(dyn_section[i].d_un.d_ptr +
							 p_info->start_addr);
		}

		if (DT_RELASZ == dyn_section[i].d_tag) {
			rela_sz = (elf32_sword_t)dyn_section[i].d_un.d_val;
		}

		if (DT_RELAENT == dyn_section[i].d_tag) {
			rela_entsz = (elf32_sword_t)dyn_section[i].d_un.d_val;
		}

		if (DT_SYMTAB == dyn_section[i].d_tag) {
			symtab =
				(elf32_sym_t *)(size_t)(dyn_section[i].d_un.d_ptr +
							p_info->start_addr);
		}

		if (DT_SYMENT == dyn_section[i].d_tag) {
			symtab_entsz = (elf32_sword_t)dyn_section[i].d_un.d_val;
		}

		if (DT_REL == dyn_section[i].d_tag) {
			rel =
				(elf32_rel_t *)(size_t)(dyn_section[i].d_un.d_ptr +
							p_info->start_addr);
		}

		if (DT_RELSZ == dyn_section[i].d_tag) {
			rel_sz = (elf32_sword_t)dyn_section[i].d_un.d_val;
		}

		if (DT_RELENT == dyn_section[i].d_tag) {
			rel_entsz = (elf32_sword_t)dyn_section[i].d_un.d_val;
		}
	}

	/* handle DT_RELA tag: */
	if ((NULL != rela)
	    && rela_sz && (NULL != symtab)
	    && (sizeof(elf32_rela_t) == rela_entsz)
	    && (sizeof(elf32_sym_t) == symtab_entsz)) {
		for (i = 0; i < rela_sz / rela_entsz; ++i) {
			elf32_addr_t *target_addr =
				(elf32_addr_t *)(uint64_t)(rela[i].r_offset +
						 relocation_offset);
			elf32_sword_t symtab_idx;

			switch (rela[i].r_info & 0xFF) {
			case R_386_32:
				*target_addr = rela[i].r_addend + relocation_offset;
				symtab_idx = rela[i].r_info >> 8;
				*target_addr += symtab[symtab_idx].st_value;
				break;
			case R_386_RELATIVE:
				*target_addr = rela[i].r_addend + relocation_offset;
				break;
			case 0: /* do nothing */
				break;
			default:
				ELF_PRINT_STRING(
					"Unsupported Relocation. rlea.r_info = 0x");
				ELF_PRINTLN_VALUE(rela[i].r_info & 0xFF);
				return MON_ERROR;
			}
		}

		return MON_OK;
	}

	/* handle DT_REL tag: */
	if ((NULL != rel) && (NULL != symtab)
	    && rel_sz && (sizeof(elf32_rel_t) == rel_entsz)) {
		/* Only elf32_rela_t and elf64_rela_t entries contain an explicit addend.
		 * Entries of type elf32_rel_t and elf64_rel_t store an implicit addend in
		 * the location to be modified. Depending on the processor
		 * architecture, one form or the other might be necessary or more
		 * convenient. Consequently, an implementation for a particular machine
		 * may use one form exclusively or either form depending on context. */
		for (i = 0; i < rel_sz / rel_entsz; ++i) {
			elf32_addr_t *target_addr =
				(elf32_addr_t *)(uint64_t)(rel[i].r_offset +
						 relocation_offset);
			elf32_sword_t symtab_idx;

			switch (rel[i].r_info & 0xFF) {
			case R_386_32:
				*target_addr += relocation_offset;
				symtab_idx = rel[i].r_info >> 8;
				*target_addr += symtab[symtab_idx].st_value;
				break;

			case R_386_RELATIVE:
				/* read the dword at this location, add it to the run-time
				 * start address of this module; deposit the result back into
				 * this dword */
				*target_addr += relocation_offset;
				break;

			default:
				ELF_PRINT_STRING(
					"Unsupported Relocation, rel.r_info = 0x");
				ELF_PRINTLN_VALUE(rel[i].r_info & 0xFF);
				return MON_ERROR;
			}
		}

		return MON_OK;
	}

	return MON_ERROR;
}

/*
 *  FUNCTION  : elf32_copy_section_header_table
 *  PURPOSE   : Copy section header table at the end of the image, update
 *            : of loaded sections and also update ELF header,
 *            : addresses so it will point to section header table new location
 *  ARGUMENTS : image - describes image and access class
 *            : LOAD_ELF_PARAMS_T *params
 *    RETURNS : MON_OK if success
 *      NOTES : Should be called after the image is already loaded
 */
mon_status_t
elf32_copy_section_header_table(gen_image_access_t *image,
				elf_load_info_t *p_info)
{
	/* notice that it is the new, copied ELF header */
	elf32_ehdr_t *ehdr = (elf32_ehdr_t *)(size_t)p_info->start_addr;
	/* sections header Table */
	uint8_t *shdrtab = (uint8_t *)(size_t)p_info->sections_addr;
	uint32_t section_hdr_tabsiz = ehdr->e_shentsize * ehdr->e_shnum;

	/* check to see if the elf header was copied to the target location
	 * normally this is the case, but not always.  If it is not copied then the
	 * program will not be able to find the section table in any event, and
	 * abort this copying of the section header table. */
	if (TRUE != elf32_header_is_valid(ehdr)) {
		ELF_PRINT_STRING("ELF header not present in target\n");
		return MON_ERROR;
	}

	/* copy sections headers table as is to new location (following program
	 * segments) */
	if (mem_image_read
		    (image, (void *)shdrtab, (size_t)ehdr->e_shoff,
		    (size_t)section_hdr_tabsiz) != section_hdr_tabsiz) {
		ELF_PRINT_STRING("failed to copy section header table\n");
		return MON_ERROR;
	}

	ELF_PRINT_STRING("elf32 shtab = 0x");
	ELF_PRINTLN_VALUE(ehdr);

	/* From now on we use new section headers table, so we can adjust the
	 * section header table offset to the new location. */
	ehdr->e_shoff = (elf32_off_t)(p_info->sections_addr - p_info->start_addr);

	return MON_OK;
}

mon_status_t
elf32_copy_sections(gen_image_access_t *image, elf_load_info_t *p_info)
{
	/* note that it is the new, copied ELF header */
	elf32_ehdr_t *ehdr = (elf32_ehdr_t *)(size_t)p_info->start_addr;
	elf32_addr_t curr_addr;
	uint32_t section_hdr_tabsiz = ehdr->e_shentsize * ehdr->e_shnum;
	int16_t i;

	/* go through sections table, looking for non-empty sections */
	curr_addr = (elf32_addr_t)(p_info->sections_addr + section_hdr_tabsiz);

	for (i = 0; i < ehdr->e_shnum; ++i) {
		/* Section header(i) */
		elf32_shdr_t *shdr =
			(elf32_shdr_t *)((size_t)GET_SHDR_LOADER(ehdr,
						p_info->sections_addr,
						i));
		if (shdr->sh_addr != 0) {
			/* section was loaded as part of the segment loading. just relocate
			 * its address and continue */
			shdr->sh_addr += (elf32_off_t)p_info->relocation_offset;
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
