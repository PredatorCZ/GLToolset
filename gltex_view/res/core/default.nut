dofile("graphics.Sampler")

graphics.Sampler("core/default", {
  props = [
      graphics.SamplerProp() {
          id = GLEnum.GL_TEXTURE_MIN_FILTER,
          value = GLEnum.GL_LINEAR_MIPMAP_NEAREST,
      }
  ]
})
