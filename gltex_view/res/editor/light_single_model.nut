dofile("graphics.ModelSingle")

graphics.ModelSingle("editor/light_single_model", {
    program = graphics.Program() {
        proto = graphics.LegacyProgram() {
            stages = [
                graphics.StageObject() {
                    resource="basics/simple_sprite",
                    type=GLEnum.GL_VERTEX_SHADER,
                },
                graphics.StageObject() {
                    resource="light/main",
                    type=GLEnum.GL_FRAGMENT_SHADER,
                }
            ]
        }
    }

    uniformValues = [
        graphics.UniformValue() {
            name = "localPos"
        },
        graphics.UniformValue() {
            name = "lightColor"
        }
    ]

    textures = [
        graphics.SampledTexture() {
            texture = "editor/light",
            slot = "smTexture",
            sampler = "core/default"
        }
    ]

    vertexArray = "editor/box_vertex"
})
