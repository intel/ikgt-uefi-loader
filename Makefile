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

export PROJS = $(CURDIR)/..

export CC = gcc
export AS = gcc
export LD = ld
export AR = ar

debug ?= 0
ifeq ($(debug), 1)
XMON_CMPL_OPT_FLAGS = -DDEBUG
export BINDIR = $(PROJS)/bin/linux/debug/
export OUTDIR = $(PROJS)/build/linux/debug/
export OUTPUTTYPE = debug
else
XMON_CMPL_OPT_FLAGS =
export BINDIR = $(PROJS)/bin/linux/release/
export OUTDIR = $(PROJS)/build/linux/release/
export OUTPUTTYPE = release
endif

$(shell mkdir -p $(OUTDIR))
$(shell mkdir -p $(BINDIR))

export XMON_CMPL_OPT_FLAGS

.PHONY: startap pre_os clean

all: startap pre_os

startap:
	$(MAKE) -C $(PROJS)/loader/startap

pre_os:
	$(MAKE) -C $(PROJS)/loader/pre_os

clean:
	-rm -rf $(OUTDIR)
	-rm -rf $(BINDIR)
	$(MAKE) -C $(PROJS)/loader/startap clean
	$(MAKE) -C $(PROJS)/loader/pre_os clean


# End of file

