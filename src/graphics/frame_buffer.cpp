#include "graphics/frame_buffer.hpp"
#include <GL/glew.h>

namespace prime::graphics {
void FrameBufferTexture::Init() {
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void FrameBufferTexture::Resize(uint32 width, uint32 height) {
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type,
               NULL);
}

FrameBuffer CreateFrameBuffer(uint32 width, uint32 height) {
  FrameBuffer retVal{};
  glGenFramebuffers(1, &retVal.bufferId);
  glBindFramebuffer(GL_FRAMEBUFFER, retVal.bufferId);
  glGenRenderbuffers(1, &retVal.renderBufferId);
  retVal.Resize(width, height);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, retVal.renderBufferId);

  return retVal;
}

void FrameBuffer::Finalize() {
  std::vector<uint32> enabledSlots;

  for (auto &r : outTextures) {
    enabledSlots.push_back(GL_COLOR_ATTACHMENT0 + uint32(r.texturetype));
    glFramebufferTexture2D(GL_FRAMEBUFFER, enabledSlots.back(), GL_TEXTURE_2D,
                           r.id, 0);
  }

  glDrawBuffers(enabledSlots.size(), enabledSlots.data());
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::Resize(uint32 width_, uint32 height_) {
  width = width_;
  height = height_;

  for (auto &t : outTextures) {
    t.Resize(width, height);
  }

  glBindRenderbuffer(GL_RENDERBUFFER, renderBufferId);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}

void FrameBuffer::AddTexture(FrameBufferTextureType type) {
  switch (type) {
  case FrameBufferTextureType::Color: {
    FrameBufferTexture mainTexture{
        0, type, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,
    };
    mainTexture.Init();
    mainTexture.Resize(width, height);
    outTextures.emplace_back(mainTexture);
    break;
  }

  case FrameBufferTextureType::Glow: {
    FrameBufferTexture mainTexture{
        0, type, GL_RGBA16F, GL_RGBA, GL_FLOAT,
    };
    mainTexture.Init();
    mainTexture.Resize(width, height);
    outTextures.emplace_back(mainTexture);
    break;
  }

  default:
    break;
  }
}

uint32 FrameBuffer::FindTexture(FrameBufferTextureType type) const {
  for (auto &t : outTextures) {
    if (t.texturetype == type) {
      return t.id;
    }
  }
  return 0;
}

} // namespace prime::graphics
