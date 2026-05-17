obj-m += ex05.o

# Adjust KDIR to match your kernel source / headers path.
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean