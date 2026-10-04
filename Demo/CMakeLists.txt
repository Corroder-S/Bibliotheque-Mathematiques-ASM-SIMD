add_executable(MathStarterDemo
        main.cpp
)
target_link_libraries(MathStarterDemo PRIVATE Maths)
target_include_directories(MathStarterDemo PRIVATE ${CMAKE_SOURCE_DIR})
