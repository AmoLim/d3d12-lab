cmake_minimum_required(VERSION 4.3)
project(a2_window_clear LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "This exercise requires Windows and MSVC.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Select the amd64/x64 toolchain.")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

# Shared platform requirements, not a runtime module.
add_library(a2_platform_options INTERFACE)
target_compile_features(a2_platform_options INTERFACE cxx_std_20)
target_compile_definitions(a2_platform_options INTERFACE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
)

function(a2_configure_target target)
    target_link_libraries(${target} PRIVATE a2_platform_options)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
endfunction()

include(FetchContent)
FetchContent_Declare(directx_headers
    URL https://github.com/microsoft/DirectX-Headers/archive/refs/tags/v1.614.1.zip
)
FetchContent_MakeAvailable(directx_headers)

# Public SDK types do not expose our D3D12 implementation helpers.
add_library(a2_directx INTERFACE)
target_include_directories(a2_directx SYSTEM BEFORE INTERFACE
    "${directx_headers_SOURCE_DIR}/include/directx"
)
target_link_libraries(a2_directx INTERFACE
    a2_platform_options Microsoft::DirectX-Headers
)

# Runtime dependency direction: App -> Renderer -> D3D12 -> Core.
# App and Renderer share Config; neither Config nor Renderer depends on App.
add_subdirectory(Config)
add_subdirectory(Core)
add_subdirectory(Graphics/D3D12)
add_subdirectory(Graphics/Renderer)
add_subdirectory(App)

option(BUILD_TESTING "Build GoogleTest tests" ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
