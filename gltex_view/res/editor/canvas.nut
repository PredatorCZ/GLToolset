dofile("utils.TextureCompiler")

utils.TextureCompiler("editor/canvas", {
    rgbaType = TextureCompilerRGBAType.BC1,
    entries = [
        utils.TextureCompilerEntry() {
            paths = [
                "editor/density2/canvas.gif"
            ]
        },
    ]
})
