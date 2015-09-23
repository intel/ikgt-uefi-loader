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
void *mon_memset(void *dest, uint8_t val, uint64_t count)
{
	__asm__ __volatile__ (
		"mov %0, %%rdi\n"
		"movb %1, %%al\n"
		"mov %2, %%rcx\n"
		"cld; rep; stosb"
		:
		: "g" (dest), "g" (val), "g" (count)
		: "rdi", "rax", "rcx", "memory"
		);

	return dest;
}

void *mon_memcpy(void *dest, const void *src, uint64_t count)
{
	__asm__ __volatile__ (
		"mov %0, %%rdi\n"
		"mov %1, %%rsi\n"
		"mov %2, %%rcx\n"
		"cld; rep; movsb"
		:
		: "g" (dest), "g" (src), "g" (count)
		: "rdi", "rsi", "rcx", "memory"
		);

	return dest;
}

int mon_strlen(const char *string)
{
	unsigned int len = 0;
	const char *next = string;

	if (!string) {
		return -1;
	}

	for (; *next != 0; ++next)
		++len;

	return len;
}
