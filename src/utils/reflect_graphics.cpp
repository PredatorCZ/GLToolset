#include "reflected/model_single.fbs.hpp"
#include "reflected/texture.fbs.hpp"
#include "reflected/vertex.fbs.hpp"
#include "spike/crypto/jenkinshash.hpp"
#include "utils/reflect.hpp"
#include <span>

namespace pg = prime::graphics;

namespace prime::utils {
std::span<const TypeInfo> GetGraphicsTables() {
  static const TypeInfo GRAPHICS_TABLES[]{
      {pg::ModelSingleTypeTable(),
       JenkinsHash_("prime::graphics::ModelSingle")},
      {pg::TextureTypeTable(), JenkinsHash_("prime::graphics::Texture")},
      {pg::VertexArrayTypeTable(),
       JenkinsHash_("prime::graphics::VertexArray")},
  };

  return GRAPHICS_TABLES;
}
} // namespace prime::utils
