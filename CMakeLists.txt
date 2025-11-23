cmake_minimum_required(VERSION 3.5)
project(leprinter C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -pedantic)

set(SRC
    src/main.c
    src/disc.c
    src/print_cups.c
    src/print_raw.c
    src/utils.c
)

add_executable(leprinter ${SRC})
target_include_directories(leprinter PRIVATE src)
target_link_libraries(leprinter PRIVATE cups)
