KERNELRELEASE ?= $(shell uname -r)
KERNELDIR ?= /lib/modules/$(KERNELRELEASE)/build
PREFIX ?= /usr
DESTDIR ?=
SRCDIR ?= $(PREFIX)/src
DEPMOD ?= depmod
obj-m += drvoscd.o

PWD := $(shell pwd)

all: module

ifneq ($(V),1)
MAKEFLAGS += --no-print-directory
endif

module:
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.PHONY: all module clean
