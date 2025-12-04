# ==============================================================================
# lprun - Advanced command-line printer & scanner tool
# Makefile v1.2.0
# ==============================================================================

# Project configuration
PROJECT     := lprun
VERSION     := 1.2.0
AUTHOR      := $(shell git config user.name 2>/dev/null || echo "benusf")
EMAIL       := $(shell git config user.email 2>/dev/null || echo "benheddi.usf@yahoo.com")

# Directories
SRC_DIR     := src
INC_DIR     := include
BUILD_DIR   := build
BIN_DIR     := bin
OBJ_DIR     := $(BUILD_DIR)/obj
DEP_DIR     := $(BUILD_DIR)/dep

# Compiler and tools
CC          := gcc
AR          := ar
INSTALL     := install
RM          := rm -f
MKDIR       := mkdir -p
CP          := cp
SED         := sed
GREP        := grep
FIND        := find
STRIP       := strip

# Source files
SRCS        := $(wildcard $(SRC_DIR)/*.c)
OBJS        := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS        := $(patsubst $(SRC_DIR)/%.c,$(DEP_DIR)/%.d,$(SRCS))

# Default target
.DEFAULT_GOAL := all

# Compiler flags
WARNINGS    := -Wall -Wextra -pedantic \
               -Wformat=2 -Wshadow -Wwrite-strings \
               -Wstrict-prototypes -Wold-style-definition \
               -Wredundant-decls -Wnested-externs -Wmissing-prototypes \
               -Wno-unused-parameter

CFLAGS      := -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
               $(WARNINGS) -I$(INC_DIR) -pthread \
               -O2 -g -fstack-protector-strong -fPIE

# Linker flags
LDFLAGS     := -pthread -pie -Wl,-z,relro,-z,now
LDLIBS      := -lcups

# Installation paths
PREFIX      := /usr/local
BINDIR      := $(PREFIX)/bin
MANDIR      := $(PREFIX)/share/man/man1
DATADIR     := $(PREFIX)/share/$(PROJECT)
SYSCONFDIR  := /etc/$(PROJECT)

# Colors for pretty output
ifneq ($(TERM),)
    COLOR_RESET   := \033[0m
    COLOR_GREEN   := \033[32m
    COLOR_YELLOW  := \033[33m
    COLOR_BLUE    := \033[34m
    COLOR_MAGENTA := \033[35m
    COLOR_CYAN    := \033[36m
endif

# Progress indicator
ifneq ($(VERBOSE),)
    Q :=
    E := @:
else
    Q := @
    E := @echo
endif

# ==============================================================================
# Build Rules
# ==============================================================================

all: $(BIN_DIR)/$(PROJECT)

# Link executable
$(BIN_DIR)/$(PROJECT): $(OBJS) | $(BIN_DIR)
	$(E) "$(COLOR_CYAN)[LD]$(COLOR_RESET) Linking $(PROJECT)"
	$(Q)$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	$(E) "$(COLOR_BLUE)[CC]$(COLOR_RESET) $<"
	$(Q)$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

# Create directories
$(BIN_DIR) $(OBJ_DIR) $(DEP_DIR):
	$(E) "$(COLOR_YELLOW)[MKDIR]$(COLOR_RESET) $@"
	$(Q)$(MKDIR) $@

# Include dependency files
-include $(DEPS)

# ==============================================================================
# Utilities
# ==============================================================================

# Clean build artifacts
clean:
	$(E) "$(COLOR_MAGENTA)[CLEAN]$(COLOR_RESET) Removing build artifacts"
	$(Q)$(RM) -r $(BUILD_DIR) $(BIN_DIR)

# Remove all generated files
distclean: clean
	$(E) "$(COLOR_MAGENTA)[DISTCLEAN]$(COLOR_RESET) Removing all generated files"
	$(Q)$(FIND) . -name "*.o" -o -name "*.d" -o -name "*.a" -o -name "*.so" -o -name "*~" | xargs $(RM) -f
	$(Q)$(RM) -f $(PROJECT) $(PROJECT)-*.tar.gz tags cscope.out

# Install binary and man page
install: all
	$(E) "$(COLOR_GREEN)[INSTALL]$(COLOR_RESET) Installing $(PROJECT) v$(VERSION)"
	$(Q)$(MKDIR) $(DESTDIR)$(BINDIR)
	$(Q)$(INSTALL) -m 755 $(BIN_DIR)/$(PROJECT) $(DESTDIR)$(BINDIR)/
	$(Q)$(STRIP) $(DESTDIR)$(BINDIR)/$(PROJECT)
	
	# Install man page if it exists
	$(Q)if [ -f doc/$(PROJECT).1 ]; then \
		$(MKDIR) $(DESTDIR)$(MANDIR); \
		$(INSTALL) -m 644 doc/$(PROJECT).1 $(DESTDIR)$(MANDIR)/; \
		gzip -f $(DESTDIR)$(MANDIR)/$(PROJECT).1; \
		echo "$(COLOR_GREEN)[INSTALL]$(COLOR_RESET) Installed man page"; \
	fi
	
	# Install config files if they exist
	$(Q)if [ -d etc/ ]; then \
		$(MKDIR) $(DESTDIR)$(SYSCONFDIR); \
		$(CP) -r etc/* $(DESTDIR)$(SYSCONFDIR)/; \
		echo "$(COLOR_GREEN)[INSTALL]$(COLOR_RESET) Installed configuration files"; \
	fi
	
	$(E) "$(COLOR_GREEN)[INSTALL]$(COLOR_RESET) Installation complete"

# Uninstall
uninstall:
	$(E) "$(COLOR_MAGENTA)[UNINSTALL]$(COLOR_RESET) Removing $(PROJECT)"
	$(Q)$(RM) $(DESTDIR)$(BINDIR)/$(PROJECT)
	$(Q)$(RM) $(DESTDIR)$(MANDIR)/$(PROJECT).1.gz
	$(Q)$(RM) -r $(DESTDIR)$(SYSCONFDIR)
	$(E) "$(COLOR_GREEN)[UNINSTALL]$(COLOR_RESET) Uninstallation complete"

# Create debug build
debug: CFLAGS += -DDEBUG -Og -ggdb3 -fsanitize=address -fsanitize=undefined
debug: LDFLAGS += -fsanitize=address -fsanitize=undefined
debug: clean all

# Create release build
release: CFLAGS += -O3 -DNDEBUG -flto -march=native
release: LDFLAGS += -flto
release: clean all
	$(Q)$(STRIP) $(BIN_DIR)/$(PROJECT)

# Run tests
test: all
	$(E) "$(COLOR_YELLOW)[TEST]$(COLOR_RESET) Running tests"
	$(Q)if [ -f tests/run_tests.sh ]; then \
		cd tests && ./run_tests.sh; \
	else \
		echo "No tests found"; \
	fi

# Create source distribution
dist: distclean
	$(E) "$(COLOR_YELLOW)[DIST]$(COLOR_RESET) Creating distribution package"
	$(Q)$(MKDIR) $(PROJECT)-$(VERSION)
	$(Q)$(CP) -r src include Makefile README.md LICENSE doc etc tests $(PROJECT)-$(VERSION)/
	$(Q)tar czf $(PROJECT)-$(VERSION).tar.gz $(PROJECT)-$(VERSION)
	$(Q)$(RM) -r $(PROJECT)-$(VERSION)
	$(E) "$(COLOR_GREEN)[DIST]$(COLOR_RESET) Created $(PROJECT)-$(VERSION).tar.gz"

# Generate tags for code navigation
tags:
	$(E) "$(COLOR_YELLOW)[TAGS]$(COLOR_RESET) Generating tags"
	$(Q)ctags -R --exclude=build --exclude=bin .

# Generate cscope database
cscope:
	$(E) "$(COLOR_YELLOW)[CSCOPE]$(COLOR_RESET) Generating cscope database"
	$(Q)$(FIND) . -name "*.c" -o -name "*.h" > cscope.files
	$(Q)cscope -b -q

# Check code style
checkstyle:
	$(E) "$(COLOR_YELLOW)[STYLE]$(COLOR_RESET) Checking code style"
	$(Q)if command -v clang-format >/dev/null 2>&1; then \
		clang-format --dry-run --Werror $(SRCS) $(wildcard $(INC_DIR)/*.h); \
	else \
		echo "clang-format not installed, skipping style check"; \
	fi

# Format code
format:
	$(E) "$(COLOR_YELLOW)[FORMAT]$(COLOR_RESET) Formatting code"
	$(Q)if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i $(SRCS) $(wildcard $(INC_DIR)/*.h); \
	else \
		echo "clang-format not installed, cannot format"; \
	fi

# Show project information
info:
	@echo "$(COLOR_CYAN)========================================$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)$(PROJECT) v$(VERSION)$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)========================================$(COLOR_RESET)"
	@echo "Author:    $(AUTHOR) <$(EMAIL)>"
	@echo "Compiler:  $(CC)"
	@echo "Sources:   $(words $(SRCS)) files"
	@echo "Flags:     $(CFLAGS)"
	@echo "Libs:      $(LDLIBS)"
	@echo ""
	@echo "$(COLOR_YELLOW)Source files:$(COLOR_RESET)"
	@for f in $(SRCS); do echo "  $$f"; done
	@echo ""
	@echo "$(COLOR_YELLOW)Available targets:$(COLOR_RESET)"
	@echo "  all       - Build project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  install   - Install to $(PREFIX)"
	@echo "  uninstall - Remove installation"
	@echo "  debug     - Build with debug flags"
	@echo "  release   - Build optimized release"
	@echo "  test      - Run tests"
	@echo "  dist      - Create source distribution"
	@echo "  tags      - Generate ctags"
	@echo "  cscope    - Generate cscope database"
	@echo "  checkstyle- Check code style"
	@echo "  format    - Format code"
	@echo "  info      - Show project info"
	@echo "$(COLOR_CYAN)========================================$(COLOR_RESET)"

# ==============================================================================
# Phony targets
# ==============================================================================
.PHONY: all clean distclean install uninstall debug release test dist tags cscope checkstyle format info

# ==============================================================================
# Help target (default when just running 'make')
# ==============================================================================
help: info

# ==============================================================================
# Special targets
# ==============================================================================
.SUFFIXES: .c .o .d
.DELETE_ON_ERROR:
