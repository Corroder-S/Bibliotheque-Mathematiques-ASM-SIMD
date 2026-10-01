add_library(Maths STATIC
    VecBatchRef.cpp
    VecBatchSSE.cpp
    MatBatchRef.cpp
    MatBatchSSE.cpp
    VecASM.asm
    ../Platform/CpuFeatures.cpp
)

target_include_directories(Maths PUBLIC
    ${CMAKE_SOURCE_DIR}
)
