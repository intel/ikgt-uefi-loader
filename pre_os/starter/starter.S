################################################################################
# Copyright (c) 2015 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

.file   "starter.s"

#define ASM_FILE  1
#include "xmon_desc.h"
#include "ikgtboot.h"

.text

.extern starter_main

#---------------------------------------------------------------------
#  void_t start(void)
# This is the first loader componet to run after EFI bootloader and
# tranfer control to the xmon loader.
# It performs below operations:
#   1. save CPU states - pushal - eax, ecx, edx, ebx, esp, ebp, esi, edi
#                        pushfl - flags
#   2. call starter efi-loader function to load xmon_loader binary
#   3. jump to xmon_loader entry point
#---------------------------------------------------------------------

.globl start, _start
.org 0
start:
_start:


.align 8
/****** ikgt private boot header ******/
/* must match the header definition in ikgtboot.h */
ikgt_boot_header:
	.quad  IKGT_BOOT_HEADER_MAGIC
	/* header struct size */
	.long  ikgt_boot_header_end - ikgt_boot_header
	/* reserved */
	.long  0
	/* 32 bit entry offset, not supported */
	.long  0xffffffff
	/* 64 bit entry offset*/
	.long  start_x64 - _start
	/* for EFI memory map, memmap_addr */
	.long  0
	/* for EFI memory map, memmap_size */
	.long  0
	/* runtime_mem_base */
	.long  0
	/* runtime_mem_size */
	.long  0
	/* loader_mem_base */
	.long  0
	/* loader_mem_size */
	.long  0
	/* image size after 0-padding to 4k aligned */
	.long  0
ikgt_boot_header_end:

/* code executed from here */
/*
 * 64bit entry point, used by ikgt boot header
 *  Scratch Registers   - rdi, rsi, rdx, rcx, r8, r9, r10, r11
*/

.code64
start_x64:
     cli
     mov %rdi, %rdi   /*rdi saved the setup header*/
     mov $(IKGT_BOOTLOADER_MAGIC), %rsi  /*rsi saved the bootloader magic*/
     mov %rsp, %rdx
     mov %rbp, %rcx
     pushf
     pop %r8
     call starter_main
     sti
     ret
