#pragma once
#include "common/resource.hpp"
#include "common/transform.hpp"

namespace prime::graphics {
struct ModelSingle;

void RebuildProgram(ModelSingle &model, common::ResourceHash referee, uint32 numLights);
void Draw(const ModelSingle &model);
void UpdateTransform(ModelSingle &model, const glm::dualquat *tm,
                     const glm::vec3 *scale);
inline void UpdateTransform(ModelSingle &model, const common::Transform &tm) {
  UpdateTransform(model, &tm.tm, &tm.inflate);
}

} // namespace prime::graphics
CLASS_RESOURCE(1, prime::graphics::ModelSingle);
