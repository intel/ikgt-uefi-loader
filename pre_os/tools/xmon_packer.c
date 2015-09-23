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
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#define int64_t _int64_t
#define size_t _size_t
#include "xmon_desc.h"
#include "ikgtboot.h"

typedef int bool;

#define true 1
#define false 0

#ifndef ALIGN_4K
#define ALIGN_4K(x)    ALIGN_FORWARD(x, 0x1000)
#endif
#ifndef ALIGN_FORWARD
#define ALIGN_FORWARD(addr, bytes)   ALIGN_BACKWARD(addr + bytes - 1, bytes)
#endif
#ifndef ALIGN_BACKWARD
#define ALIGN_BACKWARD(addr, bytes) ((UINT64)(addr) & ~((bytes) - 1))
#endif

#define STARTER_FILE_OPTION "--starter"
#define XMON_LOADER_FILE_OPTION   "--xmon_loader"
#define STARTAP_FILE_OPTION "--startap"
#define XMON_FILE_OPTION   "--xmon"


/* could change to ikgt_pkg.bin if needed */
#define XMON_PKG_BIN_NAME    "ikgt_pkg.bin"

typedef struct {
	/* if true, but file doesn't exist, then report errors
	 *  otherwise, ignore it and continue.
	 *  (used for optional secondary guest imgs)
	 */
	const bool must_exist;

	/* cmdline option */
	const char *option_name;

	/* file name */
	char *file_name;

	/* corresponding the file offset mapping header flag */
	const unsigned int flag;


	unsigned int offset;


	/* fsize is ZERO, indicates no this file, so
	 *  do not set corresponding flag
	 */
	unsigned int fsize;
} FILE_OPTIONS;


/* default settings if no cmdline inputs
 *  to add new modules, just append them at the end of
 *  this array.
 */
static FILE_OPTIONS files_options[] = {
	/* assumption 1: keeping this option at first one can make thing much easier
	 *  the code will check this assumption.
	 */
	{ true, STARTER_FILE_OPTION,	 "starter.bin",	     0,
	  0, 0 },


	/* assumption 2: keep this order as indicated in enum FILE_PACK_INDEX (0,1,2,3...)
	 *  the code will check this assumption.
	 */
	{ true, XMON_LOADER_FILE_OPTION, "xmon_loader.bin",  INDEX_TO_BITMAP_FLAG(
		  XMON_LOADER_BIN_INDEX), 0, 0 },
	{ true, STARTAP_FILE_OPTION,	 "startap.bin",	     INDEX_TO_BITMAP_FLAG(
		  STARTAP_BIN_INDEX),	  0, 0 },
	{ true, XMON_FILE_OPTION,	 "xmon.bin",	     INDEX_TO_BITMAP_FLAG(
		  XMON_BIN_INDEX),	  0, 0 },
	/* and others
	 * (to support secondary guests' img/bins, set field "must_exist" as false)
	 */
};

/* the length of array above files_options */
#define PACK_FILE_COUNT  (sizeof(files_options) / sizeof(files_options[0]))



static void *read_file_to_buf(const char *fname, unsigned int buf_size);
static int append_buf_to_file(FILE *f, const void *pbuf, size_t buf_len);
static int write_buf_to_file(FILE *f,
			     long f_offset,
			     const void *pinbuf,
			     size_t inbuf_len);


/*
 * to caller, if return value is -1 or 0, the report error.
 */
static unsigned long get_file_size(char *file)
{
	struct stat st = { 0 };

	if (stat(file, &st) == 0) {
		return st.st_size;
	} else {
		return -1;
	}
}

static void cmdline_help(FILE_OPTIONS *file_array)
{
	int idx = 0;

	printf("\r\nUsage:  [<OPTION>  <FILE>] ... \r\n");

	printf("options:\r\n");
	for (idx = 0; idx < PACK_FILE_COUNT; idx++)
		printf("  %s\t  specify the corresponding file name\r\n",
			file_array[idx].option_name);

	printf("\r\nUse default file name(s), if no such option(s).\r\n\r\n");

	return;
}

/*
 * if no cmdline, or some option missing, then use the corresponding default values.
 */
