add_library(sphere STATIC
    sphere.cpp
    sphere.hpp
)

target_include_directories(sphere PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sphere PUBLIC wgpu_app)
