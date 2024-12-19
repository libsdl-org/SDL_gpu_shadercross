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

#include <SDL3_shadercross/SDL_shadercross.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_iostream.h>

// We can emit HLSL and JSON as a destination, so let's redefine the shader format enum.
typedef enum ShaderCross_DestinationFormat {
    SHADERFORMAT_INVALID,
    SHADERFORMAT_SPIRV,
    SHADERFORMAT_DXBC,
    SHADERFORMAT_DXIL,
    SHADERFORMAT_MSL,
    SHADERFORMAT_HLSL,
    SHADERFORMAT_JSON
} ShaderCross_ShaderFormat;

void print_help(void)
{
    int column_width = 32;
    SDL_Log("Usage: shadercross <input> [options]");
    SDL_Log("Required options:\n");
    SDL_Log("  %-*s %s", column_width, "-s | --source <value>", "Source language format. May be inferred from the filename. Values: [SPIRV, HLSL]");
    SDL_Log("  %-*s %s", column_width, "-d | --dest <value>", "Destination format. May be inferred from the filename. Values: [DXBC, DXIL, MSL, SPIRV, HLSL, JSON]");
    SDL_Log("  %-*s %s", column_width, "-t | --stage <value>", "Shader stage. May be inferred from the filename. Values: [vertex, fragment, compute]");
    SDL_Log("  %-*s %s", column_width, "-e | --entrypoint <value>", "Entrypoint function name. Default: \"main\".");
    SDL_Log("  %-*s %s", column_width, "-o | --output <value>", "Output file.");
    SDL_Log("Optional options:\n");
    SDL_Log("  %-*s %s", column_width, "-I | --include <value>", "HLSL include directory. Only used with HLSL source.");
    SDL_Log("  %-*s %s", column_width, "-D<value>", "HLSL define. Only used with HLSL source. Can be repeated.");
    SDL_Log("  %-*s %s", column_width, "-g | --debug", "Generate debug information when possible.");
}

void write_graphics_reflect_json(SDL_IOStream *outputIO, SDL_ShaderCross_GraphicsShaderMetadata *info)
{
    SDL_IOprintf(
        outputIO,
        "{ \"samplers\": %u, \"storage_textures\": %u, \"storage_buffers\": %u, \"uniform_buffers\": %u }\n",
        info->num_samplers,
        info->num_storage_textures,
        info->num_storage_buffers,
        info->num_uniform_buffers
    );
}

void write_compute_reflect_json(SDL_IOStream *outputIO, SDL_ShaderCross_ComputePipelineMetadata *info)
{
    SDL_IOprintf(
        outputIO,
        "{ \"samplers\": %u, \"readonly_storage_textures\": %u, \"readonly_storage_buffers\": %u, \"readwrite_storage_textures\": %u, \"readwrite_storage_buffers\": %u, \"uniform_buffers\": %u, \"threadcount_x\": %u, \"threadcount_y\": %u, \"threadcount_z\": %u }\n",
        info->num_samplers,
        info->num_readonly_storage_textures,
        info->num_readonly_storage_buffers,
        info->num_readwrite_storage_textures,
        info->num_readwrite_storage_buffers,
        info->num_uniform_buffers,
        info->threadcount_x,
        info->threadcount_y,
        info->threadcount_z
    );
}

