add_executable(MatStarterBenchmarkGB
    BenchmarkGB.cpp
)

target_link_libraries(MatStarterBenchmarkGB PRIVATE
    Maths
    benchmark::benchmark
)

target_include_directories(MatStarterBenchmarkGB PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
