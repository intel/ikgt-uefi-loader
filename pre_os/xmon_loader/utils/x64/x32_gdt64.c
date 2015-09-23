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
 * file		: x32_gdt64.c
 * purpose	: copy existing 32-bit GDT and
 *			: expands it with 64-bit mode entries
 *
 *----------------------------------------------------*/

#include "mon_defs.h"
#include "em64t_defs.h"
#include "ia32_low_level.h"
#include "x32_gdt64.h"
#include "common.h"

extern void *CDECL mon_page_alloc(uint32_t pages);
extern void clear_screen();
extern void print_string(uint8_t *string);
extern void print_value(uint32_t value);
#define XMON_LOADER_ASSERT(__condition) \
	{                               \
		if (!(__condition)) {   \
			while (1) {     \
			}               \
		}                       \
	}

void print_gdt(uint32_t base, uint16_t limit);

static ia32_gdtr_t gdtr_32;
static ia32_gdtr_t gdtr_64;       /* still in 32-bit mode */
static uint16_t cs_64;

void x32_gdt64_setup(void)
{
	em64t_code_segment_descriptor_t *p_gdt_64;
	uint32_t last_index;

	/* allocate page for 64-bit GDT */
	/* 1 page should be sufficient ??? */
	p_gdt_64 = mon_page_alloc(1);
	XMON_LOADER_ASSERT(p_gdt_64);
	mon_memset(p_gdt_64, 0, PAGE_4KB_SIZE);

	/* read 32-bit GDTR */
	ia32_read_gdtr(&gdtr_32);

	/* clone it to the new 64-bit GDT */
	mon_memcpy(p_gdt_64, (void *)gdtr_32.base, gdtr_32.limit + 1);

	/* build and append to GDT 64-bit mode code-segment entry */
	/* check if the last entry is zero, and if so, substitute it */

	last_index = gdtr_32.limit / sizeof(em64t_code_segment_descriptor_t);

	if (*(uint64_t *)&p_gdt_64[last_index] != 0) {
		last_index++;
	}

	/* code segment for xmon code */

	p_gdt_64[last_index].hi.accessed = 0;
	p_gdt_64[last_index].hi.readable = 1;
	p_gdt_64[last_index].hi.conforming = 1;
	p_gdt_64[last_index].hi.mbo_11 = 1;
	p_gdt_64[last_index].hi.mbo_12 = 1;
	p_gdt_64[last_index].hi.dpl = 0;
	p_gdt_64[last_index].hi.present = 1;
	p_gdt_64[last_index].hi.long_mode = 1;          /* important !!! */
	p_gdt_64[last_index].hi.default_size = 0;       /* important !!! */
	p_gdt_64[last_index].hi.granularity = 1;

	/* data segment for xmon stacks */

	p_gdt_64[last_index + 1].hi.accessed = 0;
	p_gdt_64[last_index + 1].hi.readable = 1;
	p_gdt_64[last_index + 1].hi.conforming = 0;
	p_gdt_64[last_index + 1].hi.mbo_11 = 0;
	p_gdt_64[last_index + 1].hi.mbo_12 = 1;
	p_gdt_64[last_index + 1].hi.dpl = 0;
	p_gdt_64[last_index + 1].hi.present = 1;
	p_gdt_64[last_index + 1].hi.long_mode = 1;      /* important !!! */
	p_gdt_64[last_index + 1].hi.default_size = 0;   /* important !!! */
	p_gdt_64[last_index + 1].hi.granularity = 1;

	/* prepare GDTR */
	gdtr_64.base = (uint32_t)p_gdt_64;
	/* !!! * TBD * !!! will be extended by TSS */
	gdtr_64.limit = gdtr_32.limit + sizeof(em64t_code_segment_descriptor_t) * 2;
	cs_64 = last_index * sizeof(em64t_code_segment_descriptor_t);
}

void x32_gdt64_load(void)
{
	ia32_write_gdtr(&gdtr_64);
}

uint16_t x32_gdt64_get_cs(void)
{
	return cs_64;
}

void x32_gdt64_get_gdtr(ia32_gdtr_t *p_gdtr)
{
	*p_gdtr = gdtr_64;
}

void print_gdt(uint32_t base, uint16_t limit)
{
	uint32_t *tab;
	uint16_t i;

	if (0 == base) {
		ia32_gdtr_t gdtr;
		ia32_read_gdtr(&gdtr);
		base = gdtr.base;
		limit = gdtr.limit;
	}

	print_string("GDT BASE = ");
	print_value((uint32_t)base);
	print_string("GDT LIMIT = ");
	print_value((uint32_t)limit);
	print_string("\n");

	tab = (uint32_t *)base;

	for (i = 0; i < (limit + 1) / sizeof(ia32_code_segment_descriptor_t); ++i) {
		print_value((uint32_t)i);
		print_string("....");
		print_value(tab[i * 2]);
		print_string(" ");
		print_value(tab[i * 2 + 1]);
		print_string("\n");
	}
}
