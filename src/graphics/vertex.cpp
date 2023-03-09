#include "graphics/vertex.hpp"
#include "datas/supercore.hpp"
#include "graphics/pipeline.hpp"
#include <GL/glew.h>
#include <map>

namespace {
struct BufferBind {
  uint32 index;
  int32 numRefs;
};

static std::map<prime::common::ResourceHash, BufferBind> buffers;

void AddVertexArray(prime::graphics::VertexArray &vtArray) {
  prime::common::LinkResource(vtArray.pipeline);
  glGenVertexArrays(1, &vtArray.index);
  glBindVertexArray(vtArray.index);
  auto pipeline = vtArray.pipeline.resourcePtr;
  auto &locations = pipeline->locations;
  auto &attrLocations = locations.attributesLocations;

  for (auto &b : vtArray.buffers) {
    if (auto found = buffers.find(b.buffer); !es::IsEnd(buffers, found)) {
      found->second.numRefs++;
      glBindBuffer(b.target, found->second.index);
    } else {
      uint32 bId;
      glGenBuffers(1, &bId);
      glBindBuffer(b.target, bId);
      buffers.emplace(b.buffer, BufferBind{bId, 1});
      auto &resData = prime::common::LoadResource(b.buffer);
      glBufferData(b.target, b.size, resData.buffer.data(), b.usage);
      prime::common::FreeResource(resData);
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

void FreeVertexArray(prime::graphics::VertexArray &vtArray) {
  glDeleteVertexArrays(1, &vtArray.index);
  prime::common::UnlinkResource(vtArray.pipeline.resourcePtr);

  for (auto &b : vtArray.buffers) {
    if (auto found = buffers.find(b.buffer); !es::IsEnd(buffers, found)) {
      if (--found->second.numRefs < 1) {
        glDeleteBuffers(1, &found->second.index);
        buffers.erase(found);
      }
    }
  }
}
} // namespace

namespace prime::graphics {
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
                AddVertexArray(*hdr);
              },
          .Delete =
              [](ResourceData &data) {
                auto hdr = data.As<prime::graphics::VertexArray>();
                FreeVertexArray(*hdr);
              },
      });
};
