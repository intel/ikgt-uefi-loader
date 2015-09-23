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

#ifndef __CMDLINE_H__
#define __CMDLINE_H__

#define CMD_OPTION_PRESENT_BUT_NO_VALUE  "OPTIONONLY"  /* has option, but no value for this option */


#define MAX_CMDLINE_NAME_STRING_LEN     64
#define MAX_CMDLINE_VALUE_STRING_LEN    64

typedef struct {
	/* option name: "\0" as the last one */
	const char name[MAX_CMDLINE_NAME_STRING_LEN];

	/* default value string */
	char def_val_str[MAX_CMDLINE_VALUE_STRING_LEN];
} __attribute__((packed)) cmdline_string_options_t;


const char *get_cmdline_value_str(const cmdline_string_options_t *cmdline,
				  const char *para_name);
unsigned int parse_cmdline(const char *cmdline, cmdline_string_options_t *cmdoptions);

#endif
