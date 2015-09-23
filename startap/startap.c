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
#include "x32_init64.h"
#include "ap_procs_init.h"
#include "mon_startup.h"
#include "startap.h"
typedef struct {
	void *any_data1;
	void *any_data2;
	void *any_data3;
	uint64_t ep;
} application_params_struct_t;

static application_params_struct_t application_params;
static init64_struct_t *gp_init64;

/*------------------Forward Declarations for Local Functions------------------*/
static void CDECL start_application(uint32_t cpu_id,
				    const application_params_struct_t *params);
void CDECL startap_main(init32_struct_t *p_init32, init64_struct_t *p_init64,
			mon_startup_struct_t *p_startup, uint32_t entry_point)
{
	uint32_t application_procesors;

	if (NULL != p_init32) {
		/* wakeup APs */
		application_procesors = ap_procs_startup(p_init32, p_startup);
	} else {
		application_procesors = 0;
	}
#ifdef UNIPROC
	application_procesors = 0;
#endif

	if (BITMAP_GET(p_startup->flags, MON_STARTUP_POST_OS_LAUNCH_MODE) == 0) {
		/* update the number of processors in mon_startup_struct_t for pre os
		 * launch */
		p_startup->number_of_processors_at_boot_time =
			application_procesors + 1;
	}

	application_params.ep = entry_point;
	application_params.any_data1 = (void *)p_startup;
	application_params.any_data2 = NULL;
	application_params.any_data3 = NULL;

	/* first launch application on AP cores */
	if (application_procesors > 0) {
		ap_procs_run((func_continue_ap_t)start_application,
			&application_params);
	}

	/* and then launch application on BSP */
	start_application(0, &application_params);
}

static void CDECL start_application
	(uint32_t cpu_id, const application_params_struct_t *params)
{
	xmon_image_entry_point_t xmon_entry;
	xmon_entry = (xmon_image_entry_point_t)params->ep;

	call_xmon_entry(xmon_entry,cpu_id, params->any_data1, params->any_data2,params->any_data3);
	/*should never return here!*/
	while(1);
}
