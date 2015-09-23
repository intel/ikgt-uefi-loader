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

#include <mon_defs.h>
#include "cmdline.h"
#include "ctype.h"
#include "error_code.h"
#include "string.h"

const char *get_cmdline_value_str(const cmdline_string_options_t *cmdline,
				  const char *para_name)
{
	unsigned int i;

	if (cmdline == NULL || para_name == NULL) {
		return NULL;
	}

	for (i = 0;; i++) {
		/* the last one? */
		if (cmdline[i].name[0] == '\0') {
			break;
		}

		if (strcmp(para_name, cmdline[i].name) == 0) {
			return cmdline[i].def_val_str;
		}
	}

	return NULL;
}


/*
 *  cmdline: input cmdline string
 *  cmdoptions: output to it.
 */
unsigned int parse_cmdline(const char *cmdline, cmdline_string_options_t *cmdoptions)
{
	const char *p = cmdline;
	int i;

	if (p == NULL || cmdoptions == NULL) {
		return XMON_STRING_INVALID_PARAMETER;
	}

	while (true) {
		/* skip white space */
		while (isspace(*p))
			p++;

		/* the end */
		if (*p == '\0') {
			break;
		}

		/* find end of current option */
		const char *opt_start = p;
		const char *opt_end = strchr(opt_start, ' ');
		if (opt_end == NULL) {
			opt_end = opt_start + strlen(opt_start);
		}

		p = opt_end;

		/* find value part; if no value found, use default and continue */
		const char *val_start = strchr(opt_start, '=');
		if (val_start == NULL || val_start > opt_end) {
			/* doesn't find value */
			for (i = 0; cmdoptions[i].name[0] != '\0'; i++) {
				if (strncmp(cmdoptions[i].name, opt_start,
					    opt_end - opt_start) == 0) {
					/* update option present flag */
					const char *option_present =
						CMD_OPTION_PRESENT_BUT_NO_VALUE;
					strncpy(cmdoptions[i].def_val_str,
						option_present,
						strlen(option_present) + 1);
					break;
				}
			}
			continue;
		}
		val_start++;

		unsigned int opt_name_size = val_start - opt_start - 1;
		unsigned int copy_size = opt_end - val_start;

		if (copy_size > MAX_CMDLINE_VALUE_STRING_LEN - 1) {
			return XMON_CMDLINE_VALUE_STRING_LEN_TOO_LARGE;
		}

		if (opt_name_size == 0 || copy_size == 0) {
			continue;
		}

		/* value found, so copy it */
		for (i = 0; cmdoptions[i].name[0] != '\0'; i++) {
			if (strncmp(cmdoptions[i].name, opt_start,
				    opt_name_size) == 0) {
				strncpy(cmdoptions[i].def_val_str,
					val_start,
					copy_size);
				cmdoptions[i].def_val_str[copy_size] = '\0'; /* add '\0' to the end of string */
				break;
			}
		}
	}

	return XMON_LOADER_SUCCESS;
}

