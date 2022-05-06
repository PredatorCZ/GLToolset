project(simplecpp)

add_library(simplecpp SHARED simplecpp.cpp)
target_include_directories(simplecpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
