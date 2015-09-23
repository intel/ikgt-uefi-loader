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

/*---------------------------------------------------*
 *
 * file		: x32_pt64.c
 * purpose	: Configures 4G memory space for 64-bit mode
 *			: while runnning in 32-bit mode
 *
 *----------------------------------------------------*/

#include "mon_defs.h"
#include "ia32_low_level.h"
#include "em64t_defs.h"
#include "x32_pt64.h"
#include "common.h"

extern void *CDECL mon_page_alloc(uint32_t pages);
#define XMON_LOADER_ASSERT(__condition) \
	{                              \
		if (!(__condition)) {  \
			while (1) {    \
			};             \
		}                      \
	}

static em64t_cr3_t cr3_for_x64 = { 0 };

/*---------------------------------------------------------*
*  FUNCTION		: x32_pt64_setup_paging
*  PURPOSE		: establish paging tables for x64 -bit mode, 2MB pages
*                               : while running in 32-bit mode.
*				: It should scope full 32-bit space, i.e. 4G
*  ARGUMENTS	:
*  RETURNS		: void
*---------------------------------------------------------*/
void x32_pt64_setup_paging(uint64_t memory_size)
{
	em64t_pml4_t *pml4_table;
	em64t_pdpe_t *pdp_table;
	em64t_pde_2mb_t *pd_table;

	uint32_t pdpt_entry_id;
	uint32_t pdt_entry_id;
	uint32_t address = 0;

	if (memory_size >= 0x100000000) {
		memory_size = 0x100000000;
	}

	/*
	 * To cover 4G-byte addrerss space the minimum set is PML4 - 1entry PDPT -
	 * 4 entries PDT - 2048 entries */

	pml4_table = (em64t_pml4_t *)mon_page_alloc(1);
	XMON_LOADER_ASSERT(pml4_table);
	mon_memset(pml4_table, 0, PAGE_4KB_SIZE);

	pdp_table = (em64t_pdpe_t *)mon_page_alloc(1);
	XMON_LOADER_ASSERT(pdp_table);
	mon_memset(pdp_table, 0, PAGE_4KB_SIZE);

	/* only one entry is enough in PML4 table */
	pml4_table[0].lo.base_address_lo = (uint32_t)pdp_table >> 12;
	pml4_table[0].lo.present = 1;
	pml4_table[0].lo.rw = 1;
	pml4_table[0].lo.us = 0;
	pml4_table[0].lo.pwt = 0;
	pml4_table[0].lo.pcd = 0;
	pml4_table[0].lo.accessed = 0;
	pml4_table[0].lo.ignored = 0;
	pml4_table[0].lo.zeroes = 0;
	pml4_table[0].lo.avl = 0;

	/* 4 entries is enough in PDPT */
	for (pdpt_entry_id = 0; pdpt_entry_id < 4; ++pdpt_entry_id) {
		pdp_table[pdpt_entry_id].lo.present = 1;
		pdp_table[pdpt_entry_id].lo.rw = 1;
		pdp_table[pdpt_entry_id].lo.us = 0;
		pdp_table[pdpt_entry_id].lo.pwt = 0;
		pdp_table[pdpt_entry_id].lo.pcd = 0;
		pdp_table[pdpt_entry_id].lo.accessed = 0;
		pdp_table[pdpt_entry_id].lo.ignored = 0;
		pdp_table[pdpt_entry_id].lo.zeroes = 0;
		pdp_table[pdpt_entry_id].lo.avl = 0;

		pd_table = (em64t_pde_2mb_t *)mon_page_alloc(1);
		XMON_LOADER_ASSERT(pd_table);
		mon_memset(pd_table, 0, PAGE_4KB_SIZE);
		pdp_table[pdpt_entry_id].lo.base_address_lo = (uint32_t)pd_table >>
							      12;

		for (pdt_entry_id = 0; pdt_entry_id < 512;
		     ++pdt_entry_id, address += PAGE_2MB_SIZE) {
			pd_table[pdt_entry_id].lo.present = 1;
			pd_table[pdt_entry_id].lo.rw = 1;
			pd_table[pdt_entry_id].lo.us = 0;
			pd_table[pdt_entry_id].lo.pwt = 0;
			pd_table[pdt_entry_id].lo.pcd = 0;
			pd_table[pdt_entry_id].lo.accessed = 0;
			pd_table[pdt_entry_id].lo.dirty = 0;
			pd_table[pdt_entry_id].lo.pse = 1;
			pd_table[pdt_entry_id].lo.global = 0;
			pd_table[pdt_entry_id].lo.avl = 0;
			pd_table[pdt_entry_id].lo.pat = 0; /* ???? */
			pd_table[pdt_entry_id].lo.zeroes = 0;
			pd_table[pdt_entry_id].lo.base_address_lo = address >> 21;
		}
	}

	cr3_for_x64.lo.pwt = 0;
	cr3_for_x64.lo.pcd = 0;
	cr3_for_x64.lo.base_address_lo = ((uint32_t)pml4_table) >> 12;
}

void x32_pt64_load_cr3(void)
{
	ia32_write_cr3(*((uint32_t *)&(cr3_for_x64.lo)));
}

uint32_t x32_pt64_get_cr3(void)
{
	return *((uint32_t *)&(cr3_for_x64.lo));
}
