obj-m += ex01.o

KDIR := /lib/modules/$(shell uname -r)/build

all: module userapp

module:
	make -C $(KDIR) M=$(PWD) modules

userapp:
	gcc -Wall -o userspace_app userspace_app.c

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f userspace_app