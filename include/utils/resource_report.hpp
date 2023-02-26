#include "common/resource.hpp"
#include <functional>
#include <map>

namespace prime::common {
extern std::map<ResourceHash, std::pair<std::string, ResourceData>> resources;
std::map<uint32, ResourceHandle> &Registry();

using IterFunc =
    std::function<void(std::string_view path, std::string_view className,
                       uint32 nameHash, int32 numRefs, size_t dataSize)>;

void IterateResources(IterFunc cb) {
  static const std::map<uint32, std::string_view> classNames{
      [](auto... item)
          -> std::map<uint32, std::string_view> {
        return {[](auto item) -> std::pair<uint32, std::string_view> {
          return {JenkinsHash_(item), item + 7};
        }(item)...};
      }("prime::graphics::Pipeline", "prime::graphics::Texture",
          "prime::graphics::TextureStream<0>",
          "prime::graphics::TextureStream<1>",
          "prime::graphics::TextureStream<2>",
          "prime::graphics::TextureStream<3>", "prime::graphics::Sampler",
          "prime::graphics::UniformBlockData", "prime::graphics::VertexArray",
          "prime::graphics::VertexIndexData", "prime::graphics::VertexVshData",
          "prime::graphics::VertexPshData", "prime::common::String")};

  for (auto &r : resources) {
    int numRefs = -1;
    if (r.second.second.buffer.size() && Registry().contains(r.first.type)) {
      numRefs = r.second.second.numRefs;
    }
    std::string_view cls("[not registered]");

    if (classNames.contains(r.first.type)) {
      cls = classNames.at(r.first.type);
    }

    cb(r.second.first, cls, r.first.name, numRefs,
       r.second.second.buffer.size());
  }
}

} // namespace prime::common
