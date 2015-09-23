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
LOADER_CMPL_OPT_FLAGS =
export BINDIR = $(PROJS)/bin/linux/release/
export OUTDIR = $(PROJS)/loader/pre_os/build/linux/release/
endif

$(shell mkdir -p $(OUTDIR))
$(shell mkdir -p $(BINDIR))

export LOADER_CMPL_OPT_FLAGS

endif
	
SUBDIRS = elf_ld image_accessors

.PHONY: all

all:
	$(foreach D, $(SUBDIRS), $(MAKE) -C $(D);)
