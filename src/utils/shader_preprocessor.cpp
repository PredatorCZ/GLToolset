#include "utils/shader_preprocessor.hpp"
#include "simplecpp.h"
#include "spike/except.hpp"
#include "spike/master_printer.hpp"
#include <fstream>
#include <sstream>

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_GEOMETRY_SHADER 0x8DD9

namespace prime::utils {

static std::string SHADERS_SOURCE_DIR;

std::string PreProcess(common::ResourceHash object, simplecpp::DUI &dui) {
  simplecpp::OutputList outputList;
  std::vector<std::string> files;
  auto &res = common::LoadResource(object);
  std::stringstream iStr(res.buffer);
  simplecpp::TokenList rawtokens(iStr, files, SHADERS_SOURCE_DIR, &outputList);
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
  ret << "#version 450 core";

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

  return std::move(ret).str();
}

std::string PreprocessShader(uint32 object, uint16 target,
                             std::span<std::string_view> definitions) {
  simplecpp::DUI dui;
  dui.defines.emplace_back("SHADER");
  dui.includePaths.emplace_back(SHADERS_SOURCE_DIR);

  switch (target) {
  case GL_VERTEX_SHADER:
    dui.defines.emplace_back("VERTEX");
    dui.includes.emplace_back(SHADERS_SOURCE_DIR + "common/common.vert");
    break;
  case GL_FRAGMENT_SHADER:
    dui.defines.emplace_back("FRAGMENT");
    dui.includes.emplace_back(SHADERS_SOURCE_DIR + "common/common.frag");
    break;
  case GL_GEOMETRY_SHADER:
    dui.defines.emplace_back("GEOMETRY");
    break;
  default:
    throw std::runtime_error("Invalid target");
  }

  for (auto d : definitions) {
    dui.defines.emplace_back(d);
  }

  return PreProcess(common::MakeHash<char>(object), dui);
}

void SetShadersSourceDir(const std::string &path) { SHADERS_SOURCE_DIR = path; }
const std::string &ShadersSourceDir() { return SHADERS_SOURCE_DIR; }

} // namespace prime::utils
