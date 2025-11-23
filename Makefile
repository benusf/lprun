# Compiler
CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -Iinclude
LDFLAGS = -lcups

# Source files
SRC     = src/main.c src/disc.c src/print_cups.c src/print_raw.c src/utils.c
OBJ     = $(SRC:.c=.o)

# Binary name
BIN     = lprun

# Install prefix
PREFIX  = /usr/local

# -----------------------
# Build rules
# -----------------------

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

# Compile .c -> .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

# -----------------------
# Install / Uninstall
# -----------------------

install: $(BIN)
	mkdir -p $(PREFIX)/bin
	cp $(BIN) $(PREFIX)/bin/

	mkdir -p $(PREFIX)/include/lprun
	cp include/*.h $(PREFIX)/include/lprun/

uninstall:
	rm -f $(PREFIX)/bin/$(BIN)
	rm -rf $(PREFIX)/include/lprun

.PHONY: all clean install uninstall
