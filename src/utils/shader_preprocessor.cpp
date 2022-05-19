#include "utils/shader_preprocessor.hpp"
#include "datas/except.hpp"
#include "datas/master_printer.hpp"
#include "simplecpp.h"
#include <fstream>
#include <sstream>

namespace prime::utils {

static std::string SHADERS_SOURCE_DIR;

std::string PreProcess(const std::string &path, simplecpp::DUI &dui) {
  simplecpp::OutputList outputList;
  std::vector<std::string> files;
  std::string filename = SHADERS_SOURCE_DIR + path;
  std::ifstream f(filename);

  if (f.fail()) {
    throw es::FileNotFoundError(filename);
  }

  simplecpp::TokenList rawtokens(f, files, filename, &outputList);
  auto included = simplecpp::load(rawtokens, files, dui, &outputList);
  simplecpp::TokenList outputTokens(files);
  simplecpp::preprocess(outputTokens, rawtokens, files, included, dui,
                        &outputList);

  for (const simplecpp::Output &output : outputList) {
    es::print::Get(output.type == simplecpp::Output::WARNING
                       ? es::print::MPType::WRN
                       : es::print::MPType::ERR)
        << output.location.file() << ':' << output.location.line << ": ";
    switch (output.type) {
    case simplecpp::Output::ERROR:
    case simplecpp::Output::WARNING:
      break;
    case simplecpp::Output::MISSING_HEADER:
      es::print::Get() << "missing header: ";
      break;
    case simplecpp::Output::INCLUDE_NESTED_TOO_DEEPLY:
      es::print::Get() << "include nested too deeply: ";
      break;
    case simplecpp::Output::SYNTAX_ERROR:
      es::print::Get() << "syntax error: ";
      break;
    case simplecpp::Output::PORTABILITY_BACKSLASH:
      es::print::Get() << "portability: ";
      break;
    case simplecpp::Output::UNHANDLED_CHAR_ERROR:
      es::print::Get() << "unhandled char error: ";
      break;
    case simplecpp::Output::EXPLICIT_INCLUDE_NOT_FOUND:
      es::print::Get() << "explicit include not found: ";
      break;
    }
    es::print::Get() << output.msg;
    es::print::FlushAll();
  }

  std::ostringstream ret;
  ret << "#version 330 core";

  for (const simplecpp::Token *tok = outputTokens.cfront(); tok;
       tok = tok->next) {
    if (tok->previous && tok->location.sameline(tok->previous->location)) {
      int thisOP = tok->op ? tok->op : tok->str()[0];
      int prevOP =
          tok->previous->op ? tok->previous->op : tok->previous->str()[0];

      if (std::isalnum(thisOP) && std::isalnum(prevOP)) {
        ret << ' ';
      }
    } else {
      ret << '\n';
    }

    ret << tok->str();
  }

  simplecpp::cleanup(included);

  return ret.str();
}

common::ResourceData PreprocessShaderSource(VertexShaderFeatures features,
                                            const std::string &path) {
  uint32 objectHash = JenkinsHash3_(path, features.Seed());
  simplecpp::DUI dui;
  dui.defines.emplace_back("SHADER");
  dui.defines.emplace_back("VERTEX");
  char tmpBuffer[0x20]{};

  {
    static const char fmt[] = "VS_NUMBONES=%u";
    snprintf(tmpBuffer, sizeof(tmpBuffer), fmt, features.numBones);
    dui.defines.emplace_back(tmpBuffer);
  }

  {
    static const char fmt[] = "VS_NUMUVS2=%u";
    snprintf(tmpBuffer, sizeof(tmpBuffer), fmt, features.numVec2UVs);
    dui.defines.emplace_back(tmpBuffer);
  }

  {
    static const char fmt[] = "VS_NUMUVS4=%u";
    snprintf(tmpBuffer, sizeof(tmpBuffer), fmt, features.numVec4UVs);
    dui.defines.emplace_back(tmpBuffer);
  }

  {
    static const char fmt[] = "NUM_LIGHTS=%u";
    snprintf(tmpBuffer, sizeof(tmpBuffer), fmt, features.numLights);
    dui.defines.emplace_back(tmpBuffer);

    if (features.numLights) {
      dui.includes.emplace_back(SHADERS_SOURCE_DIR + "common/light_omni.vert");
    }
  }

  if (features.useInstances) {
    dui.defines.emplace_back("VS_INSTANCED");
  }

  {
    static const char fmt[] = "VS_POSTYPE=%s";
    const char *posType = nullptr;

    switch (features.posType) {
    case VSPosType::Vec3:
      posType = "vec3";
      break;
    case VSPosType::Vec4:
      posType = "vec4";
      break;
    case VSPosType::IVec3:
      posType = "ivec3";
      break;
    case VSPosType::IVec4:
      posType = "ivec4";
      break;
    default:
      break;
    }

    snprintf(tmpBuffer, sizeof(tmpBuffer), fmt, posType);
    dui.defines.emplace_back(tmpBuffer);
  }

  switch (features.tangentSpace) {
  case VSTSFeat::Normal:
    dui.defines.emplace_back("TS_NORMAL_ONLY");
    break;
  case VSTSFeat::Matrix:
    dui.defines.emplace_back("TS_MATRIX");
    break;
  case VSTSFeat::Quat:
    dui.defines.emplace_back("TS_QUAT");
    break;
  default:
    break;
  }

  if (features.tangentSpace != VSTSFeat::None) {
    dui.includes.emplace_back(SHADERS_SOURCE_DIR + "common/ts_normal.vert");
  }

  dui.includes.emplace_back(SHADERS_SOURCE_DIR + "common/common.vert");

  return {common::MakeHash<char>(objectHash), PreProcess(path, dui)};
}

common::ResourceData PreprocessShaderSource(FragmentShaderFeatures features,
                                            const std::string &path) {
  uint32 objectHash = JenkinsHash3_(path, features.Seed());
  simplecpp::DUI dui;
  dui.defines.emplace_back("SHADER");
  dui.defines.emplace_back("FRAGMENT");
  char tmpBuffer[0x20]{};

  {
    static const char fmt[] = "NUM_LIGHTS=%u";
    snprintf(tmpBuffer, sizeof(tmpBuffer), fmt, features.numLights);
    dui.defines.emplace_back(tmpBuffer);

    if (features.numLights) {
      dui.includes.emplace_back(SHADERS_SOURCE_DIR + "common/light_omni.frag");
    }
  }

  if (!features.signedNormal) {
    dui.defines.emplace_back("TS_NORMAL_UNORM");
  }

  if (features.deriveZNormal) {
    dui.defines.emplace_back("TS_NORMAL_DERIVE_Z");
  }

  dui.includes.emplace_back(SHADERS_SOURCE_DIR + "common/ts_normal.frag");

  return {common::MakeHash<char>(objectHash), PreProcess(path, dui)};
}

void SetShadersSourceDir(const std::string &path) { SHADERS_SOURCE_DIR = path; }
const std::string &ShadersSourceDir() { return SHADERS_SOURCE_DIR; }

uint32 VertexShaderFeatures::Seed() const {
  uint32 objectHashSeed =
      JenkinsHash3_({reinterpret_cast<const char *>(this), sizeof(*this)});
  return objectHashSeed;
}

uint32 FragmentShaderFeatures::Seed() const {
  uint32 objectHashSeed =
      JenkinsHash3_({reinterpret_cast<const char *>(this), sizeof(*this)});
  return objectHashSeed;
}

} // namespace prime::utils
