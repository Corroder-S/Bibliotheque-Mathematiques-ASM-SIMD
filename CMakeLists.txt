cmake_minimum_required(VERSION 3.20)
project(MathStarter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ASM_MASM_COMPILER "ml64")
enable_language(ASM_MASM)

add_subdirectory(Maths)
add_subdirectory(MatStarter.Benchmark)
