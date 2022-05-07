#include "common/camera.hpp"
#include <GL/glew.h>
#include <cstring>
#include <map>
#include <vector>

// TODO, use registry class
static std::map<uint32, uint32> CAMERA_INDICES;
static std::vector<prime::common::Camera> CAMERAS;
static uint32 CAMERA_ID = 0;
const static uint32 CAMERA_BINDING = 0;

namespace prime::common {
uint32 AddCamera(uint32 hash, Camera camera) {
  CAMERA_INDICES.emplace(hash, CAMERA_INDICES.size());
  CAMERAS.emplace_back(camera);

  if (!CAMERA_ID) {
    glGenBuffers(1, &CAMERA_ID);
    glBindBuffer(GL_UNIFORM_BUFFER, CAMERA_ID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Camera), NULL, GL_STREAM_DRAW);
    // glBindBufferBase(GL_UNIFORM_BUFFER, CAMERA_BINDING, CAMERA_ID);
  }

  return CAMERA_INDICES.size() - 1;
}
Camera &GetCamera(uint32 hash) { return CAMERAS.at(LookupCamera(hash)); }
uint32 LookupCamera(uint32 hash) { return CAMERA_INDICES.at(hash); }
uint32 GetUBCamera() { return CAMERA_ID; }
void SetCurrentCamera(uint32 index) {
  glBindBuffer(GL_UNIFORM_BUFFER, CAMERA_ID);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Camera), &CAMERAS.at(index));
}
} // namespace prime::common
