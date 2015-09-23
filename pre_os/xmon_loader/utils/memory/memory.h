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

#ifndef MEMORY_H
#define MEMORY_H

#include "common_types.h"

void_t zero_mem(void_t *address, uint64_t size);

void_t *allocate_memory(uint64_t size);

void_t print_e820_bios_memory_map(void_t);

void_t initialize_memory_manager(uint64_t heap_base_address, uint64_t heap_bytes);

void_t copy_mem(void_t *dest, void_t *source, uint64_t size);

boolean_t compare_mem(void_t *source1, void_t *source2, uint64_t size);

void *CDECL mon_page_alloc(uint64_t pages);

#endif                          /* MEMORY_H */
