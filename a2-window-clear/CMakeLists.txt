cmake_minimum_required(VERSION 4.3)
project(a2_window_clear)

set(CMAKE_CXX_STANDARD 20)

add_executable(a2_window_clear main.cpp
        utilities.h
        Core/App.cpp
        Core/App.h)
