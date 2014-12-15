KVERS = $(shell uname -r)
# Kernel modules
obj-m += inf.o
# Specify flags for the module compilation.
#EXTRA_CFLAGS=-g -O0
build: kernel_modules user_client
kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
user_client:
	gcc -o client client.c
clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
