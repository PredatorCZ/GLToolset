#pragma once
#include "common/resource.hpp"
#include <span>

namespace simplecpp {
struct DUI;
}

namespace prime::utils {
std::string PreprocessShader(uint32 object, uint16 target,
                             std::span<std::string_view> definitions);
std::string PreProcess(common::ResourceHash object, simplecpp::DUI &dui);

} // namespace prime::utils
