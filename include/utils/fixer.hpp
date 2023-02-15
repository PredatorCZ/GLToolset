#pragma once

namespace prime::utils {
    namespace detail {
        void FixupPtr(int64 &input) {
            uint64 addr = reinterpret_cast<uint64>(&input);
            input = addr + input;
        }
    }
}
