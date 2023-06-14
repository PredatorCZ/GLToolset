find_package(Flatbuffers REQUIRED)
set(FLATC_GEN "-o;${CMAKE_BINARY_DIR}/fbs;-I;${CMAKE_CURRENT_SOURCE_DIR};--cpp;--no-includes;--gen-mutable;--filename-suffix;.fbs;--filename-ext;hpp")
set(FLATC_REFLECT "-o;${CMAKE_BINARY_DIR}/fbs/reflected;-I;${CMAKE_CURRENT_SOURCE_DIR};--cpp;--filename-suffix;.fbs;--filename-ext;hpp;--reflect-names")
set(FLATC_NATIVE "-o;${CMAKE_BINARY_DIR}/fbs/native;-I;${CMAKE_CURRENT_SOURCE_DIR};--cpp;--filename-suffix;.fbs;--filename-ext;hpp;--gen-object-api")

add_custom_target(prime_fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include common/resource.hpp
  --cpp-include graphics/texture.hpp
  --cpp-include graphics/glenum.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include debug.fbs.hpp
  --cpp-include common/resource.hpp
  --cpp-include common/aabb.hpp
  --cpp-include common/transform.hpp
  --cpp-include graphics/vertex.hpp
  --cpp-include graphics/vertex_type.hpp
  --cpp-include graphics/glenum.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/vertex.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include graphics/program.hpp
  --cpp-include graphics/glenum.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/program.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include debug.fbs.hpp
  --cpp-include program.fbs.hpp
  --cpp-include graphics/model_single.hpp
  --cpp-include model_common.fbs.hpp
  --cpp-include resource_var.fbs.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_single.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include common/resource.hpp
  --cpp-include graphics/glenum.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_common.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include common/resource.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/resource_var.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN};--gen-object-api
  ${CMAKE_CURRENT_SOURCE_DIR}/common/debug.fbs
COMMAND flatbuffers::flatc ${FLATC_REFLECT}
  ${CMAKE_CURRENT_SOURCE_DIR}/common/debug.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/resource_var.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/aabb.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/math.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/resource.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/transform.idl.fbs

  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/vertex.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/program.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_single.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_common.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glenum.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture_flags.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/vertex_type.idl.fbs
COMMAND flatbuffers::flatc ${FLATC_NATIVE}
  ${CMAKE_CURRENT_SOURCE_DIR}/common/debug.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/resource_var.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/aabb.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/math.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/resource.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/transform.idl.fbs

  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/vertex.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/program.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_single.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_common.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/glenum.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture_flags.idl.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/vertex_type.idl.fbs
)
