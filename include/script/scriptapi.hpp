#pragma once

namespace prime::common {
struct ResourcePath;
}

namespace prime::script {
    void CompileScript(const common::ResourcePath &path);
    void AutogenerateScriptClasses();
}
