cmake_minimum_required(VERSION 3.20)
project(tests)

find_package(OpenCV REQUIRED)

add_executable(tests tests.cpp)
target_compile_features(tests PRIVATE cxx_std_20)
target_compile_options(tests PRIVATE -Wall -Wextra)
target_include_directories(tests PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tests PRIVATE ${OpenCV_LIBS})
