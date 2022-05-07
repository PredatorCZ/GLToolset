#pragma once
#include "common/resource.hpp"

namespace simplecpp {
struct DUI;
}

namespace prime::utils {
enum class VSTSFeat : uint8 { None, Matrix, Quat, Normal };
enum class VSPosType : uint8 {
  Vec3,
  Vec4,
  IVec3,
  IVec4,
};

struct CommonFeatures {
  uint8 numLights = 1;
};

struct VertexShaderFeatures : CommonFeatures {
  VSTSFeat tangentSpace = VSTSFeat::None;
  bool useInstances = false;
  VSPosType posType = VSPosType::Vec3;
  uint8 numBones = 1;
  uint8 numVec2UVs = 1;
  uint8 numVec4UVs = 0;

  uint32 Seed() const;
};

common::ResourceData PreprocessShaderSource(VertexShaderFeatures features,
                                            const std::string &path);

struct FragmentShaderFeatures : CommonFeatures {
  bool signedNormal = true;
  bool deriveZNormal = true;

  uint32 Seed() const;
};

common::ResourceData PreprocessShaderSource(FragmentShaderFeatures features,
                                            const std::string &path);

void SetShadersSourceDir(const std::string &path);
const std::string &ShadersSourceDir();
std::string PreProcess(const std::string &path, simplecpp::DUI &dui);

} // namespace prime::utils
