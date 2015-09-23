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

debug ?= 0
ifeq ($(debug), 1)
MBUILD = debug
else
MBUILD = release
endif

export BINDIR = $(PROJS)/bin/linux/$(MBUILD)/
export OUTDIR = $(PROJS)/loader/pre_os/build/linux/$(MBUILD)/

$(shell mkdir -p $(OUTDIR))
$(shell mkdir -p $(BINDIR))



endif




TARGET = xmonpacker
PACKAGE = ikgt_pkg.bin


CSOURCES = $(wildcard *.c)


#include $(PROJS)/loader/rule.linux

INCLUDES = -I$(PROJS)/loader/pre_os/common/include \
           -I$(PROJS)/loader/pre_os/starter \
           -I$(PROJS)/loader/startap \
           -I$(PROJS)/common/include \
           -I$(PROJS)/core/common/include \
           -I$(PROJS)/core/common/include/arch


ifeq ($(debug), 1)
CFLAGS = -static -Werror -DDEBUG
else
CFLAGS = -s -static -Werror
endif

CFLAGS += $(INCLUDES)


COBJS = $(addprefix $(OUTDIR), $(notdir $(patsubst %.c, %.o, $(CSOURCES))))





OBJS = $(OUTDIR)xmon_packer.o

.PHONY: all $(COBJS) $(TARGET) pack copy  clean

all: $(COBJS) $(TARGET) pack copy



#
# 1. build packer
# 2. pack xmon_loader, startap, xmon.bin into starter.bin
#
$(COBJS): $(CSOURCES)
	$(CC) -c $(CFLAGS) -o $@ $(filter $(*F).c, $(CSOURCES))


$(TARGET):
	$(CC) $(CFLAGS) -o $(OUTDIR)$@ $(OBJS)



pack:$(TARGET)
	chmod +x $(OUTDIR)$(TARGET) && \
	cp $(BINDIR)xmon.elf $(OUTDIR)xmon.bin && \
	cp $(BINDIR)startap.elf $(OUTDIR)startap.bin && \
	cd $(OUTDIR) && \
	./$(TARGET) --xmon  $(OUTDIR)xmon.bin

copy:pack
	cp $(OUTDIR)$(PACKAGE) $(BINDIR)

clean:
	rm -f $(OBJS)
	rm -f $(OUTDIR)$(TARGET)
