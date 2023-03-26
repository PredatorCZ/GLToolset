#pragma once
#include "datas/internal/sc_architecture.hpp"
#include <string>

namespace flatbuffers {
struct TypeTable;
}

namespace prime::utils {
struct TypeInfo {
  const flatbuffers::TypeTable *table;
  uint32 classType;
};

std::string ToString(uint32 type, const void *data);
} // namespace prime::utils
