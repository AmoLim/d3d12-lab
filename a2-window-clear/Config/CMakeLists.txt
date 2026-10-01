add_library(a2_config INTERFACE)
target_sources(a2_config INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/Config/WindowConfig.h
)
target_link_libraries(a2_config INTERFACE a2_platform_options)
