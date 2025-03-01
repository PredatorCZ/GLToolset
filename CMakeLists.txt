cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_STANDARD 23)
project(GLToolset VERSION 1.0)
add_compile_options(-Wall -Wextra -Wl,--no-undefined -fsanitize=address,leak,undefined)
add_link_options(-fsanitize=address,leak,undefined)
add_definitions(-D_SQ64)

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)
set(BUILD_SHARED_LIBS ON)
option(NO_OBJECTS "" ON)
option(CLI "" ON)
option(GLTF "" OFF)

add_subdirectory(${TPD_PATH}/spike)
include(targetex)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/fbs)

set_target_properties(spike_cli PROPERTIES OUTPUT_NAME gl_toolset)

add_subdirectory(${TPD_PATH}/imgui)
add_subdirectory(${TPD_PATH}/simplecpp)

add_subdirectory(src/utils/converters)
add_subdirectory(src/utils/reflect)

build_target(
  NAME
  prime
  TYPE
  OBJECT
  SOURCES
  src/common/constants.cpp
  src/common/resource.cpp
  src/common/camera.cpp
  src/common/registry.cpp

  src/graphics/sampler.cpp
  src/graphics/texture.cpp
  src/graphics/vertex.cpp
  src/graphics/model_single.cpp
  src/graphics/program.cpp
  src/graphics/frame_buffer.cpp
  src/graphics/post_process.cpp

  src/utils/shader_preprocessor.cpp
  src/utils/playground.cpp
  src/utils/debug.cpp
  LINKS
  prime_converters
  ImGui
  simplecpp
)

add_subdirectory(src/script)

target_include_directories(prime PUBLIC src/shaders)

add_subdirectory(gltex_view)

add_spike_subdir(gltex)
add_spike_subdir(glmod)
add_spike_subdir(cache)
add_spike_subdir(nutty_shader)

add_executable(test_curvebatch_avx src/motion/curvebatch_evaluate.cpp)
target_compile_options(test_curvebatch_avx PRIVATE -msse4 -mavx2 -mfma -O4)
target_link_libraries(test_curvebatch_avx spike-interface)

#add_executable(test_curvebatch_sse src/motion/curvebatch_evaluate.cpp)
#target_compile_options(test_curvebatch_sse PRIVATE -msse4 -O4)
#target_link_libraries(test_curvebatch_sse spike-interface)
enable_testing()
add_test(NAME test_curvebatch_avx COMMAND test_curvebatch_avx)
#add_test(NAME test_curvebatch_sse COMMAND test_curvebatch_sse)


add_subdirectory(tests)
