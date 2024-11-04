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
#include <SDL3/SDL_process.h>

// We can emit HLSL as a destination, so let's redefine the shader format enum.
typedef enum ShaderCross_DestinationFormat {
    SHADERFORMAT_INVALID,
    SHADERFORMAT_SPIRV,
    SHADERFORMAT_DXBC,
    SHADERFORMAT_DXIL,
    SHADERFORMAT_MSL,
    SHADERFORMAT_METALLIB,
    SHADERFORMAT_HLSL,
} ShaderCross_ShaderFormat;

typedef enum ShaderCross_Platform {
    PLATFORM_METAL_MACOS,
    PLATFORM_METAL_IOS,
} ShaderCross_Platform;

bool check_for_metal_tools(void);
int compile_metallib(ShaderCross_Platform platform, const char *outputFilename);

void print_help(void)
{
    int column_width = 32;
    SDL_Log("Usage: shadercross <input> [options]");
    SDL_Log("Required options:\n");
    SDL_Log("  %-*s %s", column_width, "-s | --source <value>", "Source language format. May be inferred from the filename. Values: [SPIRV, HLSL]");
    SDL_Log("  %-*s %s", column_width, "-d | --dest <value>", "Destination format. May be inferred from the filename. Values: [SPIRV, DXBC, DXIL, MSL, METALLIB, HLSL]");
    SDL_Log("  %-*s %s", column_width, "-t | --stage <value>", "Shader stage. May be inferred from the filename. Values: [vertex, fragment, compute]");
    SDL_Log("  %-*s %s", column_width, "-e | --entrypoint <value>", "Entrypoint function name. Default: \"main\".");
    SDL_Log("  %-*s %s", column_width, "-m | --shadermodel <value>", "HLSL Shader Model. Only used with HLSL destination. Values: [5.0, 6.0]");
    SDL_Log("  %-*s %s", column_width, "-p | --platform <value>", "Target platform. Only used with METALLIB destination. Values: [macOS, iOS]");
    SDL_Log("  %-*s %s", column_width, "-o | --output <value>", "Output file.");
}

