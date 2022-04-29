function(build_shader shader_name)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${shader_name}
       "//Pre-compiled source: ${shader_name}\n")
  list(PREPEND ARGN "common/defs.glsl")
  foreach(cFile ${ARGN})
    file(READ ${cFile} cFileContents)
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${shader_name}
         "//File: ${cFile}\n${cFileContents}")
  endforeach()
endfunction()

build_shader(
  vertex_lib.glsl
  common/quat.glsl
  common/dual_quat.glsl
  common/ts_normal.vert
  common/common.h.glsl
  common/common.vert
  common/light_omni.h.glsl
  common/light_omni.vert)

build_shader(
  fragment_lib.glsl
  common/ts_normal.frag
  common/light_omni.h.glsl
  common/light_omni.frag)

build_shader(
    single_texture/main_albedo.frag
    single_texture/main.h.glsl
    single_texture/main_albedo.frag)

build_shader(
  single_texture/main_normal.frag
  single_texture/main.h.glsl
  single_texture/main_normal.frag)

build_shader(
    single_texture/main.vert
    single_texture/main.vert)
