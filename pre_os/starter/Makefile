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

CSOURCES = starter_main.c \
           run_xmon_loader.c

ASOURCES = starter.S
AINCLUDES += -I$(PROJS)/loader/pre_os/common/include

include $(PROJS)/loader/rule.linux

INCLUDES += -I$(PROJS)/loader/pre_os/xmon_loader/utils/screen \
            -I$(PROJS)/loader/pre_os/common/include \
            -I$(PROJS)/loader/pre_os/common/loader_serial

ifeq ($(debug), 1)
LDFLAGS = -e start -m elf_x86_64 -pie -z max-page-size=4096 -z common-page-size=4096
else
LDFLAGS = -e start -m elf_x86_64 -pie -s -z max-page-size=4096 -z common-page-size=4096
endif

OBJS = $(OUTDIR)starter.o $(OUTDIR)starter_main.o \
       $(OUTDIR)run_xmon_loader.o $(OUTDIR)elf_ld.o \
       $(OUTDIR)elf32_ld.o $(OUTDIR)elf64_ld.o \
       $(OUTDIR)elf_info.o $(OUTDIR)image_access_mem.o \
       $(OUTDIR)memory.o $(OUTDIR)screen.o \
       $(OUTDIR)common.o \
       $(OUTDIR)loader_serial.o

TARGETS = ld utils common preos_common starter.elf copy

.PHONY: $(TARGETS) clean

all: $(TARGETS)

ld:
	$(MAKE) -C $(PROJS)/loader/common/ld

utils:
	$(MAKE) -C $(PROJS)/loader/pre_os/xmon_loader/utils

common:
	$(MAKE) -C $(PROJS)/loader/common/util

preos_common:
	$(MAKE) -C $(PROJS)/loader/pre_os/common

starter.elf:
	$(LD) $(LDFLAGS) -o $(OUTDIR)$@ $(OBJS)

copy:
	objcopy -j .text -O binary -S $(OUTDIR)starter.elf $(OUTDIR)starter.bin

clean:
	rm -f $(OBJS) $(OUTDIR)pe_ld.o
	rm -f $(OUTDIR)starter.*
