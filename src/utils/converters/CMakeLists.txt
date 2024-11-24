add_subdirectory(${TPD_PATH}/ispctex ${CMAKE_CURRENT_BINARY_DIR}/ispctex EXCLUDE_FROM_ALL)

build_target(
  NAME
  prime_converters
  TYPE
  OBJECT
  SOURCES
  converters.cpp
  image.cpp
  md2.cpp

  ${TPD_PATH}/ispctex/ispc_texcomp.cpp
  ${TPD_PATH}/stb/stb_image.c
  ${TPD_PATH}/stb/stb_image_resize.c
  ${TPD_PATH}/mikktspace/mikktspace.c
  INCLUDES
  ${TPD_PATH}/mikktspace
  ${ISPCTEX_DIR}
  ${TPD_PATH}/ispctex
  ${TPD_PATH}/stb
  DEFINITIONS
  STB_IMAGE_IMPLEMENTATION
  STB_IMAGE_RESIZE_IMPLEMENTATION
  LINKS
  spike-interface
  PROPERTIES
  POSITION_INDEPENDENT_CODE
  ON
)

target_link_libraries(prime_converters ${ISPCTEX_DIR}/kernel_ispc_avx2.o ${ISPCTEX_DIR}/kernel_ispc_sse4.o ${ISPCTEX_DIR}/kernel_ispc.o)
