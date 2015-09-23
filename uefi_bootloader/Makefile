VERSION=17

TARGET = preload.efi

PWD=$(shell pwd)
ARCH=$(shell $(CC) -dumpmachine | sed "s/\(-\).*$$//")


ifeq ($(ARCH),x86_64)
	GNUEFI_TOP=$(PWD)/efi_prebuilts/gnu-efi/linux-x86_64/
else
	GNUEFI_TOP=
endif
INCDIR=$(GNUEFI_TOP)/include
LIBDIR=$(GNUEFI_TOP)/lib
LIBEFIDIR=$(LIBDIR)

export CC = gcc
export LD = ld

CPPFLAGS = \
	-I. \
	-I$(INCDIR)/efi \
	-I$(INCDIR)/efi/$(ARCH) \

CFLAGS = \
	-DVERSION=$(VERSION) \
	-Wall \
	-ggdb \
    -fPIC \
	-fshort-wchar \
	-nostdinc \
	-ffreestanding \
	-fno-stack-protector \
	-fno-strict-aliasing \
	-mno-red-zone \
	-maccumulate-outgoing-args \
	-mno-mmx \
	-mno-sse \
	-fno-builtin \
	-Wsign-compare \
    -Werror \

ifeq ($(ARCH),x86_64)
CFLAGS += \
	-DEFI_FUNCTION_WRAPPER
else
CFLAGS += \
	-m32
endif

LDFLAGS = -T $(LIBEFIDIR)/elf_$(ARCH)_efi.lds \
    -shared \
	-Bsymbolic \
	-nostdlib \
	-znocombreloc \
	-L $(LIBDIR) \
	$(LIBEFIDIR)/crt0-efi-$(ARCH).o \


preload.efi: preload.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
	  -j .dynsym -j .rel -j .rela -j .reloc -j .eh_frame \
	  --target=efi-app-$(ARCH) $< $@

%.o: %.c %.S Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

preload.so: preload.o
	$(LD) $(LDFLAGS) $^ -o $@ -lefi -lgnuefi \
		$(shell $(CC) -print-libgcc-file-name)

clean:
	rm -f *.o preload.so preload.efi
