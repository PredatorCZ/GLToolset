dofile("graphics.ModelSingle")

graphics.ModelSingle("editor/aabb_single_model", {
    program = graphics.Program() {
        proto = graphics.LegacyProgram() {
            stages = [
                graphics.StageObject() {
                    resource="basics/simple_cube",
                    type=GLEnum.GL_VERTEX_SHADER,
                },
                graphics.StageObject() {
                    resource="basics/aabb_cube",
                    type=GLEnum.GL_FRAGMENT_SHADER,
                }
            ]
        }
    }

    vertexArray = "editor/cube_vertex"
})
