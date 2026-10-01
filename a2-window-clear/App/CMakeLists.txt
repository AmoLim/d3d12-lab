# Keep the console subsystem for main() and diagnostic output.
add_executable(a2_window_clear
    src/main.cpp
    src/App.cpp
    src/App.h
)
target_include_directories(a2_window_clear PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(a2_window_clear PRIVATE
    a2_renderer a2_core a2_config user32
)
# Preserve the original executable location for IDE/run workflows.
set_target_properties(a2_window_clear PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
a2_configure_target(a2_window_clear)
