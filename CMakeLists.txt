cmake_minimum_required(VERSION 3.15)
project(izutris)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(izutris
    src/main.cpp
    src/config.cpp
    src/game.cpp
    src/input.cpp
    src/draw.cpp
)