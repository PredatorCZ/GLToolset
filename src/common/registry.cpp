#include "graphics/pipeline.hpp"
#include "graphics/sampler.hpp"
#include "graphics/texture.hpp"
#include <map>
#include <string_view>

template <class C> auto MakePairEH() {
  return std::make_pair(prime::common::GetClassExtension<C>(),
                        prime::common::GetClassHash<C>());
}

template <class C> auto MakePairHE() {
  return std::make_pair(prime::common::GetClassHash<C>(),
                        prime::common::GetClassExtension<C>());
}

template <class... C> auto BuildRegistry() {
  return std::make_pair(std::map<std::string_view, uint32>{MakePairEH<C>()...},
                        std::map<uint32, std::string_view>{MakePairHE<C>()...});
}

namespace pg = prime::graphics;

static const auto REGISTRY{
    BuildRegistry<char, pg::Sampler, pg::Pipeline, pg::UniformBlockData,
                  pg::Texture, pg::TextureStream<0>, pg::TextureStream<1>,
                  pg::TextureStream<2>, pg::TextureStream<3>>()};

uint32 prime::common::GetClassFromExtension(std::string_view ext) {
  return REGISTRY.first.at(ext);
}

std::string_view prime::common::GetExtentionFromHash(uint32 hash) {
  return REGISTRY.second.at(hash);
}
