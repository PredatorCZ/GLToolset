find_package(Flatbuffers REQUIRED)
set(FLATC_GEN "-o;${CMAKE_BINARY_DIR}/fbs;-I;${CMAKE_CURRENT_SOURCE_DIR};--cpp;--no-includes;--gen-mutable;--filename-suffix;.fbs;--filename-ext;hpp")

add_custom_target(prime_fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include common/resource.hpp
  --cpp-include graphics/texture.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include common/resource.hpp
  --cpp-include common/aabb.hpp
  --cpp-include common/transform.hpp
  --cpp-include graphics/vertex.hpp
  --cpp-include graphics/vertex_type.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/vertex.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include graphics/program.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/program.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include program.fbs.hpp
  --cpp-include graphics/model_single.hpp
  --cpp-include model_common.fbs.hpp
  --cpp-include resource_var.fbs.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_single.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include common/resource.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_common.fbs
COMMAND flatbuffers::flatc ${FLATC_GEN}
  --cpp-include common/resource.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/resource_var.fbs
)
