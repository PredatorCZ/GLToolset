#include "utils/reflect.hpp"
#include <flatbuffers/minireflect.h>
#include <map>
#include <span>

namespace prime::utils {
std::span<const TypeInfo> GetGraphicsTables();
} // namespace prime::utils

namespace {
static const auto TABLES = [] {
  std::map<uint32, const flatbuffers::TypeTable *> tbls;

  for (auto [tbl, hash] : prime::utils::GetGraphicsTables()) {
    tbls.emplace(hash, tbl);
  }

  return tbls;
}();
}

namespace prime::utils {
std::string ToString(uint32 type, const void *data) {
  auto tbl = TABLES.at(type);

  flatbuffers::ToStringVisitor visitor("\n", true, "  ", true);
  flatbuffers::IterateFlatBuffer(reinterpret_cast<const uint8_t *>(data), tbl,
                                 &visitor);
  return visitor.s;
}
} // namespace prime::utils
