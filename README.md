# Silent Engine

Not maintained anymore: I've spent around 2 years making my first steps into graphics programming and writing this small engine as a practice. I've learned a lot and made a lot of mistakes that I want to fix in [my next game engine](https://github.com/Flone-dnb/nameless-engine).

Example screenshot of something:
![](screenshot.png?raw=true)

What is implemented in this engine:
- entity component system,
- custom audio engine written using XAudio2 with FX and 3D positioning,
- easy interface for working with custom compute shaders,
- easy interface for adding custom shaders (vertex/pixel),
- simple GUI (made with SpriteBatch/SpriteFont from DirectXTK),
- OBJ file import,
- simple profiler,
- frustum culling,
- instancing,
- shadow mapping,
- supported texture maps: only diffuse.

# Dependencies

- [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler)
- [DirectXTK12](https://github.com/microsoft/DirectXTK12)
- [Catch2](https://github.com/catchorg/Catch2)

# License

Silent Engine is distributed under the terms of the zLib license.