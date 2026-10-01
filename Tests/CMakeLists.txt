# ============================================================
#  Tests — VS CppUnitTestFramework + CTest + Ninja + MSVC
# ============================================================

# Localise les headers du framework VS Unit Test
find_path(VS_UNITTEST_INCLUDE_DIR
    NAMES CppUnitTest.h
    HINTS "$ENV{VSINSTALLDIR}/VC/Auxiliary/VS/UnitTest/include"
)

# Localise la lib x64 du framework
find_library(VS_UNITTEST_LIB
    NAMES Microsoft.VisualStudio.TestTools.CppUnitTestFramework
    HINTS "$ENV{VSINSTALLDIR}/VC/Auxiliary/VS/UnitTest/lib/x64"
)

# Localise vstest.console.exe pour CTest
find_program(VSTEST_CONSOLE
    NAMES vstest.console.exe
    HINTS "$ENV{VSINSTALLDIR}/Common7/IDE/CommonExtensions/Microsoft/TestWindow"
)

if(NOT VS_UNITTEST_INCLUDE_DIR)
    message(FATAL_ERROR "VS Unit Test headers introuvables. Vérifie ton installation Visual Studio.")
endif()
if(NOT VS_UNITTEST_LIB)
    message(FATAL_ERROR "VS Unit Test lib introuvable (x64). Vérifie ton installation Visual Studio.")
endif()
if(NOT VSTEST_CONSOLE)
    message(FATAL_ERROR "vstest.console.exe introuvable. Vérifie ton installation Visual Studio.")
endif()

# DLL de test — obligatoire pour VS Unit Test Framework
add_library(MathStarterTests SHARED
    Vec3Tests.cpp
    Vec4Tests.cpp
    Matrix3x3Tests.cpp
    Matrix4x4Tests.cpp
    VecBatchTest.cpp
    MatBatchTest.cpp
    BenchmarkTests.cpp
    CpuFeaturesTests.cpp
)

target_include_directories(MathStarterTests PRIVATE
    ${VS_UNITTEST_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(MathStarterTests PRIVATE
    Maths
    ${VS_UNITTEST_LIB}
)

# CTest délègue l'exécution à vstest.console.exe
add_test(
    NAME MathStarterTests
    COMMAND "${VSTEST_CONSOLE}" "$<TARGET_FILE:MathStarterTests>"
)
