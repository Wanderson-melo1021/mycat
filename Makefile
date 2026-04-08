CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c11 -Iinclude

SRC = src/main_mycat.c src/mycat.c src/mycat_options.c
OBJ = $(SRC:src/%.c=build/%.o)

TARGET = bin/mycat

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f build/*.o $(TARGET)

.PHONY: all clean