static int parse_packer_cmdline(int argc, char *argv[], FILE_OPTIONS *file_array)
{
	int cmd_idx, file_idx;

	for (cmd_idx = 1; cmd_idx < argc; ) {
		/* firstly searching if the option is valid */
		for (file_idx = 0; file_idx < PACK_FILE_COUNT; file_idx++) {
			if (0 ==
			    strcmp(argv[cmd_idx],
				    file_array[file_idx].option_name)) {
				break;
			}
		}

		/* cannot find option matched ?*/
		if (file_idx == PACK_FILE_COUNT) {
			printf(
				"\r\n!ERROR(packer): unrecognized option \"%s\"\r\n\r\n",
				argv[cmd_idx]);
			return -1;
		}


		/* then move to next to get the file name(path) */
		cmd_idx++;

		/* must not be with prefix "--" */
		if (0 == strncmp(argv[cmd_idx], "--", 2)) {
			printf(
				"\r\n!ERROR(packer): unrecognized option \"%s\"\r\n\r\n",
				argv[cmd_idx]);
			return -1;
		}

		/* update filename(default) info for later use */
		file_array[file_idx].file_name = argv[cmd_idx];

		cmd_idx++;
	}

	return 0;
}


/*
 *  update offset and fsize info in files_options[] array.
 */
static int update_file_info(FILE_OPTIONS *file_array)
{
	int file_idx;

	for (file_idx = 0; file_idx < PACK_FILE_COUNT; file_idx++) {
		char *fname = file_array[file_idx].file_name;

		/* check if file_name is valid if it must be required. */
		if (file_array[file_idx].must_exist && (fname == NULL)) {
			printf(
				"\r\n!ERROR(packer): option \"%s\" is missing, but required\r\n\r\n",
				file_array[file_idx].option_name);
			return -1;
		}

		/* if file name is NULL, skip it then */
		if (fname) {
			unsigned long fsize = get_file_size(fname);

			if (fsize == 0 || fsize == -1) {
				if (file_array[file_idx].must_exist) {
					printf("\r\n!ERROR(packer): file size is 0, or the file \"%s\" is missing\r\n\r\n",
						fname);
					return -1;
				} else {
					fsize = 0;
				}
			}

			/* update file size */
			file_array[file_idx].fsize = fsize;
		} else {
			file_array[file_idx].fsize = 0;
		}

		/* update file offset, skip the first one (index 0, STARTER_FILE_OPTION) */
		if (file_idx) {
			/* offset value = last file offset + last file size (could be zero) */
			file_array[file_idx].offset =
				file_array[file_idx - 1].offset +
				file_array[file_idx - 1].fsize;
		}
	}


	/* check again to see if some file missing
	 *  only when such a file must exist, but its size is zero
	 */
	for (file_idx = 0; file_idx < PACK_FILE_COUNT; file_idx++) {
		if (file_array[file_idx].must_exist &&
		    (file_array[file_idx].fsize == 0)) {
			printf(
				"\r\n!ERROR(packer): option \"%s\" is missing\r\n\r\n",
				file_array[file_idx].option_name);
			return -1;
		}
	}



	return 0;
}


static unsigned int get_total_file_size(FILE_OPTIONS *file_array)
{
	unsigned int    file_idx, total_file_size = 0;
	for (file_idx = 0; file_idx < PACK_FILE_COUNT; file_idx++) {
		total_file_size += file_array[file_idx].fsize;
	}
	return total_file_size;
}


/* check total size required...against the hardcode defined in xmon_desc.h file */
static int total_file_size_check(FILE_OPTIONS *file_array)
{
	unsigned int file_idx, total_file_size = 0;

	total_file_size = get_total_file_size(file_array);

	if (total_file_size > LOADER_BIN_SIZE) {
		printf(
			"\r\n!ERROR(packer): Required size 0x%x, actual allocated size 0x%x. Please update macro(LOADER_BIN_SIZE,xmon_desc.h)\r\n\r\n",
			total_file_size,
			LOADER_BIN_SIZE);
		return -1;
	}

	return 0;
}

/*
 * pack all the files into a new pkg binary file (now named xmon_pkg.bin)
 */
