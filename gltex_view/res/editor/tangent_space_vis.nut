dofile("graphics.ModelSingle")

graphics.ModelSingle("editor/tangent_space_vis", {
    program = graphics.Program() {
        proto = graphics.LegacyProgram() {
            stages = [
                graphics.StageObject() {
                    resource="basics/ts_normal",
                    type=GLEnum.GL_VERTEX_SHADER,
                },
                graphics.StageObject() {
                    resource="basics/ts_normal",
                    type=GLEnum.GL_FRAGMENT_SHADER,
                },
                graphics.StageObject() {
                    resource="basics/ts_normal",
                    type=GLEnum.GL_GEOMETRY_SHADER,
                }
            ]
        }
    }

    uniformValues = [
        graphics.UniformValue() {
            name = "magnitude"
        }
    ]

    vertexArray = "editor/cube_vertex"
})
