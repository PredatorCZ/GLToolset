#pragma once
#include "common/resource.hpp"

namespace simplecpp {
struct DUI;
}

namespace prime::utils {
std::string PreprocessShader(uint32 object, uint16 target,
                             std::string_view definitionBuffer);

void SetShadersSourceDir(const std::string &path);
const std::string &ShadersSourceDir();
std::string PreProcess(common::ResourceHash object, simplecpp::DUI &dui);

} // namespace prime::utils
