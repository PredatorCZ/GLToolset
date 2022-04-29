#pragma once
#include "datas/jenkinshash3.hpp"
#include "datas/reflector.hpp"

uint32 CompileShader(uint32 type, const char *const *data, size_t numChunks) {
  unsigned shaderId = glCreateShader(type);
  glShaderSource(shaderId, numChunks, data, nullptr);
  glCompileShader(shaderId);

  {
    int success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
      char infoLog[512]{};
      glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
      printf("%s\n", infoLog);
      GLint sourceLen;
      glGetShaderiv(shaderId, GL_SHADER_SOURCE_LENGTH, &sourceLen);
      std::string source;
      source.resize(sourceLen);
      glGetShaderSource(shaderId, sourceLen, &sourceLen, source.data());
      printf("%s\n", source.c_str());
    }
  }

  return shaderId;
}

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
};

struct ShaderObject {
  uint32 objectId;
  uint32 objectHash;
};

ShaderObject CompileShaderObject(VertexShaderFeatures features,
                                 const std::string &path) {
  uint32 objectHashSeed = JenkinsHash3_(
      {reinterpret_cast<const char *>(&features), sizeof(features)});
  uint32 objectHash = JenkinsHash3_(path, objectHashSeed);
  const char *dataStack[0x40]{};
  size_t currentStackIndex = 0;

  auto AddToStack = [&](const char *data) {
    dataStack[currentStackIndex++] = data;
  };

  AddToStack("#version 330 core\n#define SHADER\n#define VERTEX\n");

  static const char bonesFmt[] = "#define VS_NUMBONES %u\n";
  char bonesBuffer[0x20]{};
  snprintf(bonesBuffer, sizeof(bonesBuffer), bonesFmt, features.numBones);
  AddToStack(bonesBuffer);

  static const char uvs2Fmt[] = "#define VS_NUMUVS2 %u\n";
  char uvs2Buffer[0x20]{};
  snprintf(uvs2Buffer, sizeof(uvs2Buffer), uvs2Fmt, 1);
  AddToStack(uvs2Buffer);

  static const char uvs4Fmt[] = "#define VS_NUMUVS4 %u\n";
  char uvs4Buffer[0x20]{};
  snprintf(uvs4Buffer, sizeof(uvs4Buffer), uvs4Fmt, 0);
  AddToStack(uvs4Buffer);

  static const char lightsFmt[] = "#define NUM_LIGHTS %u\n";
  char lightsBuffer[0x20]{};
  snprintf(lightsBuffer, sizeof(lightsBuffer), lightsFmt, features.numLights);
  AddToStack(lightsBuffer);

  if (features.useInstances) {
    AddToStack("#define VS_INSTANCED\n");
  }

  AddToStack("#define VS_POSTYPE ");

  switch (features.posType) {
  case VSPosType::Vec3:
    AddToStack("vec3\n");
    break;
  case VSPosType::Vec4:
    AddToStack("vec4\n");
    break;
  case VSPosType::IVec3:
    AddToStack("ivec3\n");
    break;
  case VSPosType::IVec4:
    AddToStack("ivec4\n");
    break;
  default:
    break;
  }

  switch (features.tangentSpace) {
  case VSTSFeat::Normal:
    AddToStack("#define TS_NORMAL_ONLY\n");
    break;
  case VSTSFeat::Matrix:
    AddToStack("#define TS_MATRIX\n");
    break;
  case VSTSFeat::Quat:
    AddToStack("#define TS_QUAT\n");
    break;
  default:
    break;
  }

  static std::string vertexLib;

  if (vertexLib.empty()) {
    BinReader_t<BinCoreOpenMode::Text> rd(APP_PATH + "shaders/vertex_lib.glsl");
    rd.ReadContainer(vertexLib, rd.GetSize());
  }

  AddToStack(vertexLib.data());

  std::string program;
  BinReader_t<BinCoreOpenMode::Text> rd(APP_PATH + path);
  rd.ReadContainer(program, rd.GetSize());
  AddToStack(program.data());

  return {CompileShader(GL_VERTEX_SHADER, dataStack, currentStackIndex),
          objectHash};
}

struct FragmentShaderFeatures : CommonFeatures {
  bool signedNormal = true;
  bool deriveZNormal = true;
};

