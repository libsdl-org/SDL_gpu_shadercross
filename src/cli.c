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

#include <SDL3_gpu_shadercross/SDL_gpu_shadercross.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_iostream.h>

// We can emit HLSL as a destination, so let's redefine the shader format enum.
typedef enum ShaderCross_DestinationFormat {
    SHADERFORMAT_INVALID,
    SHADERFORMAT_SPIRV,
    SHADERFORMAT_DXBC,
    SHADERFORMAT_DXIL,
    SHADERFORMAT_MSL,
    SHADERFORMAT_HLSL
} ShaderCross_ShaderFormat;

void print_help(void)
{
    int column_width = 32;
    SDL_Log("%s", "Usage: shadercross <input> [options]");
    SDL_Log("%s", "Required options:\n");
    SDL_Log("  %-*s %s", column_width, "-s | --source <value>", "Source language format. May be inferred from the filename. Values: [SPIRV, HLSL]");
    SDL_Log("  %-*s %s", column_width, "-d | --dest <value>", "Destination format. May be inferred from the filename. Values: [DXBC, DXIL, MSL, SPIRV, HLSL]");
    SDL_Log("  %-*s %s", column_width, "-t | --stage <value>", "Shader stage. May be inferred from the filename. Values: [vertex, fragment, compute]");
    SDL_Log("  %-*s %s", column_width, "-e | --entrypoint <value>", "Entrypoint function name. Default: \"main\".");
    SDL_Log("  %-*s %s", column_width, "-m | --shadermodel <value>", "HLSL Shader Model. Only used with HLSL source or destination. Values: [50, 60]");
    SDL_Log("  %-*s %s", column_width, "-o | --output <value>", "Output file.");
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
    int shaderModel = 0;

    char *filename = NULL;
    size_t fileSize = 0;
    void *fileData = NULL;
    bool accept_optionals = true;

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
            } else if (SDL_strcmp(arg, "-m") == 0 || SDL_strcmp(arg, "--model") == 0) {
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                shaderModel = SDL_atoi(argv[i]);
            } else if (SDL_strcmp(arg, "-o") == 0 || SDL_strcmp(arg, "--output") == 0) {
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                outputFilename = argv[i];
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
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "Could not infer destination format!");
            print_help();
            return 1;
        }
    }

    if (!stageValid && destinationFormat != SHADERFORMAT_MSL) {
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

    if (spirvSource) {
        switch (destinationFormat) {
            case SHADERFORMAT_DXBC: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXBCFromSPIRV(
                    fileData,
                    fileSize,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);
                for (size_t i = 0; i < bytecodeSize; i += 1) {
                    SDL_WriteU8(outputIO, buffer[i]);
                }
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_DXIL: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXILFromSPIRV(
                    fileData,
                    fileSize,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);
                for (size_t i = 0; i < bytecodeSize; i += 1) {
                    SDL_WriteU8(outputIO, buffer[i]);
                }
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_MSL: {
                char *buffer = SDL_ShaderCross_TranspileMSLFromSPIRV(
                    fileData,
                    fileSize,
                    entrypointName);
                SDL_IOprintf(outputIO, "%s", buffer);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_HLSL: {
                char *profileName;
                if (shaderModel == 50) {
                    if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_VERTEX) {
                        profileName = "vs_5_0";
                    } else if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT) {
                        profileName = "ps_5_0";
                    } else {
                        profileName = "cs_5_0";
                    }
                } else if (shaderModel == 60) {
                    if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_VERTEX) {
                        profileName = "vs_6_0";
                    } else if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT) {
                        profileName = "ps_6_0";
                    } else {
                        profileName = "cs_6_0";
                    }
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "Unrecognized shader model!");
                    print_help();
                    return 1;
                }

                char *buffer = SDL_ShaderCross_TranspileHLSLFromSPIRV(
                    fileData,
                    fileSize,
                    entrypointName,
                    profileName);
                SDL_IOprintf(outputIO, "%s", buffer);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_SPIRV: {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Input and output are both SPIRV. Did you mean to do that?");
                return 1;
            }

            case SHADERFORMAT_INVALID: {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Destination format not provided!");
                return 1;
            }
        }
    } else {
        char *profileName;
        if (shaderModel == 50) {
            if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_VERTEX) {
                profileName = "vs_5_0";
            } else if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT) {
                profileName = "ps_5_0";
            } else {
                profileName = "cs_5_0";
            }
        } else if (shaderModel == 60) {
            if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_VERTEX) {
                profileName = "vs_6_0";
            } else if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT) {
                profileName = "ps_6_0";
            } else {
                profileName = "cs_6_0";
            }
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "Unrecognized shader model!");
            print_help();
            return 1;
        }

        switch (destinationFormat) {
            case SHADERFORMAT_DXBC: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXBCFromHLSL(
                    fileData,
                    entrypointName,
                    profileName,
                    &bytecodeSize);
                for (size_t i = 0; i < bytecodeSize; i += 1) {
                    SDL_WriteU8(outputIO, buffer[i]);
                }
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_DXIL: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXILFromHLSL(
                    fileData,
                    entrypointName,
                    profileName,
                    &bytecodeSize);
                for (size_t i = 0; i < bytecodeSize; i += 1) {
                    SDL_WriteU8(outputIO, buffer[i]);
                }
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_MSL: {
                void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    fileData,
                    entrypointName,
                    profileName,
                    &bytecodeSize);
                char *buffer = SDL_ShaderCross_TranspileMSLFromSPIRV(
                    spirv,
                    bytecodeSize,
                    entrypointName);
                SDL_IOprintf(outputIO, "%s", buffer);
                SDL_free(spirv);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_SPIRV: {
                Uint8 *buffer = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    fileData,
                    entrypointName,
                    profileName,
                    &bytecodeSize);
                for (size_t i = 0; i < bytecodeSize; i += 1) {
                    SDL_WriteU8(outputIO, buffer[i]);
                }
                SDL_free(buffer);
                return 0;
            }

            case SHADERFORMAT_HLSL: {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Input and output are both HLSL. Did you mean to do that?");
                return 1;
            }

            case SHADERFORMAT_INVALID: {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Destination format not provided!");
                return 1;
            }
        }
    }

    SDL_CloseIO(outputIO);
    SDL_free(fileData);
    SDL_ShaderCross_Quit();
    return 0;
}
