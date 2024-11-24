#pragma once
#include <memory>
#include <span>
#include <string>

struct AppContext;
struct Reflector;

namespace prime::common {
struct ResourcePath;
}

namespace prime::utils {
using ContextType = std::unique_ptr<AppContext>;
void ProcessMD2(AppContext *ctx);
std::span<std::string_view> ProcessMD2Filters();
void ProcessImage(AppContext *ctx);
Reflector *ProcessImageSettings();
std::span<std::string_view> ProcessImageFilters();
void ContextOutputPath(std::string output);
ContextType MakeContext(const std::string &filePath);

bool ConvertResource(const common::ResourcePath &path);
} // namespace prime::utils
