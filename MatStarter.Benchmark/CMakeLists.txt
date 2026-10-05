add_executable(MathStarterBenchmark
    BenchmarkGB.cpp
)

target_link_libraries(MathStarterBenchmark PRIVATE
    Maths
    benchmark::benchmark
)

target_include_directories(MathStarterBenchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
