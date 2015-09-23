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

#include "mon_defs.h"
#include "mon_arch_defs.h"
#include "image_loader.h"
#include "xmon_desc.h"
#include "error_code.h"
static uint64_t get_xmon_loader_img_base(xmon_desc_t *xmon_desc)
{
	xmon_loader_memory_layout_t *ldr_mem;

	ldr_mem = (xmon_loader_memory_layout_t *)xmon_desc->loader_mem_addr;

	return (uint64_t)ldr_mem->u_xmon_loader.img_base;
}

int run_xmon_loader(xmon_desc_t *xd)
{
	boolean_t ok;
	void *img;
	image_info_status_t image_info_status;
	image_info_t img_info;

	uint64_t (*xmon_loader) (xmon_desc_t *xd);

	img = (void *)(xd->xmon_loader_file.addr);
	if (!img) {
		return STARTER_INVALID_XMON_LOADER_BIN_ADDR;
	}

	image_info_status = get_image_info(img,
		xd->xmon_loader_file.size,
		&img_info);

	if ((image_info_status != IMAGE_INFO_OK) ||
	    (img_info.machine_type != IMAGE_MACHINE_EM64T) ||
	    (img_info.load_size == 0) ||
		(img_info.load_size > XMON_LOADER_IMG_SIZE)) {
		return STARTER_FAILED_TO_GET_XMON_LOADER_IMG_INFO;
	}

	ok = load_image((void *)img,
		(void *)get_xmon_loader_img_base(xd),
		img_info.load_size,
		(uint64_t *)&xmon_loader);

	if (!ok) {
		return STARTER_FAILED_TO_RELOCATE_XMON_LOADER;
	}

	return xmon_loader(xd);
}

/* End of file */
