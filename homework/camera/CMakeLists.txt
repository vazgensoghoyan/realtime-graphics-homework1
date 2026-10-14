find_package(SDL3 REQUIRED CONFIG)

add_library(camera STATIC
    camera.cpp
    camera.hpp
)

target_include_directories(camera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(camera PUBLIC math SDL3::SDL3)
