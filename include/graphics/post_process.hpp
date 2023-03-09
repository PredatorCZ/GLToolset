#pragma once
#include "frame_buffer.hpp"
#include <map>
#include <string>
#include <vector>

namespace prime::graphics {
struct PostProcessStage {
  struct Texture {
    uint32 id;
    uint32 location;
  };

  struct Uniform {
    float data[4]{};
    uint16 location;
    uint8 numItems = 0;
  };

  std::vector<Texture> textures;
  std::map<std::string, Uniform> uniforms;
  uint32 program;

  void RenderToResult() const;
  void Resize(uint32 width, uint32 height);
};

struct PostProcess {
  uint32 bufferId;
  uint32 width;
  uint32 height;
  uint32 texture;
  std::vector<PostProcessStage> stages;

  void Resize(uint32 width_, uint32 height_);
};

PostProcessStage AddPostProcessStage(uint32 object, const FrameBuffer &canvas);
PostProcess CreatePostProcess(uint32 width, uint32 height);
} // namespace prime::graphics
