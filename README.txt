SDL_gpu_shadercross

This is a library for translating shaders to different formats, intended for use with SDL's GPU API.
It takes SPIRV or HLSL as the source and outputs DXBC, DXIL, SPIRV, MSL, or HLSL.

This library can perform runtime translation and conveniently returns compiled SDL GPU shader objects from HLSL or SPIRV source.
This library also provides a command line interface for offline translation of shaders.

For SPIRV translation, this library depends on SPIRV-Cross: https://github.com/KhronosGroup/SPIRV-Cross
For compiling to DXIL, dxcompiler.dll and dxil.dll are required (or your platform's equivalent).
For compiling to DXBC, d3dcompiler_47 is shipped with Windows. Other platforms require vkd3d-utils.

This library is under the zlib license, see LICENSE.txt for details.