static int pack_files(FILE_OPTIONS *file_array)
{
	FILE *newfile = NULL;
	int ret = -1;
	int file_idx;
	void *file_buf = NULL;

	/* remove this file if exists*/
	if (remove(XMON_PKG_BIN_NAME)) {
		/* ignore it if not exist */
		if (errno != ENOENT) {
			printf(
				"\r\n!ERROR(packer): failed to remove the old file: %s (err - %d) \r\n\r\n",
				XMON_PKG_BIN_NAME,
				errno);
			goto exit;
		}
	}

	/* open it with "a" for appending, create one if not exists */
	newfile = fopen(XMON_PKG_BIN_NAME, "a");
	if (newfile == NULL) {
		return -1;
	}


	for (file_idx = 0; file_idx < PACK_FILE_COUNT; file_idx++) {
		char *fname = file_array[file_idx].file_name;
		unsigned int fsize = file_array[file_idx].fsize;

		/* if file size is zero, skip it, no need to pack it */
		if (fsize) {
			/* read file to tmp buffer */
			file_buf = read_file_to_buf(fname, fsize);
			if (!file_buf) {
				printf(
					"\r\n!ERROR(packer): failed to read file contents: %s\r\n\r\n",
					fname);
				goto exit;
			}

			/* append this buf content to the new file */
			if (0 != append_buf_to_file(newfile, file_buf, fsize)) {
				goto exit;
			}
			free(file_buf); file_buf = NULL;
		}
	}
	/* success */
	ret = 0;

exit:

	if (newfile) {
		fclose(newfile);
	}
	if (file_buf) {
		free(file_buf);
	}

	return ret;
}



static int  get_file_hdr_info(FILE_OPTIONS *file_array,
			      xmon_loaderbin_file_mapping_header_t *file_hdr)
{
	int file_idx;

	/* clear the flags */
	file_hdr->flags = 0;

	/* check: make sure enum FILE_PACK_INDEX is sync'd with files_options array */
	if ((PACK_FILE_COUNT - 1) != MAX_BIN_COUNT) {
		printf(
			"\r\n!ERROR(packer): make sure enum FILE_PACK_INDEX is sync'd with files_options[] array\r\n\r\n");
		return -1;
	}

	for (file_idx = 0; file_idx < PACK_FILE_COUNT; file_idx++) {
		/* only update flags for existing files */
		if (file_array[file_idx].fsize) {
			file_hdr->flags |= file_array[file_idx].flag;
		}

		/* update image offset and size
		 *  skip the first one because it is starter.bin (no need offset/size)
		 */
		if (file_idx) {
			file_hdr->files[file_idx -
					1].offset = file_array[file_idx].offset;
			file_hdr->files[file_idx -
					1].size = file_array[file_idx].fsize;
		}
	}

	return 0;
}


static int get_file_header_location(FILE_OPTIONS *file_array,
				    unsigned int *hdr_offset)
{
	FILE *starter_file = NULL;
	unsigned int *starter_buf = NULL, *tmp;
	int ret = -1;

	/* we can safely assume the first one is starter.bin due to checks before */
	starter_buf = read_file_to_buf(file_array[0].file_name, file_array[0].fsize);
	if (!starter_buf) {
		printf("!ERROR(packer): failed to get file content (%s)\r\n",
			file_array[0].file_name);
		goto exit;
	}

	for (tmp = starter_buf;
	     (unsigned long)tmp <
	     ((unsigned long)starter_buf + file_array[0].fsize - 4);
	     tmp++) {
		xmon_loaderbin_file_mapping_header_t *starter_bin_file_hdr;

		/* 4 byte aligned searching */
		starter_bin_file_hdr = (xmon_loaderbin_file_mapping_header_t *)tmp;

		if ((starter_bin_file_hdr->magic0 ==
		     XMON_LOADERBIN_FILE_MAPPING_HEADER_MAGIC0) &&
		    (starter_bin_file_hdr->magic1 ==
		     XMON_LOADERBIN_FILE_MAPPING_HEADER_MAGIC1)) {
			/* get the offset to the beginning of file */
			*hdr_offset = (unsigned long)tmp -
				      (unsigned long)starter_buf;

			if (starter_bin_file_hdr->flags != 0) {
				printf(
					"\r\n!ERROR(packer): why the flags are NOT zero set before by starter\r\n\r\n");
				goto exit;
			}

			break;
		}
	}

	ret = 0;

exit:
	if (starter_buf) {
		free(starter_buf);
	}

	return ret;
}

