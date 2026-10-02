cmake_minimum_required(VERSION 3.20)
project(MathStarter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CMAKE_ASM_MASM_COMPILER
    NAMES ml64
    HINTS
        "$ENV{VCToolsInstallDir}/bin/Hostx64/x64"
        "$ENV{VSINSTALLDIR}/VC/Tools/MSVC/$ENV{VCToolsVersion}/bin/Hostx64/x64"
    REQUIRED
)
enable_language(ASM_MASM)

include(FetchContent)
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_subdirectory(Maths)
add_subdirectory(MatStarter.Benchmark)

# Cible pour lancer les tests VS depuis CLion
find_program(MSBUILD_EXE MSBuild
    HINTS
        "C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/MSBuild/Current/Bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/MSBuild/Current/Bin"
)
find_program(VSTEST_EXE vstest.console
    HINTS
        "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/Extensions/TestPlatform"
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/Extensions/TestPlatform"
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/Extensions/TestPlatform"
)
if(MSBUILD_EXE AND VSTEST_EXE)
    add_custom_target(RunTests
        COMMAND ${MSBUILD_EXE}
            "${CMAKE_SOURCE_DIR}/Tests/MathStarter.Tests.vcxproj"
            /p:Configuration=Debug /p:Platform=x64 /nologo /v:minimal
        COMMAND ${VSTEST_EXE}
            "${CMAKE_SOURCE_DIR}/bin/x64/Debug/MathStarter.Tests.dll"
            /Platform:x64
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Build + run unit tests (VS CppUnitTestFramework)"
    )
endif()
