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
ifndef PROJS
export PROJS = $(CURDIR)/../../..

export CC = gcc
export AS = gcc
export LD = ld

debug ?= 0
ifeq ($(debug), 1)
LOADER_CMPL_OPT_FLAGS = -DDEBUG
export BINDIR = $(PROJS)/bin/linux/debug/
export OUTDIR = $(PROJS)/loader/pre_os/build/linux/debug/
else
export BINDIR = $(PROJS)/bin/linux/release/
export OUTDIR = $(PROJS)/loader/pre_os/build/linux/release/
endif


$(shell mkdir -p $(OUTDIR))
$(shell mkdir -p $(BINDIR))
export LOADER_CMPL_OPT_FLAGS
endif

CSOURCES = $(wildcard *.c)
ASOURCES = $(wildcard *.S)
include $(PROJS)/loader/rule.linux

INCLUDES += -I$(PROJS)/loader/pre_os/common/include \
            -I$(PROJS)/loader/pre_os/common/multiboot \
            -I$(PROJS)/loader/common/ld/mb_ld \
            -I$(PROJS)/loader/pre_os/common/loader_serial \
            -Iutils/memory \
            -Iutils/screen \
            -Iutils/string \
            -Iutils/x64

OBJS = $(OUTDIR)xmon_loader.o \
       $(OUTDIR)e820.o \
       $(OUTDIR)idt.o \
       $(OUTDIR)screen.o \
       $(OUTDIR)memory.o \
       $(OUTDIR)elf_info.o \
       $(OUTDIR)elf32_ld.o \
       $(OUTDIR)elf64_ld.o \
       $(OUTDIR)elf_ld.o \
       $(OUTDIR)image_access_mem.o \
       $(OUTDIR)common.o \
       $(OUTDIR)primary_guest.o \
       $(OUTDIR)boot_protocol_util.o \
       $(OUTDIR)loader_serial.o \
       $(OUTDIR)string.o \
       $(OUTDIR)cmdline.o \
       $(OUTDIR)ctype.o

TARGET = xmon_loader.bin

ifeq ($(debug), 1)
LDFLAGS = -e xmon_loader -m elf_x86_64 -pie -z max-page-size=4096 -z common-page-size=4096
else
LDFLAGS = -e xmon_loader -m elf_x86_64 -pie -s -z max-page-size=4096 -z common-page-size=4096
endif

.PHONY: all ia32 ld util common preos_common $(TARGET) clean

all: ld util common preos_common $(TARGET)

ia32:
	$(MAKE) -C $(PROJS)/core/common/libc/ia32

ld:
	$(MAKE) -C $(PROJS)/loader/common/ld

util:
	$(MAKE) -C $(PROJS)/loader/pre_os/xmon_loader/utils

common:
	$(MAKE) -C $(PROJS)/loader/common/util

preos_common:
	$(MAKE) -C $(PROJS)/loader/pre_os/common

$(TARGET):
	$(LD) $(LDFLAGS) -o $(OUTDIR)$@ $(OBJS)

clean:
	rm -f $(OBJS)
	rm -f $(OUTDIR)xmon_loader.*
