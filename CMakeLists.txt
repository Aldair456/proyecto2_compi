cmake_minimum_required(VERSION 4.0)
project(proyecto)

set(CMAKE_CXX_STANDARD 14)

include_directories(parser)
include_directories(scanner)
include_directories(visitors)

add_executable(proyecto
        parser/ast.cpp
        parser/ast.h
        parser/parser.cpp
        parser/parser.h
        scanner/scanner.cpp
        scanner/scanner.h
        scanner/token.cpp
        scanner/token.h
        tests/base/test1.c
        tests/base/test2.c
        tests/base/test3.c
        tests/base/test4.c
        tests/base/test5.c
        tests/extensions/ext1.c
        tests/extensions/ext2.c
        tests/extensions/ext3.c
        tests/extensions/ext4.c
        tests/extensions/ext5.c
        tests/functions/func1.c
        tests/functions/func2.c
        tests/functions/func3.c
        tests/optimization/opt1.c
        tests/optimization/opt2.c
        tests/optimization/opt3.c
        tests/optimization/opt4.c
        tests/optimization/opt5.c
        visitors/codegen.cpp
        visitors/codegen.h
        main.cpp)
