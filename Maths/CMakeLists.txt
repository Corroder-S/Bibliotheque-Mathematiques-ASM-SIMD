add_library(Maths STATIC
    Ref/VecBatchRef.cpp
    Ref/MatBatchRef.cpp
    SSE/VecBatchSSE.cpp
    SSE/MatBatchSSE.cpp
    ../Platform/CpuFeatures.cpp
    ASM/ASM_Vec3.asm
    ASM/ASM_Vec3.h
)

target_include_directories(Maths PUBLIC
    ${CMAKE_SOURCE_DIR}
)
