CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c11

SRC = src/main.c
OBJ = $(SRC:src/%.c=build/%.o)
TARGET = bin/mycat-lang

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build bin

.PHONY: all clean
