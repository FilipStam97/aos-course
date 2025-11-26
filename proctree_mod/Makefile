# Name of the module
obj-m := proctree_mod.o

# Path to the kernel build directory for the *current* running kernel
KDIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD  := $(shell pwd)

# Default target: build the module
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
