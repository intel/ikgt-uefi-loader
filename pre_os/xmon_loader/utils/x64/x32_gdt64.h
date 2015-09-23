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
 * file		: x32_gdt64.h
 * purpose	: copy existing 32-bit GDT and
 *			: expands it with 64-bit mode entries
 *
 *----------------------------------------------------*/

#ifndef _X32_GDT64_H_
#define _X32_GDT64_H_

#include "ia32_defs.h"

void x32_gdt64_setup(void);
void x32_gdt64_load(void);
uint16_t x32_gdt64_get_cs(void);
void x32_gdt64_get_gdtr(ia32_gdtr_t *p_gdtr);

#endif                          /* _X32_GDT64_H_ */
