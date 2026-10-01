add_executable(MatStarterBenchmark
    main.cpp
    Menu.cpp
)

target_link_libraries(MatStarterBenchmark PRIVATE Maths)

target_include_directories(MatStarterBenchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
