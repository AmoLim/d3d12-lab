add_library(a2_core STATIC
    src/DxException.cpp
    src/Win32Exception.cpp
)
target_sources(a2_core PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/Core/Exception.h
        include/Core/DxException.h
        include/Core/Win32Exception.h
)
target_include_directories(a2_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(a2_core PUBLIC a2_platform_options)
a2_configure_target(a2_core)
