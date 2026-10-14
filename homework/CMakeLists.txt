add_subdirectory(sphere)
add_subdirectory(camera)
add_subdirectory(scene)

add_executable(src main.cpp)
target_link_libraries(src PRIVATE math wgpu_app sphere camera scene)
target_compile_definitions(src PRIVATE
    PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
