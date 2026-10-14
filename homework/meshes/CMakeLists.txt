add_library(meshes STATIC
    sphere.cpp
    sphere.hpp
    ring.cpp
    ring.hpp
)

target_include_directories(meshes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(meshes PUBLIC wgpu_app)
