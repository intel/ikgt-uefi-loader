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

#ifndef BOOT_PROTOCOL_UTIL_H
#define BOOT_PROTOCOL_UTIL_H

#include "xmon_desc.h"

#define true 1
#define false 0

typedef struct {
	char name[10];

	boolean_t (*get_e820_table)(xmon_desc_t *td, uint64_t *e820_addr);
	boolean_t (*hide_runtime_memory)(xmon_desc_t *xd, uint32_t hide_mem_addr,
					uint32_t hide_mem_size);
} boot_protocol_ops_t;

boolean_t protocol_ops_init(uint32_t boot_magic);
boolean_t loader_get_e820_table(xmon_desc_t *td, uint64_t *e820_addr);
boolean_t loader_hide_runtime_memory(xmon_desc_t *xd,
				    uint32_t hide_mem_addr,
				    uint32_t hide_mem_size);

#endif    /* BOOT_PROTOCOL_UTIL_H */
