include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

# Match the MSVC runtime used by the application and test executable.
set(gtest_force_shared_crt ON CACHE BOOL "Use the shared MSVC runtime" FORCE)
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

add_executable(cpu_descriptor_heap_tests
    CpuDesciptorHeapTests.cpp
    D3D12TestFixture.h
        ../Core/Dx12/CpuDescriptorHeap.cpp
    ../Utils/DxException.cpp
)

target_compile_features(cpu_descriptor_heap_tests PRIVATE cxx_std_20)
set_target_properties(cpu_descriptor_heap_tests PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(cpu_descriptor_heap_tests PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
)
target_compile_options(cpu_descriptor_heap_tests PRIVATE /W4 /permissive- /utf-8)

target_include_directories(cpu_descriptor_heap_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_include_directories(cpu_descriptor_heap_tests SYSTEM BEFORE PRIVATE
    "${directx_headers_SOURCE_DIR}/include/directx"
)
target_link_libraries(cpu_descriptor_heap_tests PRIVATE
    GTest::gtest_main
    Microsoft::DirectX-Headers
    d3d12
    dxgi
)

add_executable(dx12_renderer_tests
    Dx12RendererTests.cpp
    ../Core/Dx12/Dx12Renderer.cpp
    ../Core/Dx12/CpuDescriptorHeap.cpp
    ../Core/Dx12/FenceEvent.cpp
    ../Utils/DxException.cpp
    ../Utils/Win32Exception.cpp
)

target_compile_features(dx12_renderer_tests PRIVATE cxx_std_20)
set_target_properties(dx12_renderer_tests PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(dx12_renderer_tests PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
)
target_compile_options(dx12_renderer_tests PRIVATE /W4 /permissive- /utf-8)
target_include_directories(dx12_renderer_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(dx12_renderer_tests SYSTEM BEFORE PRIVATE
    "${directx_headers_SOURCE_DIR}/include/directx"
)
target_link_libraries(dx12_renderer_tests PRIVATE
    GTest::gtest_main Microsoft::DirectX-Headers user32 d3d12 dxgi
)
