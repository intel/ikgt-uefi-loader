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
 * file      : x32_init64.h
 * purpose   : jump to 64-bit execution mode
 *
 *----------------------------------------------------*/

#ifndef _X32_INIT64_H_
#define _X32_INIT64_H_

#include "ia32_defs.h"
#include "common_types.h"

typedef struct _INIT32_STRUCT {
	uint32_t i32_low_memory_page;           /* address of page in low memory, used for AP bootstrap */
	uint16_t i32_num_of_aps;                /* number of detected APs (Application Processors) */
	uint16_t i32_pad;
	uint32_t i32_esp[MAX_CPUS];             /* array of 32-bit SPs (SP - top of the stack) */
} init32_struct_t;

typedef struct _INIT64_STRUCT {
	uint16_t i64_cs;                /* 64-bit code segment selector */
	ia32_gdtr_t i64_gdtr;           /* still in 32-bit format */
	uint64_t i64_efer;              /* EFER minimal required value */
	uint32_t i64_cr3;               /* 32-bit value of CR3 */
} init64_struct_t;

void x32_init64_setup(void);

void x32_init64_start(init64_struct_t *p_init64_data,
		      uint32_t address_of_64bit_code,
		      void *arg1,
		      void *arg2,
		      void *arg3,
		      void *arg4);

#endif                          /* _X32_INIT64_H_ */
