#pragma once
#include "common/array.hpp"
#include "common/variant.hpp"

namespace prime::graphics {
union SamplerProp;
struct Sampler;
} // namespace prime::graphics

HASH_CLASS(prime::graphics::SamplerProp);
CLASS_RESOURCE(1, prime::graphics::Sampler);

namespace prime::graphics {

union SamplerProp {
  common::Variant<int32, float, Color> value{};
  struct {
    uint8 pad_[6];
    uint16 id;
  };
};

struct Sampler : common::Resource<Sampler> {
  common::LocalArray16<SamplerProp> props;
};

uint32 AddSampler(uint32 hash, const Sampler &sampler);
void SetDefaultAnisotropy(uint32 newValue);
uint32 LookupSampler(uint32 hash);

}; // namespace prime::graphics
