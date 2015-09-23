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
 * file		: x32_pt64.h
 * purpose	: Configures 4G memory space for 64-bit mode
 *			: while runnning in 32-bit mode
 *
 *
 *----------------------------------------------------*/
#ifndef _X32_PTT64_H_
#define _X32_PTT64_H_

/*---------------------------------------------------------*
*  FUNCTION		: x32_pt64_setup_paging
*  PURPOSE		: establish paging tables for x64 -bit mode, 2MB pages
*                               : while running in 32-bit mode.
*				: It should scope full 32-bit space, i.e. 4G
*  ARGUMENTS	:
*  RETURNS		: void
*---------------------------------------------------------*/
void x32_pt64_setup_paging(uint64_t memory_size);

void x32_pt64_load_cr3(void);

uint32_t x32_pt64_get_cr3(void);

#endif                          /* _X32_INIT64_H_ */
