#pragma once
#include "common/core.hpp"

namespace prime::graphics {
struct UIFrame;
struct UIFrameT;
void Draw(UIFrame &frame);
void Fixup(UIFrame &frame, UIFrameT &native);
} // namespace prime::graphics
CLASS_RESOURCE(1, prime::graphics::UIFrame);
