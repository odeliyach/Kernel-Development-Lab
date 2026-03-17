# Makefile for Message Slot Kernel Module
# Builds the kernel module and user-space programs

# Kernel module object
obj-m := message_slot.o

# Kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build

# Current working directory
PWD := $(shell pwd)

# User-space programs
USER_PROGS := message_sender message_reader

# Compiler flags for user-space programs
CFLAGS := -O3 -Wall -std=c11

.PHONY: all module user_programs clean

# Default target: build everything
all: module user_programs

# Build the kernel module
module:
	make -C $(KDIR) M=$(PWD) modules

# Build user-space programs
user_programs: $(USER_PROGS)

message_sender: message_sender.c message_slot.h
	gcc $(CFLAGS) -o message_sender message_sender.c

message_reader: message_reader.c message_slot.h
	gcc $(CFLAGS) -o message_reader message_reader.c

# Clean build artifacts
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f $(USER_PROGS)
