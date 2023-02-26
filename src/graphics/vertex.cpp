#include "graphics/vertex.hpp"
#include "datas/supercore.hpp"
#include "graphics/pipeline.hpp"
#include <GL/glew.h>
#include <map>

namespace prime::graphics {
static std::map<uint32, uint32> buffers;

void AddVertexArray(VertexArray &vtArray) {
  common::LinkResource(vtArray.pipeline);
  glGenVertexArrays(1, &vtArray.index);
  glBindVertexArray(vtArray.index);
  auto pipeline = vtArray.pipeline.resourcePtr;
  auto &locations = pipeline->locations;
  auto &attrLocations = locations.attributesLocations;

  for (auto &b : vtArray.buffers) {
    if (auto found = buffers.find(b.buffer.name); !es::IsEnd(buffers, found)) {
      glBindBuffer(b.target, found->second);
    } else {
      uint32 bId;
      glGenBuffers(1, &bId);
      glBindBuffer(b.target, bId);
      buffers.emplace(b.buffer.name, bId);
      auto &resData = common::LoadResource(b.buffer);
      glBufferData(b.target, b.size, resData.buffer.data(), b.usage);
      common::FreeResource(resData);
    }

    for (auto &a : b.attributes) {
      const GLuint location = attrLocations[uint32(a.slot)];
      if (location != 0xff) {
        glVertexAttribPointer(location, a.size, a.type, a.normalized, b.stride,
                              (void *)uintptr_t(a.offset));
        glEnableVertexAttribArray(location);
      }
    }
  }

  glBindVertexArray(0);
}
uint32 ubTransforms;
void VertexArray::BeginRender() {
  pipeline.resourcePtr->BeginRender();
  glBindVertexArray(index);
  auto &locations = pipeline->locations;
  glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubInstanceTransforms,
                   ubTransforms);
}

void VertexArray::EndRender() { glDrawElements(mode, count, type, nullptr); }

void VertexArray::SetupTransform(bool staticTransform) {
  auto &locations = pipeline->locations;
  glGenBuffers(1, &ubTransforms);
  glBindBuffer(GL_UNIFORM_BUFFER, ubTransforms);
  glBufferData(GL_UNIFORM_BUFFER, transformData.numItems, transformData.begin(),
               staticTransform ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
  glUniformBlockBinding(pipeline->program, locations.ubInstanceTransforms,
                        locations.ubInstanceTransforms);
}

} // namespace prime::graphics

template <> class prime::common::InvokeGuard<prime::graphics::VertexArray> {
  static inline const bool data =
      prime::common::AddResourceHandle<prime::graphics::VertexArray>({
          .Process =
              [](ResourceData &data) {
                auto hdr = data.As<prime::graphics::VertexArray>();
                prime::graphics::AddVertexArray(*hdr);
              },
          .Delete = nullptr,
      });
};
