cmake_minimum_required(VERSION 3.30)
project(c-rex C)

set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug/Release)" FORCE)
endif ()

set(CMAKE_C_FLAGS_DEBUG "-ggdb3 -Og -fsanitize=address")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

add_executable(c-rex
        main.c
        src/ast.c
        include/ast.h
        src/lexer.c
        include/lexer.h
        src/rrange.c
        include/rrange.h
        include/utf8.h
        include/charclass.h
)

target_include_directories(c-rex PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
