:: Create output directory
if not exist %1binary mkdir %1binary

:: Compile shaders using glslc
glslc.exe -fshader-stage=vertex %1VertexShader.glsl -o %1binary\VertexShader.spv
glslc.exe -fshader-stage=fragment %1FragmentShader.glsl -o %1binary\FragmentShader.spv