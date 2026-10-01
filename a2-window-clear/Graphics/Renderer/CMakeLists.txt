add_library(a2_renderer STATIC
    src/Dx12Renderer.cpp
)
target_sources(a2_renderer PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/Renderer/Dx12Renderer.h
)
target_include_directories(a2_renderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
# The public header uses SDK types, but not the D3D12 helper definitions.
target_link_libraries(a2_renderer
    PUBLIC a2_directx
    PRIVATE a2_d3d12 a2_core a2_config d3d12 dxgi
)
a2_configure_target(a2_renderer)
