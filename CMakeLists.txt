cmake_minimum_required(VERSION 3.10)
project(lprun VERSION 1.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -pedantic)

# Source files
set(SRC
    src/main.c
    src/disc.c
    src/print_cups.c
    src/print_raw.c
    src/utils.c
)

# Executable
add_executable(lprun ${SRC})

# Header directory
target_include_directories(lprun PRIVATE include)

# CUPS detection
find_package(CUPS REQUIRED)
target_link_libraries(lprun PRIVATE CUPS::CUPS)

# -----------------------
# Install Rules
# -----------------------

# Install binary into system bin directory
install(TARGETS lprun
    RUNTIME DESTINATION bin
)

# Install headers (optional)
install(DIRECTORY include/lprun    DESTINATION include/lprun
    FILES_MATCHING PATTERN "*.h"
)

# Install package config (optional)
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lprunConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lprunConfig.cmake"
    INSTALL_DESTINATION lib/cmake/lprun
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/lprunConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/myprinterConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lprunConfigVersion.cmake"
    DESTINATION lib/cmake/lprun
)

# Print version info
message(STATUS "Building lprun version ${PROJECT_VERSION}")
