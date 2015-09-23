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
export PROJS = $(CURDIR)/../..

export CC = gcc
export AS = gcc
export LD = ld
endif

debug ?= 0
ifeq ($(debug), 1)
LOADER_CMPL_OPT_FLAGS = -DDEBUG
export BINDIR = $(PROJS)/bin/linux/debug/
export OUTDIR = $(PROJS)/loader/startap/build/linux/debug/
else
LOADER_CMPL_OPT_FLAGS =
export BINDIR = $(PROJS)/bin/linux/release/
export OUTDIR = $(PROJS)/loader/startap/build/linux/release/
endif

$(shell mkdir -p $(OUTDIR))
$(shell mkdir -p $(BINDIR))

export LOADER_CMPL_OPT_FLAGS

TARGET = startap.elf

ifeq ($(debug), 1)
LDFLAGS = -e startap_main -m elf_x86_64 -pie -z max-page-size=4096 -z common-page-size=4096
else
LDFLAGS = -e startap_main -m elf_x86_64 -pie -s -z max-page-size=4096 -z common-page-size=4096
endif

ASOURCES = wakeup_init64.S

CSOURCES = startap.c ap_procs_init.c
include $(PROJS)/loader/rule.linux

AFLAGS += $(INCLUDES)

.PHONY: ia32 common $(TARGET) copy clean

all: common $(TARGET) copy

common:
	$(MAKE) -C $(PROJS)/loader/common/util

$(TARGET):
	$(LD) $(LDFLAGS) -o $(OUTDIR)$@ $(wildcard $(OUTDIR)*.o)

copy:
	cp $(OUTDIR)$(TARGET) $(BINDIR)$(TARGET)

clean:
	-rm -rf $(PROJS)/loader/startap/build
