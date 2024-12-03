#include "common/transform.hpp"
#include <span>
#include <string>

namespace prime::utils {

struct InstanceTransformSpanner {
  std::span<glm::vec4> uvTransform;
  std::span<common::Transform> transforms;

  InstanceTransformSpanner() = default;
  InstanceTransformSpanner(char *buffer, size_t numTms,
                           size_t numUvTransforms) {
    uvTransform = {reinterpret_cast<glm::vec4 *>(buffer), numUvTransforms};
    transforms = {reinterpret_cast<common::Transform *>(
                      buffer + sizeof(glm::vec4) * numUvTransforms),
                  numTms};
  }
};

struct InstanceTransformsBuilder : InstanceTransformSpanner {
  std::string buffer;

  InstanceTransformsBuilder(size_t numTms, size_t numUvTransforms) {
    buffer.resize(sizeof(glm::vec4) * numUvTransforms +
                  sizeof(common::Transform) * numTms);

    std::construct_at<InstanceTransformSpanner>(this, buffer.data(), numTms,
                                                numUvTransforms);

    for (auto &i : transforms) {
      i.tm = {{1, 0, 0, 0}, {0, 0, 0}};
      i.inflate = {1, 1, 1};
    }

    for (auto &i : uvTransform) {
      i = {0, 0, 1, 1};
    }
  }
};

} // namespace prime::utils
