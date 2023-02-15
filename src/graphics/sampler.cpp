#include "graphics/sampler.hpp"
#include "datas/vectors.hpp"
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
    switch (p.funcType) {
    case SamplerPropType::Int:
      glSamplerParameteri(unit, p.id, p.propInt);
      break;
    case SamplerPropType::Float:
      glSamplerParameterf(unit, p.id, p.propFloat);
      break;
    case SamplerPropType::Color: {
      Vector4 unpacked =
          reinterpret_cast<const UCVector4 *>(p.propColor)->Convert<float>() *
          (1.f / 0xff);
      glSamplerParameterfv(unit, p.id, unpacked._arr);
      break;
    }
    default:
      break;
    }
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
