cmake_minimum_required(VERSION 3.24)
project(A1EnvironmentCheck LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "A1 requires Windows and the MSVC toolchain, not MinGW or WSL.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Select the amd64/x64 toolchain for A1.")
endif()

add_executable(a1_environment_check main.cpp)
target_compile_features(a1_environment_check PRIVATE cxx_std_20)
target_compile_definitions(a1_environment_check PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(a1_environment_check PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(a1_environment_check PRIVATE d3d12 dxgi)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME a1_environment_check COMMAND a1_environment_check)
    set_tests_properties(a1_environment_check PROPERTIES TIMEOUT 30)
endif()
