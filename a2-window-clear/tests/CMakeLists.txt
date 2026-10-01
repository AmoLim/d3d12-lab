include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

# Match the MSVC runtime used by the application and test executable.
set(gtest_force_shared_crt ON CACHE BOOL "Use the shared MSVC runtime" FORCE)
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

# Link the production modules instead of compiling copies of their sources.
add_executable(cpu_descriptor_heap_tests
    CpuDesciptorHeapTests.cpp
    D3D12TestFixture.h
)
target_link_libraries(cpu_descriptor_heap_tests PRIVATE
    GTest::gtest_main a2_d3d12 d3d12 dxgi
)
a2_configure_target(cpu_descriptor_heap_tests)

add_executable(dx12_renderer_tests Dx12RendererTests.cpp)
target_link_libraries(dx12_renderer_tests PRIVATE
    GTest::gtest_main a2_renderer a2_core a2_config user32
)
a2_configure_target(dx12_renderer_tests)