int main(int argc, char *argv[])
{
    bool sourceValid = false;
    bool destinationValid = false;
    bool stageValid = false;

    bool spirvSource = false;
    ShaderCross_ShaderFormat destinationFormat = SHADERFORMAT_INVALID;
    SDL_ShaderCross_ShaderStage shaderStage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    char *outputFilename = NULL;
    char *entrypointName = "main";
    char *includeDir = NULL;

    char *filename = NULL;
    size_t fileSize = 0;
    void *fileData = NULL;
    bool accept_optionals = true;

    Uint32 numDefines = 0;
    char **defines = NULL;

    bool enableDebug = false;

    for (int i = 1; i < argc; i += 1) {
        char *arg = argv[i];

        if (accept_optionals && arg[0] == '-') {
            if (SDL_strcmp(arg, "-h") == 0 || SDL_strcmp(arg, "--help") == 0) {
                print_help();
                return 0;
            } else if (SDL_strcmp(arg, "-s") == 0 || SDL_strcmp(arg, "--source") == 0) {
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                if (SDL_strcasecmp(argv[i], "spirv") == 0) {
                    spirvSource = true;
                    sourceValid = true;
                } else if (SDL_strcasecmp(argv[i], "hlsl") == 0) {
                    spirvSource = false;
                    sourceValid = true;
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized source input %s, source must be SPIRV or HLSL!", argv[i]);
                    print_help();
                    return 1;
                }
            } else if (SDL_strcmp(arg, "-d") == 0 || SDL_strcmp(arg, "--dest") == 0) {
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                if (SDL_strcasecmp(argv[i], "DXBC") == 0) {
                    destinationFormat = SHADERFORMAT_DXBC;
                    destinationValid = true;
                } else if (SDL_strcasecmp(argv[i], "DXIL") == 0) {
                    destinationFormat = SHADERFORMAT_DXIL;
                    destinationValid = true;
                } else if (SDL_strcasecmp(argv[i], "MSL") == 0) {
                    destinationFormat = SHADERFORMAT_MSL;
                    destinationValid = true;
                } else if (SDL_strcasecmp(argv[i], "SPIRV") == 0) {
                    destinationFormat = SHADERFORMAT_SPIRV;
                    destinationValid = true;
                } else if (SDL_strcasecmp(argv[i], "HLSL") == 0) {
                    destinationFormat = SHADERFORMAT_HLSL;
                    destinationValid = true;
                } else if (SDL_strcasecmp(argv[i], "JSON") == 0) {
                    destinationFormat = SHADERFORMAT_JSON;
                    destinationValid = true;
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized destination input %s, destination must be DXBC, DXIL, MSL or SPIRV!", argv[i]);
                    print_help();
                    return 1;
                }
            } else if (SDL_strcmp(arg, "-t") == 0 || SDL_strcmp(arg, "--stage") == 0) {
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                if (SDL_strcasecmp(argv[i], "vertex") == 0) {
                    shaderStage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
                    stageValid = true;
                } else if (SDL_strcasecmp(argv[i], "fragment") == 0) {
                    shaderStage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
                    stageValid = true;
                } else if (SDL_strcasecmp(argv[i], "compute") == 0) {
                    shaderStage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
                    stageValid = true;
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized shader stage input %s, must be vertex, fragment, or compute.", argv[i]);
                    print_help();
                    return 1;
                }
            } else if (SDL_strcmp(arg, "-e") == 0 || SDL_strcmp(arg, "--entrypoint") == 0) {
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                entrypointName = argv[i];
            } else if (SDL_strcmp(arg, "-I") == 0 || SDL_strcmp(arg, "--include") == 0) {
                if (includeDir) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "'%s' can only be used once", arg);
                    print_help();
                    return 1;
                }
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                includeDir = argv[i];
            } else if (SDL_strcmp(arg, "-o") == 0 || SDL_strcmp(arg, "--output") == 0) {
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                outputFilename = argv[i];
            } else if (strncmp(argv[i], "-D", strlen("-D")) == 0) {
                numDefines += 1;
                defines = SDL_realloc(defines, sizeof(char *) * numDefines);
                defines[numDefines - 1] = argv[i];
            } else if (SDL_strcmp(argv[i], "-g") == 0 || SDL_strcmp(arg, "--debug") == 0) {
                enableDebug = true;
            } else if (SDL_strcmp(arg, "--") == 0) {
                accept_optionals = false;
            } else {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: Unknown argument: %s", argv[0], arg);
                print_help();
                return 1;
            }
        } else if (!filename) {
            filename = arg;
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: Unknown argument: %s", argv[0], arg);
            print_help();
            return 1;
        }
    }
    if (!filename) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: missing input path", argv[0]);
        print_help();
        return 1;
    }
    if (!outputFilename) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: missing output path", argv[0]);
        print_help();
        return 1;
    }
    fileData = SDL_LoadFile(filename, &fileSize);
    if (fileData == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid file (%s)", SDL_GetError());
        return 1;
    }

    if (!SDL_ShaderCross_Init())
    {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", "Failed to initialize shadercross!");
        return 1;
    }

    if (!sourceValid) {
        if (SDL_strstr(filename, ".spv")) {
            spirvSource = true;
        } else if (SDL_strstr(filename, ".hlsl")) {
            spirvSource = false;
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "Could not infer source format!");
            print_help();
            return 1;
        }
    }

    if (!destinationValid) {
        if (SDL_strstr(outputFilename, ".dxbc")) {
            destinationFormat = SHADERFORMAT_DXBC;
        } else if (SDL_strstr(outputFilename, ".dxil")) {
            destinationFormat = SHADERFORMAT_DXIL;
        } else if (SDL_strstr(outputFilename, ".msl")) {
            destinationFormat = SHADERFORMAT_MSL;
        } else if (SDL_strstr(outputFilename, ".spv")) {
            destinationFormat = SHADERFORMAT_SPIRV;
        } else if (SDL_strstr(outputFilename, ".hlsl")) {
            destinationFormat = SHADERFORMAT_HLSL;
        } else if (SDL_strstr(outputFilename, ".json")) {
            destinationFormat = SHADERFORMAT_JSON;
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "Could not infer destination format!");
            print_help();
            return 1;
        }
    }

    if (!stageValid) {
        if (SDL_strcasestr(filename, ".vert")) {
            shaderStage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
        } else if (SDL_strcasestr(filename, ".frag")) {
            shaderStage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
        } else if (SDL_strcasestr(filename, ".comp")) {
            shaderStage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not infer shader stage from filename!");
            print_help();
            return 1;
        }
    }

    SDL_IOStream *outputIO = SDL_IOFromFile(outputFilename, "w");

    if (outputIO == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
        return 1;
    }

    size_t bytecodeSize;
    int result = 0;

    if (spirvSource) {
        SDL_ShaderCross_SPIRV_Info spirvInfo;
        spirvInfo.bytecode = fileData;
        spirvInfo.bytecode_size = fileSize;
        spirvInfo.entrypoint = entrypointName;
        spirvInfo.shader_stage = shaderStage;
        spirvInfo.enable_debug = enableDebug;
        spirvInfo.name = filename;
        spirvInfo.props = 0;

        switch (destinationFormat) {
            case SHADERFORMAT_DXBC: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXBCFromSPIRV(
                    &spirvInfo,
                    &bytecodeSize);
                if (buffer == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile DXBC from SPIR-V: %s", SDL_GetError());
                    result = 1;
                } else {
                    SDL_WriteIO(outputIO, buffer, bytecodeSize);
                    SDL_free(buffer);
                }
                break;
            }

            case SHADERFORMAT_DXIL: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXILFromSPIRV(
                    &spirvInfo,
                    &bytecodeSize);
                if (buffer == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile DXIL from SPIR-V: %s", SDL_GetError());
                    result = 1;
                } else {
                    SDL_WriteIO(outputIO, buffer, bytecodeSize);
                    SDL_free(buffer);
                }
                break;
            }

            case SHADERFORMAT_MSL: {
                char *buffer = SDL_ShaderCross_TranspileMSLFromSPIRV(
                    &spirvInfo);
                if (buffer == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to transpile MSL from SPIR-V: %s", SDL_GetError());
                    result = 1;
                } else {
                    SDL_IOprintf(outputIO, "%s", buffer);
                    SDL_free(buffer);
                }
                break;
            }

            case SHADERFORMAT_HLSL: {
                char *buffer = SDL_ShaderCross_TranspileHLSLFromSPIRV(
                    &spirvInfo);
                if (buffer == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to transpile HLSL from SPIRV: %s", SDL_GetError());
                    result = 1;
                } else {
                    SDL_IOprintf(outputIO, "%s", buffer);
                    SDL_free(buffer);
                }
                break;
            }

            case SHADERFORMAT_SPIRV: {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Input and output are both SPIRV. Did you mean to do that?");
                result = 1;
                break;
            }

            case SHADERFORMAT_JSON: {
                if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_COMPUTE) {
                    SDL_ShaderCross_ComputePipelineMetadata info;
                    if (SDL_ShaderCross_ReflectComputeSPIRV(
                        fileData,
                        fileSize,
                        &info)) {
                        write_compute_reflect_json(outputIO, &info);
                    } else {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reflect SPIRV: %s", SDL_GetError());
                        result = 1;
                    }
                } else {
                    SDL_ShaderCross_GraphicsShaderMetadata info;
                    if (SDL_ShaderCross_ReflectGraphicsSPIRV(
                        fileData,
                        fileSize,
                        &info)) {
                        write_graphics_reflect_json(outputIO, &info);
                    } else {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reflect SPIRV: %s", SDL_GetError());
                        result = 1;
                    }
                }
                break;
            }

            case SHADERFORMAT_INVALID: {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Destination format not provided!");
                result = 1;
                break;
            }
        }
    } else {
        SDL_ShaderCross_HLSL_Info hlslInfo;
        hlslInfo.source = fileData;
        hlslInfo.entrypoint = entrypointName;
        hlslInfo.include_dir = includeDir;
        hlslInfo.defines = defines;
        hlslInfo.num_defines = numDefines;
        hlslInfo.shader_stage = shaderStage;
        hlslInfo.enable_debug = enableDebug;
        hlslInfo.name = filename;
        hlslInfo.props = 0;

        switch (destinationFormat) {
            case SHADERFORMAT_DXBC: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXBCFromHLSL(
                    &hlslInfo,
                    &bytecodeSize);
                if (buffer == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile DXBC from HLSL: %s", SDL_GetError());
                    result = 1;
                } else {
                    SDL_WriteIO(outputIO, buffer, bytecodeSize);
                    SDL_free(buffer);
                }
                break;
            }

            case SHADERFORMAT_DXIL: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXILFromHLSL(
                    &hlslInfo,
                    &bytecodeSize);
                if (buffer == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile DXIL from HLSL: %s", SDL_GetError());
                    result = 1;
                } else {
                    SDL_WriteIO(outputIO, buffer, bytecodeSize);
                    SDL_free(buffer);
                }
                break;
            }

            // TODO: Should we have TranspileMSLFromHLSL?
            case SHADERFORMAT_MSL: {
                void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    &hlslInfo,
                    &bytecodeSize);
                if (spirv == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to transpile MSL from HLSL: %s", SDL_GetError());
                    result = 1;
                } else {
                    SDL_ShaderCross_SPIRV_Info spirvInfo;
                    spirvInfo.bytecode = spirv;
                    spirvInfo.bytecode_size = bytecodeSize;
                    spirvInfo.entrypoint = entrypointName;
                    spirvInfo.shader_stage = shaderStage;
                    spirvInfo.enable_debug = enableDebug;
                    spirvInfo.props = 0;
                    char *buffer = SDL_ShaderCross_TranspileMSLFromSPIRV(
                        &spirvInfo);
                    if (buffer == NULL) {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to transpile MSL from HLSL: %s", SDL_GetError());
                        result = 1;
                    } else {
                        SDL_IOprintf(outputIO, "%s", buffer);
                        SDL_free(spirv);
                        SDL_free(buffer);
                    }
                }
                break;
            }

            case SHADERFORMAT_SPIRV: {
                Uint8 *buffer = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    &hlslInfo,
                    &bytecodeSize);
                if (buffer == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile SPIR-V From HLSL: %s", SDL_GetError());
                    result = 1;
                } else {
                    SDL_WriteIO(outputIO, buffer, bytecodeSize);
                    SDL_free(buffer);
                }
                break;
            }

            case SHADERFORMAT_HLSL: {
                void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    &hlslInfo,
                    &bytecodeSize);

                if (spirv == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile HLSL to SPIRV: %s", SDL_GetError());
                    result = 1;
                    break;
                }

                SDL_ShaderCross_SPIRV_Info spirvInfo;
                spirvInfo.bytecode = spirv;
                spirvInfo.bytecode_size = bytecodeSize;
                spirvInfo.entrypoint = entrypointName;
                spirvInfo.shader_stage = shaderStage;
                spirvInfo.enable_debug = enableDebug;
                spirvInfo.props = 0;

                char *buffer = SDL_ShaderCross_TranspileHLSLFromSPIRV(
                    &spirvInfo);

                if (buffer == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to transpile HLSL from SPIRV: %s", SDL_GetError());
                    result = 1;
                    break;
                }

                SDL_IOprintf(outputIO, "%s", buffer);
                SDL_free(spirv);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_JSON: {
                void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    &hlslInfo,
                    &bytecodeSize);

                if (spirv == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile HLSL to SPIRV: %s", SDL_GetError());
                    result = 1;
                    break;
                }

                if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_COMPUTE) {
                    SDL_ShaderCross_ComputePipelineMetadata info;
                    bool result = SDL_ShaderCross_ReflectComputeSPIRV(
                        spirv,
                        bytecodeSize,
                        &info);
                    SDL_free(spirv);

                    if (result) {
                        write_compute_reflect_json(outputIO, &info);
                    } else {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reflect SPIRV: %s", SDL_GetError());
                        result = 1;
                    }
                } else {
                    SDL_ShaderCross_GraphicsShaderMetadata info;
                    bool result = SDL_ShaderCross_ReflectGraphicsSPIRV(
                        spirv,
                        bytecodeSize,
                        &info);
                    SDL_free(spirv);

                    if (result) {
                        write_graphics_reflect_json(outputIO, &info);
                    } else {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reflect SPIRV: %s", SDL_GetError());
                        result = 1;
                    }
                }

                break;
            }

            case SHADERFORMAT_INVALID: {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Destination format not provided!");
                result = 1;
                break;
            }
        }
    }

    SDL_CloseIO(outputIO);
    SDL_free(fileData);
    SDL_free(defines);
    SDL_ShaderCross_Quit();
    return result;
}