static int update_boot_header(FILE_OPTIONS *file_array)
{
	FILE		   *file = NULL;
	unsigned int   *buf  = NULL, *tmp = NULL, found = 0;
	int			   ret = -1, padding[4096] = {0};
	unsigned int   hdr_offset = 0;
	const char	   *file_name = XMON_PKG_BIN_NAME;
	unsigned int   fsize = get_total_file_size(file_array);

	ikgt_loader_boot_header_t *boot_hdr;

	buf = read_file_to_buf(file_name, fsize);
	if (!buf) {
		printf("!ERROR(packer): failed to get file content (%s)\r\n", file_name);
		goto exit;
	}

	for (tmp = buf;
		(unsigned long)tmp < ((unsigned long)buf + fsize - 4);
		tmp++){


		/* 4 byte aligned searching */
		boot_hdr = (ikgt_loader_boot_header_t *) tmp;

		if (boot_hdr->magic == IKGT_BOOT_HEADER_MAGIC) {
			/* get the offset to the beginning of file */
			hdr_offset = (unsigned long)tmp - (unsigned long)buf;
			found = 1;
			break;
		}
	}
	if (!found)
		goto exit;

	boot_hdr->rt_mem_size = sizeof(xmon_runtime_memory_layout_t);
	boot_hdr->rt_mem_base = RT_MEM_BASE;
	boot_hdr->ldr_mem_size = sizeof(xmon_loader_memory_layout_t);
	boot_hdr->ldr_mem_base = LDR_MEM_BASE;
	boot_hdr->version = BOOT_HDR_VERSION;
	/* image is 4K aligned after padding later */
	boot_hdr->image_size = ALIGN_4K(fsize);

	file = fopen (XMON_PKG_BIN_NAME, "r+");
	if (file == NULL)
		goto exit;
	if (0 != write_buf_to_file(file,
						hdr_offset,
						boot_hdr,
						sizeof(ikgt_loader_boot_header_t))) {
		goto exit;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		printf("!ERROR(packer): failed to fseek file position to end");
		goto exit;
	}

	/* padding 0 to 4k align */
	fwrite(padding, 1, (ALIGN_4K(fsize) - fsize), file);
	fflush(file);

	if (ferror(file) != 0 ||
		feof(file) != 0) {
		printf("!ERROR(packer): failed to append %ld bytes\r\n",
					(unsigned long)(ALIGN_4K(fsize) - fsize));
		goto exit;
	}

	ret = 0;

exit:
	if (buf)
		free(buf);
	if (file)
		fclose(file);
	return ret;
}


/* update file header information in the new created file */
static int update_file_header(FILE_OPTIONS *file_array)
{
	unsigned int hdr_offset = -1;
	xmon_loaderbin_file_mapping_header_t file_hdr = {
		XMON_LOADERBIN_FILE_MAPPING_HEADER_MAGIC0,
		XMON_LOADERBIN_FILE_MAPPING_HEADER_MAGIC1,
		0, 0 };
	int ret = -1;
	FILE *newfile = NULL;


	/* fill up header info from files_options[] */
	if (0 != get_file_hdr_info(file_array, &file_hdr)) {
		goto exit;
	}

	/* search file magic header in starter.bin and get the header location */
	if ((0 != get_file_header_location(file_array, &hdr_offset)) ||
	    (hdr_offset == -1)) {
		printf(
			"\r\n!ERROR(packer): failed to get file offset mapping magic header in file (%s)\r\n\r\n",
			file_array[0].file_name);
		goto exit;
	}

	/* update file header info in new created file */
	newfile = fopen(XMON_PKG_BIN_NAME, "r+");
	if (newfile == NULL) {
		goto exit;
	}

	if (0 != write_buf_to_file(newfile,
		    hdr_offset,
		    &file_hdr,
		    sizeof(xmon_loaderbin_file_mapping_header_t))) {
		goto exit;;
	}

	ret = 0;

exit:
	if (newfile) {
		fclose(newfile);
	}

	return ret;
}


/*
 * see the assumptions when defining files_options[] array.
 */
