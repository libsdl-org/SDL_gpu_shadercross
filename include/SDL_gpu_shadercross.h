/*
  Simple DirectMedia Layer Shader Cross Compiler
  Copyright (C) 2024 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SDL_GPU_SHADERCROSS_H
#define SDL_GPU_SHADERCROSS_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_begin_code.h>
#include <SDL3/SDL_gpu.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef SDL_GPU_SHADERCROSS_SPIRVCROSS
#define SDL_GPU_SHADERCROSS_SPIRVCROSS 1
#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

#ifndef SDL_GPU_SHADERCROSS_HLSL
#define SDL_GPU_SHADERCROSS_HLSL 1
#endif /* SDL_GPU_SHADERCROSS_HLSL */

typedef enum SDL_ShaderCross_ShaderStage
{
   SDL_SHADERCROSS_SHADERSTAGE_VERTEX,
   SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT,
   SDL_SHADERCROSS_SHADERSTAGE_COMPUTE
} SDL_ShaderCross_ShaderStage;

/**
 * Initializes SDL_gpu_shadercross
 *
 * \threadsafety This should only be called once, from a single thread.
 */
extern SDL_DECLSPEC bool SDLCALL SDL_ShaderCross_Init(void);
/**
 * De-initializes SDL_gpu_shadercross
 *
 * \threadsafety This should only be called once, from a single thread.
 */
extern SDL_DECLSPEC void SDLCALL SDL_ShaderCross_Quit(void);

#if SDL_GPU_SHADERCROSS_SPIRVCROSS
/**
 * Get the supported shader formats that SPIRV cross-compilation can output
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC SDL_GPUShaderFormat SDLCALL SDL_ShaderCross_GetSPIRVShaderFormats(void);

/**
 * Transpile to MSL code from SPIRV code.
 *
 * You must SDL_free the returned string once you are done with it.
 *
 * \param bytecode the SPIRV bytecode.
 * \param bytecodeSize the length of the SPIRV bytecode.
 * \param entrypoint the entry point function name for the shader in UTF-8.
 * \param shaderStage the shader stage to transpile the shader with.
 * \returns an SDL_malloc'd string containing MSL code.
 */
extern SDL_DECLSPEC void * SDLCALL SDL_ShaderCross_TranspileMSLFromSPIRV(
    const Uint8 *bytecode,
    size_t bytecodeSize,
    const char *entrypoint);

/**
 * Transpile to HLSL code from SPIRV code.
 *
 * You must SDL_free the returned string once you are done with it.
 *
 * \param bytecode the SPIRV bytecode.
 * \param bytecodeSize the length of the SPIRV bytecode.
 * \param entrypoint the entry point function name for the shader in UTF-8.
 * \param shaderProfile the HLSL shader profile to transpile with.
 * \returns an SDL_malloc'd string containing HLSL code.
 */
extern SDL_DECLSPEC void * SDLCALL SDL_ShaderCross_TranspileHLSLFromSPIRV(
    const Uint8 *bytecode,
    size_t bytecodeSize,
    const char *entrypoint,
    const char *shaderProfile);

/**
 * Compile DXBC bytecode from SPIRV code.
 *
 * You must SDL_free the returned buffer once you are done with it.
 *
 * \param bytecode the SPIRV bytecode.
 * \param bytecodeSize the length of the SPIRV bytecode.
 * \param entrypoint the entry point function name for the shader in UTF-8.
 * \param shaderStage the shader stage to compile the shader with.
 * \param size filled in with the bytecode buffer size.
 */
extern SDL_DECLSPEC void * SDLCALL SDL_ShaderCross_CompileDXBCFromSPIRV(
    const Uint8 *bytecode,
    size_t bytecodeSize,
    const char *entrypoint,
    SDL_ShaderCross_ShaderStage shaderStage,
    size_t *size);

/**
 * Compile DXIL bytecode from SPIRV code.
 *
 * You must SDL_free the returned buffer once you are done with it.
 *
 * \param bytecode the SPIRV bytecode.
 * \param bytecodeSize the length of the SPIRV bytecode.
 * \param entrypoint the entry point function name for the shader in UTF-8.
 * \param shaderStage the shader stage to compile the shader with.
 * \param size filled in with the bytecode buffer size.
 */
extern SDL_DECLSPEC void * SDLCALL SDL_ShaderCross_CompileDXILFromSPIRV(
    const Uint8 *bytecode,
    size_t bytecodeSize,
    const char *entrypoint,
    SDL_ShaderCross_ShaderStage shaderStage,
    size_t *size);

