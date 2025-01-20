obj-m += src/kernel_module.o

kmodule:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

api:
	gcc -Wall -o src/api src/api.c

all: kmodule api

test:
	sudo dmesg -C
	sudo insmod src/kernel_module.ko
	sudo rmmod src/kernel_module.ko
	sudo dmesg

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f src/api