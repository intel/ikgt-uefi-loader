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
#include <xmon_loader.h>
#include <screen.h>
#include "loader_serial.h"

/* There are actually 50, but we start to count from 0 */
#define MAX_ROWS      49
#define MAX_COLOUMNS  80

#define VGA_BASE_ADDRESS 0xB8000

static uint8_t *cursor = (uint8_t *)VGA_BASE_ADDRESS;

void_t clear_screen()
{
	uint32_t index;

	for (cursor = (uint8_t *)VGA_BASE_ADDRESS, index = 0;
	     index < MAX_COLOUMNS * MAX_ROWS; cursor += 2, index++)
		*cursor = ' ';
	cursor = (uint8_t *)VGA_BASE_ADDRESS;
}

void_t print_string(uint8_t *string)
{
	uint32_t index;

	for (index = 0; string[index] != 0; index++) {
		if (string[index] == '\n') {
			uint64_t line_number;

			line_number =
				((uint64_t)cursor -
				 VGA_BASE_ADDRESS) / (MAX_COLOUMNS * 2);
			line_number++;
			cursor =
				(uint8_t *)(line_number * MAX_COLOUMNS *
					    2) + VGA_BASE_ADDRESS;
		} else {
			*cursor = string[index];
			cursor += 2;
		}
	}

	/*
	 * print log through serial port.
	 */
	loader_serial_puts(string);
}

void_t print_value(uint32_t value)
{
	uint32_t index;
	uint8_t character;

	for (index = 0; index < 8; index++) {
		character = (uint8_t)((value >> ((7 - index) * 4)) & 0x0f) + '0';
		if (character > '9') {
			character = character - '0' - 10 + 'A';
		}
		*cursor = character;
		cursor += 2;
	}

	/*
	 * print log through serial port.
	 */
	loader_serial_put_hex(value, 1);
}

void_t print_string_value(uint8_t *string, uint32_t value)
{
	print_string(string);
	print_value(value);
	print_string("\n");
}
