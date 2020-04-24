MOD_DIR  := $(shell pwd)
MOD_NAME := drvoscd.ko
SUDO     := sudo
obj-m += drvoscd.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=${MOD_DIR} modules

insert: all remove
	${SUDO} insmod ${MOD_NAME}

remove:
	@-${SUDO} rmmod ${MOD_NAME}

clean:
	make -C /lib/modules/$(shell uname -r)/build M=${MOD_DIR} clean

