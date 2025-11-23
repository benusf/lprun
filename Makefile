CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -Iinclude
LDFLAGS = -lcups

SRC     = src/main.c src/discover.c src/print_cups.c src/print_raw.c src/utils.c
OBJ     = $(SRC:.c=.o)
BIN     = lprun

PREFIX  = /usr/local

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(BIN)

install: $(BIN)
	mkdir -p $(PREFIX)/bin
	cp $(BIN) $(PREFIX)/bin/

	mkdir -p $(PREFIX)/include/lprun
	cp include/*.h $(PREFIX)/include/lprun/

uninstall:
	rm -f $(PREFIX)/bin/$(BIN)
	rm -rf $(PREFIX)/include/lprun

.PHONY: all clean install uninstall
