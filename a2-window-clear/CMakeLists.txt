cmake_minimum_required(VERSION 4.3)
project(a2_window_clear LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "This exercise requires Windows and MSVC.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Select the amd64/x64 toolchain.")
endif()

include(FetchContent)
FetchContent_Declare(directx_headers
    URL https://github.com/microsoft/DirectX-Headers/archive/refs/tags/v1.614.1.zip
)
FetchContent_MakeAvailable(directx_headers)

# Keep the console subsystem for main() and diagnostic output.
add_executable(a2_window_clear
    main.cpp
    Core/App.cpp
    Core/App.h
        Core/Dx12/Dx12Renderer.cpp
        Core/Dx12/Dx12Renderer.h
        Core/Dx12/CpuDescriptorHeap.cpp
        Core/Dx12/CpuDescriptorHeap.h
        Utils/DxException.cpp
        Utils/DxException.h
        Utils/Exception.h
        Utils/Win32Exception.cpp
        Utils/Win32Exception.h
        Core/Dx12/FenceEvent.cpp
        Core/Dx12/FenceEvent.h
)

target_compile_features(a2_window_clear PRIVATE cxx_std_20)
set_target_properties(a2_window_clear PROPERTIES CXX_EXTENSIONS OFF)

target_compile_definitions(a2_window_clear PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

target_compile_options(a2_window_clear PRIVATE /W4 /permissive- /utf-8)

target_include_directories(a2_window_clear PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Use matching D3D12 and D3DX12 headers before the Windows SDK headers.
target_include_directories(a2_window_clear SYSTEM BEFORE PRIVATE
    "${directx_headers_SOURCE_DIR}/include/directx"
)

target_link_libraries(a2_window_clear PRIVATE
    Microsoft::DirectX-Headers
    user32
    d3d12
    dxgi
)

option(BUILD_TESTING "Build GoogleTest tests" ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
