# Makefile for Message Slot Kernel Module and User Applications
# Builds the kernel module and user-space programs with a single command

# Kernel module object
obj-m := message_slot.o

# Kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build

# Current working directory
PWD := $(shell pwd)

# Source directories
SRC_DIR := src
INCLUDE_DIR := include
USER_APPS_DIR := user_apps

# User-space programs
USER_PROGS := message_sender message_reader

# Compiler flags for user-space programs
CFLAGS := -O3 -Wall -std=c11 -I$(INCLUDE_DIR)

.PHONY: all module user_programs clean help

# Default target: build everything
all: module user_programs

# Build the kernel module
module:
	@echo "Building kernel module..."
	make -C $(KDIR) M=$(PWD)/$(SRC_DIR) modules
	@cp $(SRC_DIR)/message_slot.ko .
	@echo "Kernel module built successfully: message_slot.ko"

# Build user-space programs
user_programs: $(addprefix $(USER_APPS_DIR)/, $(USER_PROGS))
	@echo "User applications built successfully"

$(USER_APPS_DIR)/message_sender: $(USER_APPS_DIR)/message_sender.c $(INCLUDE_DIR)/message_slot.h
	@echo "Building message_sender..."
	gcc $(CFLAGS) -o $(USER_APPS_DIR)/message_sender $(USER_APPS_DIR)/message_sender.c

$(USER_APPS_DIR)/message_reader: $(USER_APPS_DIR)/message_reader.c $(INCLUDE_DIR)/message_slot.h
	@echo "Building message_reader..."
	gcc $(CFLAGS) -o $(USER_APPS_DIR)/message_reader $(USER_APPS_DIR)/message_reader.c

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	make -C $(KDIR) M=$(PWD)/$(SRC_DIR) clean
	rm -f $(addprefix $(USER_APPS_DIR)/, $(USER_PROGS))
	rm -f message_slot.ko
	@echo "Clean complete"

# Help target
help:
	@echo "Message Slot Build System"
	@echo "========================="
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build kernel module and user applications (default)"
	@echo "  module         - Build only the kernel module"
	@echo "  user_programs  - Build only the user-space programs"
	@echo "  clean          - Remove all build artifacts"
	@echo "  help           - Display this help message"
	@echo ""
	@echo "Directory Structure:"
	@echo "  src/           - Kernel module source code"
	@echo "  include/       - Header files"
	@echo "  user_apps/     - User-space applications"
	@echo ""
	@echo "Build Products:"
	@echo "  message_slot.ko              - Kernel module (root directory)"
	@echo "  user_apps/message_sender     - Message sender program"
	@echo "  user_apps/message_reader     - Message reader program"
