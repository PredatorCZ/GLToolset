#include "utils/shader_preprocessor.hpp"
#include "graphics/program.hpp"
#include "simplecpp.h"
#include "spike/except.hpp"
#include "spike/master_printer.hpp"
#include <fstream>
#include <sstream>

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_GEOMETRY_SHADER 0x8DD9

static std::map<uint32, std::set<prime::common::ResourceHash>> REFERENCES;

namespace prime::graphics {
class StageObject;
}

HASH_CLASS(prime::graphics::StageObject);

namespace prime::utils {
std::string PreProcess(common::ResourceHash object, simplecpp::DUI &dui) {
  simplecpp::OutputList outputList;
  std::vector<std::string> files;
  auto &res = common::LoadResource(object);
  std::stringstream iStr(res.buffer);
  simplecpp::TokenList rawtokens(iStr, files, {}, &outputList);
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

  const std::string includePath = dui.includePaths.front();
  REFERENCES[object.name].emplace(
      common::MakeHash<graphics::StageObject>(object.name));

  for (auto &[path_, data] : included) {
    std::string_view path(path_);
    path.remove_prefix(includePath.size());
    const common::ResourceHash pathHash(common::MakeHash<char>(path));
    REFERENCES[pathHash.name].emplace(
        common::MakeHash<graphics::StageObject>(object.name));
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

std::string PreprocessShader(JenHash3 object, uint16 target,
                             std::span<std::string_view> definitions) {
  common::ResourceHash resource(object);
  simplecpp::DUI dui;
  dui.defines.emplace_back("SHADER");

  switch (target) {
  case GL_VERTEX_SHADER:
    dui.defines.emplace_back("VERTEX");
    dui.includes.emplace_back(
        common::FindResource<graphics::VertexSource>("common/common")
            .AbsPath());
    resource.type = common::GetClassHash<graphics::VertexSource>();
    break;
  case GL_FRAGMENT_SHADER:
    dui.defines.emplace_back("FRAGMENT");
    dui.includes.emplace_back(
        common::FindResource<graphics::FragmentSource>("common/common")
            .AbsPath());
    resource.type = common::GetClassHash<graphics::FragmentSource>();
    break;
  case GL_GEOMETRY_SHADER:
    dui.defines.emplace_back("GEOMETRY");
    resource.type = common::GetClassHash<graphics::GeometrySource>();
    break;
  default:
    throw std::runtime_error("Invalid target");
  }

  for (auto d : definitions) {
    dui.defines.emplace_back(d);
  }

  dui.includePaths.emplace_back(common::FindResource(resource).workingDir);

  return PreProcess(resource, dui);
}

class ShaderPreprocessor;
} // namespace prime::utils

HASH_CLASS(prime::utils::ShaderPreprocessor)

template <> class prime::common::InvokeGuard<prime::utils::ShaderPreprocessor> {
  static inline const bool data =
      prime::common::AddResourceHandle<prime::utils::ShaderPreprocessor>({
          .Process = nullptr,
          .Delete = nullptr,
          .Update =
              [](ResourceHash hash) {
                if (hash.type > 0) {
                  return;
                }

                if (REFERENCES.contains(hash.name)) {
                  common::LoadResource(common::MakeHash<char>(hash.name), true);
                  auto &refs = REFERENCES.at(hash.name);
                  for (auto &ref : refs) {
                    GetClassHandle(ref.type)
                        .Success([&](const ResourceHandle *item) {
                          item->Update(ref);
                        })
                        .Unused();
                  }
                } else {
                  GetClassHandle(common::GetClassHash<graphics::StageObject>())
                      .Success([&](const ResourceHandle *item) {
                        item->Update(
                            common::MakeHash<graphics::StageObject>(hash.name));
                      })
                      .Unused();
                }
              },
      });
};