static int check_file_array_assumptions(FILE_OPTIONS *file_array)
{
	int file_idx;

	for (file_idx = 0; file_idx < PACK_FILE_COUNT; file_idx++) {
		/*
		 *  check the assumption 1: STARTER_FILE_OPTION must be the first one!!
		 */
		if ((file_idx == 0) &&
		    strcmp(file_array[file_idx].option_name, STARTER_FILE_OPTION)) {
			printf(
				"\r\n!ERROR(packer): make sure the first one is \"%s\" in files_options[] array\r\n\r\n",
				STARTER_FILE_OPTION);
			return -1;
		}

		/*
		 * check the assumption 2:
		 * the file index order must be as indicated in enum FILE_PACK_INDEX
		 */
		if (file_idx) {
			if (INDEX_TO_BITMAP_FLAG(file_idx - 1) !=
			    file_array[file_idx].flag) {
				printf(
					"\r\n!ERROR(packer): make sure the file order must be as indicated in enum FILE_PACK_INDEX in files_options[] array\r\n\r\n");
				return -1;
			}
		}
	}

	return 0;
}



int main(int argc, char *argv[])
{
	int ret = 0;
	int idx = 0;

	FILE_OPTIONS *file_array = files_options;


	/* check assumptions for files_options[] before moving forward*/
	ret = check_file_array_assumptions(file_array);
	if (ret == -1) {
		goto error;
	}

	/* parse cmdline and fill up files_options[] */
	ret = parse_packer_cmdline(argc, argv, file_array);
	if (ret == -1) {
		goto error;
	}


	/* update file size and offset info in files_options[] */
	ret = update_file_info(file_array);
	if (ret == -1) {
		goto error;
	}


	/* sanity check for file size */
	ret = total_file_size_check(file_array);
	if (ret == -1) {
		goto error;
	}


	/* pack/append all the files into a new binary file.*/
	ret = pack_files(file_array);
	if (ret == -1) {
		goto error;
	}

	/* update file header info */
	ret = update_file_header(file_array);
	if (ret == -1) {
		goto error;
	}

	ret = update_boot_header(file_array);
	if (ret == -1) {
		goto error;
    }

	printf("\r\n!INFO(packer): Successfully pack below binaries into %s:\r\n",
		XMON_PKG_BIN_NAME);
	for (idx = 0; idx < PACK_FILE_COUNT; idx++) {
		if (file_array[idx].fsize) {
			printf("\t %20s %16d bytes\n",
				basename(file_array[idx].file_name), file_array[idx].fsize);
		}
	}

	return 0;


error:
	cmdline_help(file_array);
	return ret;
}


/*
 * append buf (with length) to file (starting from offset)
 */
static int append_buf_to_file(FILE *f, const void *input_buf, size_t input_buf_len)
{
	fwrite(input_buf, input_buf_len, 1, f);
	if (ferror(f) != 0 ||
	    feof(f) != 0) {
		printf("!ERROR(packer): failed to append %ld bytes\r\n",
			(unsigned long)input_buf_len);
		return -1;
	}

	fflush(f);

	return 0;
}



/*
 * write buf (with length) to file (starting from f_offset)
 */
static int write_buf_to_file(FILE *f,
			     long f_offset,
			     const void *pinbuf,
			     size_t inbuf_len)
{
	if (fseek(f, f_offset, SEEK_SET) != 0) {
		printf(
			"!ERROR(packer): failed to fseek file position to offset %ld\r\n",
			f_offset);
		return -1;
	}

	fwrite(pinbuf, inbuf_len, 1, f);
	if (ferror(f) != 0 ||
	    feof(f) != 0) {
		printf("!ERROR(packer): failed to write %ld bytes\r\n",
			(unsigned long)inbuf_len);
		return -1;
	}
	rewind(f);

	fflush(f);

	return 0;
}


/*
 * read file content to a buffer, the caller is responsible
 * for freeing buffer.
 */
static void *read_file_to_buf(const char *fname, unsigned int buf_size)
{
	FILE *f = fopen(fname, "r");
	void *pbuf = NULL;

	if (!f) {
		printf("!ERROR(packer): failed to open %s\r\n", fname);
		goto error;
	}

	pbuf = malloc(buf_size);
	if (!pbuf) {
		printf("!ERROR(packer): failed to allocate memory\r\n");
		goto error;
	}

	fread(pbuf, buf_size, 1, f);
	if (ferror(f) != 0 ||
	    feof(f) != 0) {
		printf("!ERROR(packer): failed to read %d bytes in file %s\r\n",
			buf_size,
			fname);

		free(pbuf);
		pbuf = NULL;

		goto error;
	}

error:
	if (f) {
		fclose(f);
	}

	return pbuf;
}



/* End of file */
