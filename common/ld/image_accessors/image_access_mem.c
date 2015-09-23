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

#include "image_access_mem.h"

void *mon_memcpy(void *dest, const void *src, size_t count);
/*---------------------------------------Code---------------------------------*/

void mem_image_close(gen_image_access_t *ia)
{
	mem_image_access_t *mia = (mem_image_access_t *)ia;

	FREE(mia);
}

size_t mem_image_read(gen_image_access_t *ia,
		      void *dest, size_t src_offset, size_t bytes_to_read)
{
	mem_image_access_t *mia = (mem_image_access_t *)ia;

	if ((src_offset + bytes_to_read) > mia->size) {
		bytes_to_read = mia->size - src_offset; /* read no more than size */
	}

	mon_memcpy(dest, mia->image + src_offset, bytes_to_read);
	return bytes_to_read;
}

size_t mem_image_map_to_mem(gen_image_access_t *ia,
			    void **dest,
			    size_t src_offset, size_t bytes_to_read)
{
	mem_image_access_t *mia = (mem_image_access_t *)ia;

	if ((src_offset + bytes_to_read) > mia->size) {
		bytes_to_read = mia->size - src_offset; /* read no more than size */
	}

	*dest = mia->image + src_offset;

	return bytes_to_read;
}

gen_image_access_t *mem_image_create_ex(char *image, size_t size, void *buf)
{
	mem_image_access_t *mia = (mem_image_access_t *)buf;

	mia->gen.close = mem_image_close;
	mia->gen.read = mem_image_read;
	mia->gen.map_to_mem = mem_image_map_to_mem;
	mia->image = image;
	mia->size = size;
	return &mia->gen;
}
