cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_STANDARD 20)
project(GLToolset VERSION 1.0)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -rdynamic")
endif()

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)
set(BUILD_SHARED_LIBS
    ON
    CACHE BOOL "")
set(BUILD_STATIC_OBJECTS
    OFF
    CACHE BOOL "")
set(NO_OBJECTS
    ON
    CACHE BOOL "" FORCE)

add_subdirectory(${TPD_PATH}/precore)
include(targetex)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(${TPD_PATH}/precore/spike ${CMAKE_CURRENT_BINARY_DIR}/spike)
add_subdirectory(${TPD_PATH}/precore/gltf)
set_target_properties(spike PROPERTIES OUTPUT_NAME gl_toolset)

add_subdirectory(${TPD_PATH}/imgui)
add_subdirectory(${TPD_PATH}/simplecpp)

build_target(
  NAME
  prime
  TYPE
  OBJECT
  SOURCES
  src/graphics/sampler.cpp
  src/graphics/texture.cpp
  src/graphics/vertex.cpp
  src/common/resource.cpp
  src/utils/shader_preprocessor.cpp
  src/graphics/pipeline.cpp
  src/common/camera.cpp
  src/common/registry.cpp
  LINKS
  precore
  simplecpp)

target_include_directories(prime PUBLIC src/shaders)

add_subdirectory(gltex_view)

add_spike_subdir(gltex)
add_spike_subdir(glmod)
