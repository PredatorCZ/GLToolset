#include "graphics/sampler.hpp"
#include "spike/type/vectors.hpp"
#include <GL/glew.h>
#include <map>

static std::map<uint32, uint32> samplerUnits;
static uint32 anisotropy = 1;

namespace prime::graphics {
uint32 AddSampler(uint32 hash, const Sampler &sampler) {
  ValidateClass(sampler);
  uint32 unit;
  glGenSamplers(1, &unit);
  samplerUnits.insert({hash, unit});
  glSamplerParameteri(unit, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);

  for (auto &p : sampler.props) {
    p.value.Visit([&](auto &value) {
      using value_type = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<value_type, int32>) {
        glSamplerParameteri(unit, p.id, value);
      } else if constexpr (std::is_same_v<value_type, float>) {
        glSamplerParameterf(unit, p.id, value);
      } else {
        Vector4 unpacked =
            reinterpret_cast<const UCVector4 &>(value).Convert<float>() *
            (1.f / 0xff);
        glSamplerParameterfv(unit, p.id, unpacked._arr);
      }
    });

  }

  return unit;
}

void SetDefaultAnisotropy(uint32 newValue) {
  anisotropy = newValue;

  for (auto &s : samplerUnits) {
    glSamplerParameteri(s.second, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
  }
}

uint32 LookupSampler(uint32 hash) { return samplerUnits.at(hash); }

} // namespace prime::graphics

REGISTER_CLASS(prime::graphics::Sampler);
