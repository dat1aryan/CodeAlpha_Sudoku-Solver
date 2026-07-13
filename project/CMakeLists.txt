cmake_minimum_required(VERSION 3.20)
project(SudokuStudio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(SudokuStudio WIN32 src/main.cpp)
target_compile_definitions(SudokuStudio PRIVATE UNICODE _UNICODE)
target_link_libraries(SudokuStudio PRIVATE user32 gdi32)
