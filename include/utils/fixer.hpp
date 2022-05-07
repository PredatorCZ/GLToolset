#pragma once
#include "graphics/sampler.hpp"
#include "graphics/texture.hpp"
#include "graphics/pipeline.hpp"

namespace prime::utils {
    namespace detail {
        void FixupPtr(int64 &input) {
            uint64 addr = reinterpret_cast<uint64>(&input);
            input = addr + input;
        }
    }

    void FixupClass(graphics::Texture &data) {
        detail::FixupPtr(data.entries.pointer);
    }

    void FixupClass(graphics::Sampler &data) {
        detail::FixupPtr(data.props.pointer);
    }

    void FixupClass(graphics::Pipeline &data) {
        detail::FixupPtr(data.stageObjects.pointer);
        detail::FixupPtr(data.textureUnits.pointer);
        detail::FixupPtr(data.uniformBlocks.pointer);
    }
}
