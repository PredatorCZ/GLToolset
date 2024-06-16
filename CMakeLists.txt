cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_STANDARD 20)
project(GLToolset VERSION 1.0)
add_compile_options(-Wall -Wextra -Wl,--no-undefined)

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
add_subdirectory(flatbuffers EXCLUDE_FROM_ALL)
add_subdirectory(${TPD_PATH}/ispctex EXCLUDE_FROM_ALL)

build_target(
  NAME
  prime_converters
  TYPE
  OBJECT
  SOURCES
  ${TPD_PATH}/ispctex/ispc_texcomp.cpp
  ${TPD_PATH}/stb/stb_image.c
  ${TPD_PATH}/stb/stb_image_resize.c
  ${TPD_PATH}/mikktspace/mikktspace.c
  src/utils/md2.cpp
  src/utils/image.cpp
  INCLUDES
  ${TPD_PATH}/mikktspace
  ${ISPCTEX_DIR}
  ${TPD_PATH}/ispctex
  ${TPD_PATH}/stb
  DEFINITIONS
  STB_IMAGE_IMPLEMENTATION
  STB_IMAGE_RESIZE_IMPLEMENTATION
  LINKS
  spike
  PROPERTIES
  POSITION_INDEPENDENT_CODE
  ON
)

target_link_libraries(prime_converters ${ISPCTEX_DIR}/kernel_ispc_avx.o ${ISPCTEX_DIR}/kernel_ispc_sse2.o ${ISPCTEX_DIR}/kernel_ispc.o)

build_target(
  NAME
  prime
  TYPE
  OBJECT
  SOURCES
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
  src/graphics/ui_frame.cpp

  src/utils/shader_preprocessor.cpp
  src/utils/reflect.cpp
  src/utils/reflect_graphics.cpp
  src/utils/converters.cpp
  LINKS
  prime_converters
  ImGui
  flatbuffers
  simplecpp)

target_include_directories(prime PUBLIC src/shaders)

add_subdirectory(gltex_view)
add_subdirectory(frame_designer)

add_spike_subdir(gltex)
add_spike_subdir(glmod)
add_spike_subdir(cache)


add_executable(test_curvebatch src/motion/curvebatch_evaluate.cpp)
target_compile_options(test_curvebatch PRIVATE -msse4 -mavx2 -mfma -O4)
enable_testing()
add_test(NAME test_curvebatch COMMAND test_curvebatch)
