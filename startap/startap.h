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

#ifndef _STARTAP_H_
#define _STARTAP_H_

#include "x32_init64.h"
#include "ap_procs_init.h"
#include "mon_startup.h"

typedef void (CDECL * xmon_image_entry_point_t)(uint32_t local_apic_id,
        void *any_data1,
        void *any_data2, void *any_data3);

void CDECL call_xmon_entry(xmon_image_entry_point_t xmon_entry,
                                  uint32_t local_apic_id,
                                  void *any_data1,
                                  void *any_data2,
                                  void *any_data3);

typedef void (CDECL * startap_image_entry_point_t)(init32_struct_t *p_init32,
	init64_struct_t *p_init64,
	mon_startup_struct_t *
	p_startup,
	uint64_t entry_point);

#endif                          /* _STARTAP_H_ */
