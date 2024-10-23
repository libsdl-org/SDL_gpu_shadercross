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

#ifndef SDL_GPU_SHADERCROSS_SPIRVCROSS
#define SDL_GPU_SHADERCROSS_SPIRVCROSS 1
#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

#ifndef SDL_GPU_SHADERCROSS_HLSL
#define SDL_GPU_SHADERCROSS_HLSL 1
#endif /* SDL_GPU_SHADERCROSS_HLSL */

#ifndef SDL_GPU_SHADERCROSS_EXPORT
#define SDL_GPU_SHADERCROSS_EXPORT
#endif

/**
 * Initializes SDL_gpu_shadercross
 *
 * \threadsafety This should only be called once, from a single thread.
 */
extern bool SDL_ShaderCross_Init(void);
/**
 * De-initializes SDL_gpu_shadercross
 *
 * \threadsafety This should only be called once, from a single thread.
 */
extern void SDL_ShaderCross_Quit(void);

#if SDL_GPU_SHADERCROSS_SPIRVCROSS
/**
 * Get the supported shader formats that SPIRV cross-compilation can output
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_GPUShaderFormat SDL_ShaderCross_GetSPIRVShaderFormats(void);

/**
 * Compile an SDL GPU shader from SPIRV code.
 *
 * \param device the SDL GPU device.
 * \param createInfo a pointer to an SDL_GPUShaderCreateInfo.
 * \returns a compiled SDL_GPUShader
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_GPUShader *SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(SDL_GPUDevice *device,
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
extern SDL_GPUComputePipeline *SDL_ShaderCross_CompileComputePipelineFromSPIRV(SDL_GPUDevice *device,
                                              const SDL_GPUComputePipelineCreateInfo *createInfo);
#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

#if SDL_GPU_SHADERCROSS_HLSL
/**
 * Get the supported shader formats that HLSL cross-compilation can output
 *
 * \threadsafety It is safe to call this function from any thread.
 */
extern SDL_GPUShaderFormat SDL_ShaderCross_GetHLSLShaderFormats(void);

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
extern SDL_GPUShader *SDL_ShaderCross_CompileGraphicsShaderFromHLSL(SDL_GPUDevice *device,
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
extern SDL_GPUComputePipeline *SDL_ShaderCross_CompileComputePipelineFromHLSL(SDL_GPUDevice *device,
                                             const SDL_GPUComputePipelineCreateInfo *createInfo,
                                             const char *hlslSource,
                                             const char *shaderProfile);
#endif /* SDL_GPU_SHADERCROSS_HLSL */

#endif /* SDL_GPU_SHADERCROSS_H */
