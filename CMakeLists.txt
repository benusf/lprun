cmake_minimum_required(VERSION 3.10)
project(lprun VERSION 1.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -pedantic -std=c11 -D_DEFAULT_SOURCE -pthread)

# -----------------------
# Sources
# -----------------------
set(SRC
    src/lprun.c
    src/disc.c
    src/print_cups.c
    src/print_raw.c
    src/utils.c
    src/printer_list.c
    src/history.c
    src/scanner.c
)

find_package(Threads REQUIRED)

# ONLY ONE add_executable FOR lprun!
add_executable(lprun ${SRC})
target_include_directories(lprun PRIVATE include)

# Link pthreads FIRST
target_link_libraries(lprun PRIVATE Threads::Threads)

# -----------------------
# Find CUPS: try pkg-config first
# -----------------------
find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
    # QUIET avoids fatal error if libcups.pc not found
    pkg_check_modules(CUPS QUIET IMPORTED_TARGET libcups)
    if(CUPS_FOUND)
        message(STATUS "Using CUPS via pkg-config")
        target_include_directories(lprun PRIVATE ${CUPS_INCLUDE_DIRS})
        # Use keyword signature for consistency
        target_link_libraries(lprun PRIVATE PkgConfig::CUPS)
    else()
        message(WARNING "pkg-config found, but libcups not found. Falling back to manual linking")
        target_include_directories(lprun PRIVATE /usr/include/cups)
        target_link_libraries(lprun PRIVATE cups)
    endif()
else()
    message(WARNING "pkg-config not found. Falling back to manual linking")
    target_include_directories(lprun PRIVATE /usr/include/cups)
    target_link_libraries(lprun PRIVATE cups)
endif()

# -----------------------
# Install Rules
# -----------------------
install(TARGETS lprun
        RUNTIME DESTINATION bin)

install(DIRECTORY include/
        DESTINATION include/lprun
        FILES_MATCHING PATTERN "*.h")

# -----------------------
# Optional package config
# -----------------------
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
    "${CMAKE_CURRENT_BINARY_DIR}/lprunConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lprunConfigVersion.cmake"
    DESTINATION lib/cmake/lprun
)

message(STATUS "Building lprun version ${PROJECT_VERSION}")
