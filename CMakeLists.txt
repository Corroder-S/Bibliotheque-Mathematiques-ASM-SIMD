cmake_minimum_required(VERSION 3.20)
project(MathStarter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CMAKE_ASM_MASM_COMPILER
    NAMES ml64
    HINTS
        "$ENV{VCToolsInstallDir}bin/Hostx64/x64"
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64"
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.38.33130/bin/Hostx64/x64"
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64"
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64"
    REQUIRED
)
enable_language(ASM_MASM)

enable_testing()

add_subdirectory(Maths)
add_subdirectory(MatStarter.Benchmark)
add_subdirectory(Tests)
