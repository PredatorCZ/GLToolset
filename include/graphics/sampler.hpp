#pragma once
#include "common/array.hpp"

namespace prime::graphics {
struct SamplerProp;
struct Sampler;
} // namespace prime::graphics

HASH_CLASS(prime::graphics::SamplerProp);
CLASS_RESOURCE(1, prime::graphics::Sampler);

namespace prime::graphics {
enum class SamplerPropType : uint8 {
  Invalid,
  Int,
  Float,
  Color,
};

struct SamplerProp {
  uint16 id;
  SamplerPropType funcType;
  union {
    int32 propInt;
    float propFloat;
    uint8 propColor[4];
  };
};

struct Sampler : common::Resource<Sampler> {
  common::LocalArray16<SamplerProp> props;
};

uint32 AddSampler(uint32 hash, const Sampler &sampler);
void SetDefaultAnisotropy(uint32 newValue);
uint32 LookupSampler(uint32 hash);

}; // namespace prime::graphics
