dofile("utils.TextureCompiler")

utils.TextureCompiler("editor/density", {
    generateMipmaps = false,
    entries = [
        utils.TextureCompilerEntry() {
            level = 0,
            paths = ["editor/density/8192.png"],
        },
        utils.TextureCompilerEntry() {
            level = 1,
            paths = ["editor/density/4096.png"],
        },
        utils.TextureCompilerEntry() {
            level = 2,
            paths = ["editor/density/2048.png"],
        },
        utils.TextureCompilerEntry() {
            level = 3,
            paths = ["editor/density/1024.png"],
        },
        utils.TextureCompilerEntry() {
            level = 4,
            paths = ["editor/density/512.png"],
        },
        utils.TextureCompilerEntry() {
            level = 5,
            paths = ["editor/density/256.png"],
        },
        utils.TextureCompilerEntry() {
            level = 6,
            paths = ["editor/density/128.png"],
        },
        utils.TextureCompilerEntry() {
            level = 7,
            paths = ["editor/density/64.png"],
        },
        utils.TextureCompilerEntry() {
            level = 8,
            paths = ["editor/density/32.png"],
        },
        utils.TextureCompilerEntry() {
            level = 9,
            paths = ["editor/density/16.png"],
        },
        utils.TextureCompilerEntry() {
            level = 10,
            paths = ["editor/density/8.png"],
        },
    ]
})
