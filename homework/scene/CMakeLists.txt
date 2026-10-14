add_library(scene STATIC scene.cpp scene.hpp)
target_include_directories(scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene PUBLIC math)
