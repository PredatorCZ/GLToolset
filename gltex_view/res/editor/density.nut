dofile("utils.TextureCompiler")

utils.TextureCompiler("editor/density2", {
    generateMipmaps = false,
    streamLimitCache = -1,
    entries = [
        utils.TextureCompilerEntry() {
            paths = [
                "editor/density2/8192.png",
                "editor/density2/4096.png",
                "editor/density2/2048.png",
                "editor/density2/1024.png",
                "editor/density2/512.png",
                "editor/density2/256.png",
                "editor/density2/128.png",
                "editor/density2/64.png",
                "editor/density2/32.png",
                "editor/density2/16.png",
                "editor/density2/8.png",
            ]
        },
    ]
})
