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


#include "common_types.h"
#include "mon_defs.h"
#include "mon_arch_defs.h"
#include "mon_startup.h"
#include "startap.h"
#include "loader.h"
#include "xmon_desc.h"
#include "common.h"
#include "boot_protocol_util.h"
#include "e820.h"
#include "ikgtboot.h"

static boot_protocol_ops_t *boot_protocol_ops;

static boot_protocol_ops_t default_ops = {
    .name   = "default",
};

/* ikgt boot header ops*/
static boot_protocol_ops_t ibh_ops = {
	.name			= "ibh",
	.get_e820_table		= get_e820_table_from_ibh,
};

boolean_t protocol_ops_init(uint32_t boot_magic)
{
	if (boot_magic == IKGT_BOOTLOADER_MAGIC) {
		boot_protocol_ops = &ibh_ops;
		return true;
	} else {
		boot_protocol_ops = &default_ops;
		return false;
	}
}

boolean_t loader_get_e820_table(xmon_desc_t *xd, uint64_t *e820_addr)
{
	if (boot_protocol_ops->get_e820_table) {
		return boot_protocol_ops->get_e820_table(xd, e820_addr);
	} else {
		return false;
	}
}

boolean_t loader_hide_runtime_memory(xmon_desc_t *xd, uint32_t hide_mem_addr,
				    uint32_t hide_mem_size)
{
	if (boot_protocol_ops->hide_runtime_memory) {
		return boot_protocol_ops->hide_runtime_memory(xd, hide_mem_addr,
			hide_mem_size);
	} else {
		/* No need to hide runtime memory
		 * hide_run_time_memory() is not a must for some boot loaders,
		 * for instance, EFI Kernelflinger
		 */
		return true;
	}
}