ShaderObject CompileShaderObject(FragmentShaderFeatures features,
                                 const std::string &path) {
  uint32 objectHashSeed = JenkinsHash3_(
      {reinterpret_cast<const char *>(&features), sizeof(features)});
  uint32 objectHash = JenkinsHash3_(path, objectHashSeed);
  const char *dataStack[0x40]{};
  size_t currentStackIndex = 0;

  auto AddToStack = [&](const char *data) {
    dataStack[currentStackIndex++] = data;
  };

  AddToStack("#version 330 core\n#define SHADER\n#define FRAGMENT\n");

  static const char lightsFmt[] = "#define NUM_LIGHTS %u\n";
  char lightsBuffer[0x20]{};
  snprintf(lightsBuffer, sizeof(lightsBuffer), lightsFmt, features.numLights);
  AddToStack(lightsBuffer);

  if (!features.signedNormal) {
    AddToStack("#define TS_NORMAL_UNORM\n");
  }

  if (features.deriveZNormal) {
    AddToStack("#define TS_NORMAL_DERIVE_Z\n");
  }

  static std::string pixelLib;

  if (pixelLib.empty()) {
    BinReader_t<BinCoreOpenMode::Text> rd(APP_PATH +
                                          "shaders/fragment_lib.glsl");
    rd.ReadContainer(pixelLib, rd.GetSize());
  }

  AddToStack(pixelLib.data());

  std::string program;
  BinReader_t<BinCoreOpenMode::Text> rd(APP_PATH + path);
  rd.ReadContainer(program, rd.GetSize());
  AddToStack(program.data());

  return {CompileShader(GL_FRAGMENT_SHADER, dataStack, currentStackIndex),
          objectHash};
}

enum class VertexType {
  Position,
  Tangent,
  QTangent,
  Normal,
  TexCoord20,
  TexCoord21,
  TexCoord22,
  TexCoord23,
  TexCoord40,
  TexCoord41,
  TexCoord42,
  TexCoord43,
  Transforms,
  count_,
};

struct BaseShaderLocations {
  union {
    uint8 attributesLocations[size_t(VertexType::count_)]{};
    struct {
      uint8 inPos;
      uint8 inTangent;
      uint8 inQTangent;
      uint8 inNormal;
      uint8 inTexCoord20;
      uint8 inTexCoord21;
      uint8 inTexCoord22;
      uint8 inTexCoord23;
      uint8 inTexCoord40;
      uint8 inTexCoord41;
      uint8 inTexCoord42;
      uint8 inTexCoord43;
      uint8 inTransform;
    };
  };

  uint32 ubPosition;
  uint32 ubLights;
  uint32 ubFragmentProperties;
  uint32 ubLightData;
  uint32 ubInstanceTransforms;
};

REFLECT(CLASS(BaseShaderLocations), MEMBER(inPos), MEMBER(inTangent),
        MEMBER(inQTangent), MEMBER(inNormal),
        MEMBERNAME(inTexCoord20, "inTexCoord2[0]"), MEMBER(ubPosition),
        MEMBER(ubLights), MEMBER(ubFragmentProperties), MEMBER(ubLightData),
        MEMBER(ubInstanceTransforms), MEMBER(inTransform))

BaseShaderLocations IntrospectShader(uint32 program) {
  GLint numActiveAttribs = 0;
  GLint numActiveUniformBlocks = 0;
  // GLint numActiveUniforms = 0;
  glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES,
                          &numActiveAttribs);
  glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES,
                          &numActiveUniformBlocks);
  /* glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                           &numActiveUniforms);*/

  char nameData[0x40]{};
  BaseShaderLocations retVal{};
  ReflectorWrap ref(retVal);

  for (int attrib = 0; attrib < numActiveAttribs; attrib++) {
    glGetProgramResourceName(program, GL_PROGRAM_INPUT, attrib,
                             sizeof(nameData), NULL, nameData);
    auto location = glGetAttribLocation(program, nameData);
    ref.SetReflectedValueUInt(JenHash(es::string_view(nameData)), location);
  }

  for (int ub = 0; ub < numActiveUniformBlocks; ub++) {
    glGetProgramResourceName(program, GL_UNIFORM_BLOCK, ub, sizeof(nameData),
                             NULL, nameData);
    auto location = glGetUniformBlockIndex(program, nameData);
    ref.SetReflectedValueUInt(JenHash(es::string_view(nameData)), location);
  }

  /*  for (int u = 0; u < numActiveUniforms; u++) {
      glGetProgramResourceName(program, GL_UNIFORM, u, sizeof(nameData),
                               NULL, nameData);
      auto location = glGetUniformLocation(program, nameData);
      printf("%u %s\n", location, nameData);
      ref.SetReflectedValueUInt(JenHash(es::string_view(nameData)), location);
    }
  */
  return retVal;
}
