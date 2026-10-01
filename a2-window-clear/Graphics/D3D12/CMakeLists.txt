add_library(a2_d3d12 STATIC
    src/CpuDescriptorHeap.cpp
    src/FenceEvent.cpp
)
target_sources(a2_d3d12 PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/D3D12/CpuDescriptorHeap.h
        include/D3D12/FenceEvent.h
)
target_include_directories(a2_d3d12 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(a2_d3d12
    PUBLIC a2_directx
    PRIVATE a2_core d3d12
)
a2_configure_target(a2_d3d12)