/**
 * Compile an SDL GPU shader from SPIRV code.
 *
 * \param device the SDL GPU device.
 * \param createInfo a pointer to an SDL_GPUShaderCreateInfo.
 * \returns a compiled SDL_GPUShader
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC SDL_GPUShader * SDLCALL SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
    SDL_GPUDevice *device,
    const SDL_GPUShaderCreateInfo *createInfo);

/**
 * Compile an SDL GPU compute pipeline from SPIRV code.
 *
 * \param device the SDL GPU device.
 * \param createInfo a pointer to an SDL_GPUComputePipelineCreateInfo.
 * \returns a compiled SDL_GPUComputePipeline
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC SDL_GPUComputePipeline * SDLCALL SDL_ShaderCross_CompileComputePipelineFromSPIRV(
    SDL_GPUDevice *device,
    const SDL_GPUComputePipelineCreateInfo *createInfo);
#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

#if SDL_GPU_SHADERCROSS_HLSL
/**
 * Get the supported shader formats that HLSL cross-compilation can output
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC SDL_GPUShaderFormat SDLCALL SDL_ShaderCross_GetHLSLShaderFormats(void);

/**
 * Compile to DXBC bytecode from HLSL code.
 *
 * You must SDL_free the returned buffer once you are done with it.
 *
 * \param hlslSource the HLSL source code for the shader.
 * \param entrypoint the entry point function name for the shader in UTF-8.
 * \param shaderProfile the shader profile to compile the shader with.
 * \param size filled in with the bytecode buffer size.
 * \returns an SDL_malloc'd buffer containing DXBC bytecode.
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC void * SDLCALL SDL_ShaderCross_CompileDXBCFromHLSL(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile,
    size_t *size);

/**
 * Compile to DXIL bytecode from HLSL code.
 *
 * You must SDL_free the returned buffer once you are done with it.
 *
 * \param hlslSource the HLSL source code for the shader.
 * \param entrypoint the entry point function name for the shader in UTF-8.
 * \param shaderProfile the shader profile to compile the shader with.
 * \param size filled in with the bytecode buffer size.
 * \returns an SDL_malloc'd buffer containing DXIL bytecode.
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC void * SDLCALL SDL_ShaderCross_CompileDXILFromHLSL(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile,
    size_t *size);

/**
 * Compile to SPIRV bytecode from HLSL code.
 *
 * You must SDL_free the returned buffer once you are done with it.
 *
 * \param hlslSource the HLSL source code for the shader.
 * \param entrypoint the entry point function name for the shader in UTF-8.
 * \param shaderProfile the shader profile to compile the shader with.
 * \param size filled in with the bytecode buffer size.
 * \returns an SDL_malloc'd buffer containing SPIRV bytecode.
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC void * SDLCALL SDL_ShaderCross_CompileSPIRVFromHLSL(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile,
    size_t *size);

/**
 * Compile an SDL GPU shader from HLSL code.
 *
 * \param device the SDL GPU device.
 * \param createInfo a pointer to an SDL_GPUShaderCreateInfo.
 * \param hlslSource the HLSL source code for the shader.
 * \param shaderProfile the shader profile to compile the shader with.
 * \returns a compiled SDL_GPUShader
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC SDL_GPUShader * SDLCALL SDL_ShaderCross_CompileGraphicsShaderFromHLSL(
    SDL_GPUDevice *device,
    const SDL_GPUShaderCreateInfo *createInfo,
    const char *hlslSource,
    const char *shaderProfile);

/**
 * Compile an SDL GPU compute pipeline from HLSL code.
 *
 * \param device the SDL GPU device.
 * \param createInfo a pointer to an SDL_GPUComputePipelineCreateInfo.
 * \param hlslSource the HLSL source code for the shader.
 * \param shaderProfile the shader profile to compile the shader with.
 * \returns a compiled SDL_GPUComputePipeline
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_DECLSPEC SDL_GPUComputePipeline * SDLCALL SDL_ShaderCross_CompileComputePipelineFromHLSL(
    SDL_GPUDevice *device,
    const SDL_GPUComputePipelineCreateInfo *createInfo,
    const char *hlslSource,
    const char *shaderProfile);
#endif /* SDL_GPU_SHADERCROSS_HLSL */

#endif /* SDL_GPU_SHADERCROSS_H */
