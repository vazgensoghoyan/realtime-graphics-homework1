add_executable(src main.cpp)
target_link_libraries(src PRIVATE math wgpu_app)
target_compile_definitions(src PRIVATE
    PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
