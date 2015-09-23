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

#include "image_access_file.h"

/*--------------------------Local Types Definitions-------------------------*/
typedef struct mem_chunk_t {
	struct mem_chunk_t *next; /* used for purge only */
	long offset;
	long length;
	char buffer[1];
} mem_chunk_t;

typedef struct {
	gen_image_access_t gen;         /* inherits to gen_image_access_t */
	FILE *file;
	mem_chunk_t *memory_list;       /* should be free when image access destructed */
} file_image_access_t;

/*-------------------------Local Functions Declarations-----------------------*/
static void file_image_close(gen_image_access_t *);
static size_t file_image_read(gen_image_access_t *, void *, size_t, size_t);
static size_t file_image_map_to_mem(gen_image_access_t *, void **, size_t, size_t);

/*---------------------------------------Code---------------------------------*/

gen_image_access_t *file_image_create(char *filename)
{
	file_image_access_t *fia;
	FILE *file;

	file = fopen(filename, "rb");
	if (NULL == file) {
		return NULL;
	}

	fia = malloc(sizeof(file_image_access_t));
	if (NULL == file) {
		return NULL;
	}

	fia->gen.close = file_image_close;
	fia->gen.read = file_image_read;
	fia->gen.map_to_mem = file_image_map_to_mem;
	fia->file = file;
	fia->memory_list = NULL;
	return &fia->gen;
}

void file_image_close(gen_image_access_t *ia)
{
	file_image_access_t *fia = (file_image_access_t *)ia;
	mem_chunk_t *chunk;

	fclose(fia->file);

	while (NULL != fia->memory_list) {
		chunk = fia->memory_list;
		fia->memory_list = fia->memory_list->next;
		free(chunk);
	}
	free(fia);
}

size_t file_image_read(gen_image_access_t *ia,
		       void *dest, size_t src_offset, size_t bytes)
{
	file_image_access_t *fia = (file_image_access_t *)ia;

	if (0 != fseek(fia->file, src_offset, SEEK_SET)) {
		return 0;
	}
	return fread(dest, 1, bytes, fia->file);
}

size_t file_image_map_to_mem(gen_image_access_t *ia,
			     void **dest, size_t src_offset, size_t bytes)
{
	file_image_access_t *fia = (file_image_access_t *)ia;
	mem_chunk_t *chunk;
	size_t bytes_mapped;

	/* first search if this chunk is already allocated */
	for (chunk = fia->memory_list; chunk != NULL; chunk = chunk->next) {
		if (chunk->offset == src_offset && chunk->length == bytes) {
			/* found */
			break;
		}
	}

	if (NULL == chunk) {
		/* if not found, allocate new chunk */
		size_t bytes_to_alloc =
			sizeof(mem_chunk_t) - sizeof(chunk->buffer) + bytes;
		chunk = (mem_chunk_t *)malloc(bytes_to_alloc);
		chunk->next = fia->memory_list;
		fia->memory_list = chunk;
		chunk->offset = src_offset;
		chunk->length = bytes;
		bytes_mapped = file_image_read(ia, chunk->buffer, src_offset, bytes);
	} else {
		/* reuse old chunk */
		bytes_mapped = bytes;
	}

	*dest = chunk->buffer;

	return bytes_mapped;
}
