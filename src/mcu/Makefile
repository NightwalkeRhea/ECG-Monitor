CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
SRC = spi.c i2c.c
OBJ = $(SRC:.c=.o)

.PHONY: all build clean lint
all: build

build: $(OBJ)
	$(CC) $(CFLAGS) -o mcu_demo $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

lint:
	clang-tidy $(SRC)

clean:
	rm -f $(OBJ) mcu_demo
