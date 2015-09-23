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
 * file      : x32_init64.c
 * purpose   : implement transition to 64-bit execution mode
 *
 *----------------------------------------------------*/

#include "mon_defs.h"
#include "ia32_low_level.h"
#include "x32_init64.h"

#define PSE_BIT     0x10
#define PAE_BIT     0x20

extern void CDECL start_64bit_mode(uint32_t address,
				   /* MUST BE 32-bit wide, because it delivered to 64-bit
				    * code using 32-bit push/retf commands */
				   uint32_t segment,
				   uint32_t *arg1,
				   uint32_t *arg2,
				   uint32_t *arg3,
				   uint32_t *arg4);

void x32_init64_start(init64_struct_t *p_init64_data,
		      uint32_t address_of_64bit_code,
		      void *arg1, void *arg2, void *arg3, void *arg4)
{
	uint32_t cr4;

	ia32_write_gdtr(&p_init64_data->i64_gdtr);
	ia32_write_cr3(p_init64_data->i64_cr3);
	cr4 = ia32_read_cr4();
	BITMAP_SET(cr4, PAE_BIT | PSE_BIT);
	ia32_write_cr4(cr4);
	ia32_write_msr(0xC0000080, &p_init64_data->i64_efer);

	start_64bit_mode(address_of_64bit_code, p_init64_data->i64_cs, arg1, arg2,
		arg3, arg4);
}
