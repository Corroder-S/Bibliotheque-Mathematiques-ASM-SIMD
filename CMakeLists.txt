cmake_minimum_required(VERSION 3.20)
project(MathStarter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB _ML64_EXES
        "C:/Program Files/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/*/bin/Hostx64/x64/ml64.exe"
        "$ENV{VCToolsInstallDir}bin/Hostx64/x64/ml64.exe"
)
if(_ML64_EXES)
    list(GET _ML64_EXES 0 CMAKE_ASM_MASM_COMPILER)
    enable_language(ASM_MASM OPTIONAL)
else()
    message(WARNING "ml64 not found — ASM sources will be skipped")
    set(CMAKE_ASM_MASM_COMPILER_WORKS FALSE)
endif()

include(FetchContent)

FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark googletest)

enable_testing()

add_subdirectory(Maths)
add_subdirectory(Demo)
add_subdirectory(Tests)
add_subdirectory(MatStarter.Benchmark)