int main(int argc, char *argv[])
{
    bool sourceValid = false;
    bool destinationValid = false;
    bool stageValid = false;
    bool shaderModelValid = false; // only required for HLSL destination
    bool platformValid = false; // only required for METALLIB destination

    bool spirvSource = false;
    ShaderCross_ShaderFormat destinationFormat = SHADERFORMAT_INVALID;
    SDL_ShaderCross_ShaderStage shaderStage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    SDL_ShaderCross_ShaderModel shaderModel;
    ShaderCross_Platform platform;
    char *outputFilename = NULL;
    char *entrypointName = "main";

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
                } else if (SDL_strcasecmp(argv[i], "HLSL") == 0) {
                    destinationFormat = SHADERFORMAT_HLSL;
                    destinationValid = true;
                } else if (SDL_strcasecmp(argv[i], "METALLIB") == 0) {
                    destinationFormat = SHADERFORMAT_METALLIB;
                    destinationValid = true;
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized destination input %s, destination must be SPIRV, DXBC, DXIL, MSL, METALLIB, or HLSL!", argv[i]);
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
                if (SDL_strcmp(argv[i], "5.0") == 0) {
                    shaderModel = SDL_SHADERCROSS_SHADERMODEL_5_0;
                    shaderModelValid = true;
                } else if (SDL_strcmp(argv[i], "6.0") == 0) {
                    shaderModel = SDL_SHADERCROSS_SHADERMODEL_6_0;
                    shaderModelValid = true;
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s is not a recognized HLSL Shader Model!", argv[i]);
                    print_help();
                    return 1;
                }
            } else if (SDL_strcmp(arg, "-p") == 0 || SDL_strcmp(arg, "--platform") == 0) {
                if (i + 1 >= argc) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s requires an argument", arg);
                    print_help();
                    return 1;
                }
                i += 1;
                if (SDL_strcasecmp(argv[i], "macOS") == 0) {
                    platform = PLATFORM_METAL_MACOS;
                    platformValid = true;
                } else if (SDL_strcasecmp(argv[i], "iOS") == 0) {
                    platform = PLATFORM_METAL_IOS;
                    platformValid = true;
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s is not a recognized platform!", argv[i]);
                    print_help();
                    return 1;
                }
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
        } else if (SDL_strstr(outputFilename, ".metallib")) {
            destinationFormat = SHADERFORMAT_METALLIB;
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

    if (spirvSource) {
        switch (destinationFormat) {
            case SHADERFORMAT_DXBC: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXBCFromSPIRV(
                    fileData,
                    fileSize,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);
                SDL_WriteIO(outputIO, buffer, bytecodeSize);
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
                SDL_WriteIO(outputIO, buffer, bytecodeSize);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_MSL: {
                char *buffer = SDL_ShaderCross_TranspileMSLFromSPIRV(
                    fileData,
                    fileSize,
                    entrypointName,
                    shaderStage);
                SDL_IOprintf(outputIO, "%s", buffer);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_METALLIB: {
                if (!platformValid) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "METALLIB destination requires a target platform!");
                    print_help();
                    return 1;
                }

                if (!check_for_metal_tools())
                {
                    return 1;
                }

                // We won't generate the metallib file directly...
                SDL_CloseIO(outputIO);
                outputIO = NULL;

                // ...instead we'll send the MSL to a temp file and then compile that.
                SDL_IOStream *tempFileIO = SDL_IOFromFile("tmp.metal", "w");
                char *buffer = SDL_ShaderCross_TranspileMSLFromSPIRV(
                    fileData,
                    fileSize,
                    entrypointName,
                    shaderStage);
                SDL_IOprintf(tempFileIO, "%s", buffer);
                SDL_free(buffer);
                SDL_CloseIO(tempFileIO);

                int exitcode = compile_metallib(platform, outputFilename);
                if (exitcode != 0) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Metal library creation failed with error code %d", exitcode);
                    return 1;
                }

                break;
            }

            case SHADERFORMAT_HLSL: {
                if (!shaderModelValid) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "HLSL destination requires a shader model specification!");
                    print_help();
                    return 1;
                }

                char *buffer = SDL_ShaderCross_TranspileHLSLFromSPIRV(
                    fileData,
                    fileSize,
                    entrypointName,
                    shaderStage,
                    shaderModel);
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
        switch (destinationFormat) {
            case SHADERFORMAT_DXBC: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXBCFromHLSL(
                    fileData,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);
                SDL_WriteIO(outputIO, buffer, bytecodeSize);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_DXIL: {
                Uint8 *buffer = SDL_ShaderCross_CompileDXILFromHLSL(
                    fileData,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);
                SDL_WriteIO(outputIO, buffer, bytecodeSize);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_MSL: {
                void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    fileData,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);
                if (spirv == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "Failed to compile SPIR-V!");
                    return 1;
                }
                char *buffer = SDL_ShaderCross_TranspileMSLFromSPIRV(
                    spirv,
                    bytecodeSize,
                    entrypointName,
                    shaderStage);
                SDL_IOprintf(outputIO, "%s", buffer);
                SDL_free(spirv);
                SDL_free(buffer);
                break;
            }

            case SHADERFORMAT_METALLIB: {
                if (!platformValid) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "METALLIB destination requires a target platform!");
                    print_help();
                    return 1;
                }

                if (!check_for_metal_tools())
                {
                    return 1;
                }

                void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    fileData,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);
                if (spirv == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "Failed to compile SPIR-V!");
                    return 1;
                }

                // We won't generate the metallib file directly...
                SDL_CloseIO(outputIO);
                outputIO = NULL;

                // ...instead we'll send the MSL to a temp file and then compile that.
                SDL_IOStream *tempFileIO = SDL_IOFromFile("tmp.metal", "w");
                char *buffer = SDL_ShaderCross_TranspileMSLFromSPIRV(
                    spirv,
                    bytecodeSize,
                    entrypointName,
                    shaderStage);
                SDL_IOprintf(tempFileIO, "%s", buffer);
                SDL_free(spirv);
                SDL_free(buffer);
                SDL_CloseIO(tempFileIO);

                int exitcode = compile_metallib(platform, outputFilename);
                if (exitcode != 0) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Metal library creation failed with error code %d", exitcode);
                    return 1;
                }

                break;
            }

            case SHADERFORMAT_SPIRV: {
                Uint8 *buffer = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    fileData,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);
                SDL_WriteIO(outputIO, buffer, bytecodeSize);
                SDL_free(buffer);
                return 0;
            }

            case SHADERFORMAT_HLSL: {
                if (!shaderModelValid) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "HLSL destination requires a shader model specification!");
                    print_help();
                    return 1;
                }

                void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
                    fileData,
                    entrypointName,
                    shaderStage,
                    &bytecodeSize);

                if (spirv == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "Failed to compile HLSL to SPIRV!");
                    return 1;
                }

                char *buffer = SDL_ShaderCross_TranspileHLSLFromSPIRV(
                    spirv,
                    bytecodeSize,
                    entrypointName,
                    shaderStage,
                    shaderModel);

                SDL_IOprintf(outputIO, "%s", buffer);
                SDL_free(spirv);
                SDL_free(buffer);
                break;
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

