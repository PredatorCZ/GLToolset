cmake_minimum_required(VERSION 3.3)

project(GLTEX VERSION 1.0)

set(ISPCTEX_DIR ${CMAKE_CURRENT_BINARY_DIR}/ispctex)
file(MAKE_DIRECTORY ${ISPCTEX_DIR})

build_target(
  NAME
  make_gltex
  TYPE
  ESMODULE
  SOURCES
  make_gltex.cpp
  ${TPD_PATH}/ispctex/ispc_texcomp.cpp
  ${TPD_PATH}/stb/stb_image.c
  ${TPD_PATH}/stb/stb_image_resize.c
  INCLUDES
  ${ISPCTEX_DIR}
  ${TPD_PATH}/ispctex
  ${TPD_PATH}/stb
  LINKS
  precore
  DEFINITIONS
  STB_IMAGE_IMPLEMENTATION
  STB_IMAGE_RESIZE_IMPLEMENTATION
  AUTHOR
  "Lukas Cone"
  DESCR
  "OpenGL texture creator"
  START_YEAR
  2022)

#add_custom_target(kernel_ispc ispc -g --arch=x86-64 --target=avx,sse2 --opt=fast-math --pic -o ${ISPCTEX_DIR}/kernel_ispc.o -h ${ISPCTEX_DIR}/kernel_ispc.h ${TPD_PATH}/ispctex/kernel.ispc)
#add_dependencies(make_gltex kernel_ispc)

target_link_libraries(make_gltex ${ISPCTEX_DIR}/kernel_ispc_avx.o ${ISPCTEX_DIR}/kernel_ispc_sse2.o ${ISPCTEX_DIR}/kernel_ispc.o)
