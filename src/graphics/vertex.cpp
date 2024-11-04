#include "graphics/detail/vertex_array.hpp"
#include "spike/util/supercore.hpp"
#include <GL/glew.h>
#include <map>

namespace {
struct BufferBind {
  uint32 index;
  int32 numRefs;
};

static std::map<prime::common::ResourceHash, BufferBind> buffers;

void AddVertexArray(prime::graphics::VertexArray &vtArray) {
  if (!vtArray.buffers.numItems) {
    return;
  }

  glGenVertexArrays(1, &vtArray.index);
  glBindVertexArray(vtArray.index);

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
      glVertexAttribPointer(uint32(a.slot), a.size, a.type, a.normalized,
                            b.stride, (void *)uintptr_t(a.offset));
      glEnableVertexAttribArray(uint32(a.slot));
    }
  }

  glBindVertexArray(0);
}

void FreeVertexArray(prime::graphics::VertexArray &vtArray) {
  if (!vtArray.buffers.numItems) {
    return;
  }

  glDeleteVertexArrays(1, &vtArray.index);

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

template <> class prime::common::InvokeGuard<prime::graphics::VertexArray> {
  static inline const bool data =
      prime::common::AddResourceHandle<prime::graphics::VertexArray>({
          .Process =
              [](ResourceData &data) {
                auto hdr = data.As<graphics::VertexArray>();
                AddVertexArray(*hdr);
              },
          .Delete =
              [](ResourceData &data) {
                auto hdr = data.As<graphics::VertexArray>();
                FreeVertexArray(*hdr);
              },
      });
};

REGISTER_CLASS(prime::graphics::VertexArray);
REGISTER_CLASS(prime::graphics::VertexIndexData);
REGISTER_CLASS(prime::graphics::VertexVshData);
REGISTER_CLASS(prime::graphics::VertexPshData);
