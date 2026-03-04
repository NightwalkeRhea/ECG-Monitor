# ============================================================================
# MCU Firmware Makefile (XMC1100 Target)
# ============================================================================
# Build MCU firmware with SPI, I2C, GPIO, and clock management

# Compiler and flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c99
LDFLAGS ?=

# Source files
SOURCES := main.c common.c clock_systick.c gpio.c usic.c
HEADERS := common.h clock_systick.h gpio.h usic.h mcu_definition.h system_XMC1100.h

# Object files
OBJECTS := $(SOURCES:.c=.o)

# Output executable
TARGET := firmware.elf

# Phony targets (not actual files)
.PHONY: all build clean lint help

# Default target
all: build

# Build the firmware
build: $(OBJECTS)
	@echo "Linking firmware..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Lint C code with clang-tidy (requires clang-tools-extra)
lint:
	@echo "Linting MCU C code..."
	@command -v clang-tidy >/dev/null 2>&1 || (echo "clang-tidy not found. Install clang-tools." && exit 1)
	clang-tidy $(SOURCES) -- $(CFLAGS)

# Clean build artifacts
clean:
	@echo "Cleaning MCU build artifacts..."
	del /q $(OBJECTS) $(TARGET) 2>nul || echo "No artifacts to clean"

help:
	@echo "MCU Firmware Makefile (XMC1100)"
	@echo ""
	@echo "Targets:"
	@echo "  make build        - Build MCU firmware (default)"
	@echo "  make all          - Same as build"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make lint         - Lint C code with clang-tidy"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CC                - C compiler (default: gcc)"
	@echo "  CFLAGS            - Compiler flags (default: -Wall -Wextra -O2 -std=c99)"
	@echo "  LDFLAGS           - Linker flags (default: empty)"
	@echo ""
	@echo "Example:"
	@echo "  make CC=arm-none-eabi-gcc CFLAGS='-mcpu=cortex-m0 -Wall -O2'"