bool check_for_metal_tools(void)
{
#if defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_WINDOWS)

#if defined(SDL_PLATFORM_MACOS)
    char *compilerName = "xcrun";
    char *cantFindMessage = "Install Xcode or the Xcode Command Line Tools.";
#else
    char *compilerName = "metal";
    char *cantFindMessage = "Install Metal Developer Tools for Windows 5.0 beta 2 or newer (https://developer.apple.com/download/all/?q=metal%20developer%20tools%20for%20windows) and add \"C:\\Program Files\\Metal Developer Tools\\bin\" to your PATH.";
#endif

    // Check for the Metal Developer Tools...
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (char*[]){ compilerName, "--help", NULL });
    SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_NULL);
    SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_NULL);
    SDL_Process *process = SDL_CreateProcessWithProperties(props);
    SDL_DestroyProperties(props);

    if (process == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s not found! %s", compilerName, cantFindMessage);
        return false;
    }

    SDL_DestroyProcess(process);
    return true;

#else
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Compiling to METALLIB is not supported on this platform!");
    return false;
#endif
}

int compile_metallib(ShaderCross_Platform platform, const char *outputFilename)
{
    const char *stdString;
    const char *minversion;
    if (platform == PLATFORM_METAL_MACOS) {
        stdString = "-std=macos-metal2.0";
        minversion = "-mmacosx-version-min=10.13";
    } else {
        stdString = "-std=ios-metal2.0";
        minversion = "-miphoneos-version-min=13.0";
    }

#if defined(SDL_PLATFORM_MACOS)
    const char* sdkString = (platform == PLATFORM_METAL_MACOS) ? "macosx" : "iphoneos";
    const char* compileToIRCommand[] = { "xcrun", "-sdk", sdkString, "metal", stdString, minversion, "-Wall", "-O3", "-c", "tmp.metal", "-o", "tmp.ir", NULL };
    const char* compileToMetallibCommand[] = { "xcrun", "-sdk", sdkString, "metallib", "tmp.ir", "-o", outputFilename, NULL };
#else
    const char* compileToIRCommand[] = { "metal", stdString, minversion, "-Wall", "-O3", "-c", "tmp.metal", "-o", "tmp.ir", NULL};
    const char* compileToMetallibCommand[] = { "metallib", "tmp.ir", "-o", outputFilename, NULL};
#endif

    int exitcode;
    SDL_Process *process = SDL_CreateProcess(compileToIRCommand, true);
    SDL_WaitProcess(process, true, &exitcode);
    SDL_RemovePath("tmp.metal");
    if (exitcode != 0) {
        return exitcode;
    }
    SDL_DestroyProcess(process);

    process = SDL_CreateProcess(compileToMetallibCommand, true);
    SDL_WaitProcess(process, true, &exitcode);
    SDL_RemovePath("tmp.ir");
    if (exitcode != 0) {
        return exitcode;
    }
    SDL_DestroyProcess(process);
    return 0;
}
