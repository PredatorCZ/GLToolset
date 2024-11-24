dofile("graphics.VertexArray")

graphics.VertexArray("editor/cube_vertex", {
    count = 36,
    mode = GLDrawMode.GL_TRIANGLES
    transforms = [common.Transform() {inflate = vec3(1)}]
})
