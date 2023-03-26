#pragma once
#include <string>
#include <memory>

struct AppContext;
struct Reflector;

namespace prime::utils {
    using ContextType = std::unique_ptr<AppContext>;
    void ProcessMD2(AppContext *ctx);
    void ProcessImage(AppContext *ctx);
    Reflector *ProcessImageSettings();
    void ContextOutputPath(std::string output);
    ContextType MakeContext(const std::string &filePath);
}
