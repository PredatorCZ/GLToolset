#pragma once
#include "common/core.hpp"
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

using CompileFunc = common::Return<void> (*)(std::string buffer,
                                             std::string_view output);
CompileFunc GetCompileFunction(uint32 compilerClassHash);

namespace detail {
uint32 RegisterCompiler(uint32 compilerClassHash, CompileFunc func);
template <class C> class CompilerRegistryInvokeGuard;
} // namespace detail

} // namespace prime::utils

#define REGISTER_COMPILER(compilerclass, func)                                 \
  template <>                                                                  \
  class prime::utils::detail::CompilerRegistryInvokeGuard<compilerclass> {     \
    static inline const uint32 data =                                          \
        RegisterCompiler(prime::common::GetClassHash<compilerclass>(), func);  \
  }
