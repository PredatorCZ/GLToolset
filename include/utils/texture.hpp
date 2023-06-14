#include "common/resource.hpp"
#include "graphics/texture.hpp"

namespace prime::utils {
inline common::ResourceHash RedirectTexture(common::ResourceHash tex,
                                            size_t index) {
  uint32 classHash = [&] {
    switch (index) {
    case 0:
      return common::GetClassHash<graphics::TextureStream<0>>();
    case 1:
      return common::GetClassHash<graphics::TextureStream<1>>();
    case 2:
      return common::GetClassHash<graphics::TextureStream<2>>();
    case 3:
      return common::GetClassHash<graphics::TextureStream<3>>();
    }
  }();

  return common::ResourceHash{tex.name, classHash};
}
} // namespace prime::utils
