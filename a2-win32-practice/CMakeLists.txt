cmake_minimum_required(VERSION 3.24)
project(A2Win32Practice LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "This exercise requires Windows and MSVC.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Select the amd64/x64 toolchain.")
endif()

add_executable(a2_win32_practice main.cpp
        App.cpp
        App.h
        utilities.h)
target_compile_features(a2_win32_practice PRIVATE cxx_std_20)
target_compile_definitions(a2_win32_practice PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_compile_options(a2_win32_practice PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(a2_win32_practice PRIVATE user32)