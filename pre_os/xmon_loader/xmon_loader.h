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

#ifndef XMON_LOADER_H
#define XMON_LOADER_H

#include "xmon_desc.h"

#ifdef DEBUG
#define PRINT_STRING(arg) print_string((uint8_t *)arg)
#define PRINT_VALUE(arg)  print_value((uint32_t)arg)
#define PRINT_STRING_AND_VALUE(string, value) { PRINT_STRING(string); \
						PRINT_VALUE(value); \
						PRINT_STRING("\n"); }
#else
#define PRINT_STRING(arg)
#define PRINT_VALUE(arg)
#define PRINT_STRING_AND_VALUE(string, value)
#endif

void setup_idt(void);
void parse_mb_xmon_cmdline_mb1(xmon_desc_t *xd);
void parse_mb_xmon_cmdline_mb2(xmon_desc_t *xd);

#endif    /* XMON_LOADER_H */
