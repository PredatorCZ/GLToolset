dofile("graphics.ModelSingle")

graphics.ModelSingle("editor/texture_cube_normal", {
    program = graphics.Program() {
        proto = graphics.LegacyProgram() {
            stages = [
                graphics.StageObject() {
                    resource="basics/simple_cube",
                    type=GLEnum.GL_VERTEX_SHADER,
                },
                graphics.StageObject() {
                    resource="single_texture/main_normal",
                    type=GLEnum.GL_FRAGMENT_SHADER,
                }
            ]

            definitions = [
                "NUM_LIGHTS=1",
                "TS_TAGENT_ATTR",
                "TS_QUAT"
            ]
        }
    }

    uniformBlocks = [
        graphics.UniformBlock() {
            bufferSlot = "ubFragmentProperties",
            data = "main_uniform"
        }
    ]

    textures = [
        graphics.SampledTexture() {
            slot = "smTSNormal",
            sampler = "core/default"
        }
    ]

    vertexArray = "editor/cube_vertex"
})
