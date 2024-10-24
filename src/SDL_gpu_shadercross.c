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

#include "SDL_gpu_shadercross.h"
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_log.h>

#if SDL_GPU_SHADERCROSS_HLSL

/* Win32 Type Definitions */

typedef int HRESULT;
typedef const void *LPCVOID;
typedef size_t SIZE_T;
typedef const char *LPCSTR;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef void *LPVOID;
typedef void *REFIID;

/* DXIL via DXC */

/* dxcompiler Type Definitions */
typedef int BOOL;
typedef void *REFCLSID;
typedef wchar_t *LPCWSTR;
typedef void IDxcBlobEncoding;   /* hack, unused */
typedef void IDxcBlobWide;       /* hack, unused */
typedef void IDxcIncludeHandler; /* hack, unused */

/* Dynamic Library / Linking */
#ifdef DXCOMPILER_DLL
#undef DXCOMPILER_DLL
#endif
#if defined(_WIN32)
#if defined(_GAMING_XBOX_SCARLETT)
#define DXCOMPILER_DLL "dxcompiler_xs.dll"
#elif defined(_GAMING_XBOX_XBOXONE)
#define DXCOMPILER_DLL "dxcompiler_x.dll"
#else
#define DXCOMPILER_DLL "dxcompiler.dll"
#endif
#elif defined(__APPLE__)
#define DXCOMPILER_DLL "libdxcompiler.dylib"
#else
#define DXCOMPILER_DLL "libdxcompiler.so"
#endif

#ifdef DXIL_DLL
#undef DXIL_DLL
#endif
#if defined(_WIN32)
#define DXIL_DLL "dxil.dll"
#elif defined(__APPLE__)
#define DXIL_DLL "libdxil.dylib"
#else
#define DXIL_DLL "libdxil.so"
#endif

/* Unlike vkd3d-utils, libdxcompiler.so does not use msabi */
#if !defined(_WIN32)
#define __stdcall
#endif

/* Compiler Interface, _technically_ unofficial but it's MS C++, come on */
typedef enum DXC_OUT_KIND
{
    DXC_OUT_NONE = 0,
    DXC_OUT_OBJECT = 1,
    DXC_OUT_ERRORS = 2,
    DXC_OUT_PDB = 3,
    DXC_OUT_SHADER_HASH = 4,
    DXC_OUT_DISASSEMBLY = 5,
    DXC_OUT_HLSL = 6,
    DXC_OUT_TEXT = 7,
    DXC_OUT_REFLECTION = 8,
    DXC_OUT_ROOT_SIGNATURE = 9,
    DXC_OUT_EXTRA_OUTPUTS = 10,
    DXC_OUT_REMARKS = 11,
    DXC_OUT_TIME_REPORT = 12,
    DXC_OUT_TIME_TRACE = 13,
    DXC_OUT_LAST = DXC_OUT_TIME_TRACE,
    DXC_OUT_NUM_ENUMS,
    DXC_OUT_FORCE_DWORD = 0xFFFFFFFF
} DXC_OUT_KIND;

#define DXC_CP_UTF8  65001
#define DXC_CP_UTF16 1200
#define DXC_CP_UTF32 12000
/* This is for binary, ANSI-text, or to tell the compiler to try autodetecting UTF using the BOM */
#define DXC_CP_ACP 0

typedef struct DxcBuffer
{
    LPCVOID Ptr;
    SIZE_T Size;
    UINT Encoding;
} DxcBuffer;

/* *INDENT-OFF* */ // clang-format off

static Uint8 IID_IDxcBlob[] = {
    0x08, 0xFB, 0xA5, 0x8B,
    0x95, 0x51,
    0xE2, 0x40,
    0xAC,
    0x58,
    0x0D,
    0x98,
    0x9C,
    0x3A,
    0x01,
    0x02
};
typedef struct IDxcBlob IDxcBlob;
typedef struct IDxcBlobVtbl
{
    HRESULT(__stdcall *QueryInterface)(IDxcBlob *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)(IDxcBlob *This);
    ULONG(__stdcall *Release)(IDxcBlob *This);

    LPVOID(__stdcall *GetBufferPointer)(IDxcBlob *This);
    SIZE_T(__stdcall *GetBufferSize)(IDxcBlob *This);
} IDxcBlobVtbl;
struct IDxcBlob
{
    IDxcBlobVtbl *lpVtbl;
};

static Uint8 IID_IDxcBlobUtf8[] = {
    0xC9, 0x36, 0xA6, 0x3D,
    0x71, 0xBA,
    0x24, 0x40,
    0xA3,
    0x01,
    0x30,
    0xCB,
    0xF1,
    0x25,
    0x30,
    0x5B
};
typedef struct IDxcBlobUtf8 IDxcBlobUtf8;
typedef struct IDxcBlobUtf8Vtbl
{
    HRESULT(__stdcall *QueryInterface)(IDxcBlobUtf8 *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)(IDxcBlobUtf8 *This);
    ULONG(__stdcall *Release)(IDxcBlobUtf8 *This);

    LPVOID(__stdcall *GetBufferPointer)(IDxcBlobUtf8 *This);
    SIZE_T(__stdcall *GetBufferSize)(IDxcBlobUtf8 *This);

    HRESULT(__stdcall *GetEncoding)(IDxcBlobUtf8 *This, BOOL *pKnown, Uint32 *pCodePage);

    LPCSTR(__stdcall *GetStringPointer)(IDxcBlobUtf8 *This);
    SIZE_T(__stdcall *GetStringLength)(IDxcBlobUtf8 *This);
} IDxcBlobUtf8Vtbl;
struct IDxcBlobUtf8
{
    IDxcBlobUtf8Vtbl *lpVtbl;
};

static Uint8 IID_IDxcResult[] = {
    0xDA, 0x6C, 0x34, 0x58,
    0xE7, 0xDD,
    0x97, 0x44,
    0x94,
    0x61,
    0x6F,
    0x87,
    0xAF,
    0x5E,
    0x06,
    0x59
};
typedef struct IDxcResult IDxcResult;
typedef struct IDxcResultVtbl
{
    HRESULT(__stdcall *QueryInterface)(IDxcResult *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)(IDxcResult *This);
    ULONG(__stdcall *Release)(IDxcResult *This);

    HRESULT(__stdcall *GetStatus)(IDxcResult *This, HRESULT *pStatus);
    HRESULT(__stdcall *GetResult)(IDxcResult *This, IDxcBlob **ppResult);
    HRESULT(__stdcall *GetErrorBuffer)(IDxcResult *This, IDxcBlobEncoding **ppErrors);

    BOOL(__stdcall *HasOutput)(IDxcResult *This, DXC_OUT_KIND dxcOutKind);
    HRESULT(__stdcall *GetOutput)(
        IDxcResult *This,
        DXC_OUT_KIND dxcOutKind,
        REFIID iid,
        void **ppvObject,
        IDxcBlobWide **ppOutputName
    );
    Uint32(__stdcall *GetNumOutputs)(IDxcResult *This);
    DXC_OUT_KIND(__stdcall *GetOutputByIndex)(IDxcResult *This, Uint32 Index);
    DXC_OUT_KIND(__stdcall *PrimaryOutput)(IDxcResult *This);
} IDxcResultVtbl;
struct IDxcResult
{
    IDxcResultVtbl *lpVtbl;
};

static struct
{
    Uint32 Data1;
    Uint16 Data2;
    Uint16 Data3;
    Uint8 Data4[8];
} CLSID_DxcCompiler = {
    .Data1 = 0x73e22d93,
    .Data2 = 0xe6ce,
    .Data3 = 0x47f3,
    .Data4 = { 0xb5, 0xbf, 0xf0, 0x66, 0x4f, 0x39, 0xc1, 0xb0 }
};
static Uint8 IID_IDxcCompiler3[] = {
    0x87, 0x46, 0x8B, 0x22,
    0x6A, 0x5A,
    0x30, 0x47,
    0x90,
    0x0C,
    0x97,
    0x02,
    0xB2,
    0x20,
    0x3F,
    0x54
};
typedef struct IDxcCompiler3 IDxcCompiler3;
typedef struct IDxcCompiler3Vtbl
{
    HRESULT(__stdcall *QueryInterface)(IDxcCompiler3 *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)(IDxcCompiler3 *This);
    ULONG(__stdcall *Release)(IDxcCompiler3 *This);

    HRESULT(__stdcall *Compile)(
        IDxcCompiler3 *This,
        const DxcBuffer *pSource,
        LPCWSTR *pArguments,
        Uint32 argCount,
        IDxcIncludeHandler *pIncludeHandler,
        REFIID riid,
        LPVOID *ppResult
    );

    HRESULT(__stdcall *Disassemble)(
        IDxcCompiler3 *This,
        const DxcBuffer *pObject,
        REFIID riid,
        LPVOID *ppResult
    );
} IDxcCompiler3Vtbl;
struct IDxcCompiler3
{
    const IDxcCompiler3Vtbl *lpVtbl;
};

/* *INDENT-ON* */ // clang-format on

/* DXCompiler */
static SDL_SharedObject *dxcompiler_dll = NULL;

typedef HRESULT(__stdcall *DxcCreateInstanceProc)(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv);

static DxcCreateInstanceProc SDL_DxcCreateInstance = NULL;

static void *SDL_ShaderCross_INTERNAL_CompileUsingDXC(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile,
    bool spirv,
    size_t *size) // filled in with number of bytes of returned buffer
{
    DxcBuffer source;
    IDxcResult *dxcResult;
    IDxcBlob *blob;
    IDxcBlobUtf8 *errors;
    LPCWSTR args[] = {
        (LPCWSTR)L"-E",
        (LPCWSTR)L"main", /* FIXME: convert entry point to UTF-16 */
        NULL,
        NULL,
        NULL,
        NULL
    };
    Uint32 argCount = 2;
    HRESULT ret;

    /* Non-static DxcInstance, since the functions we call on it are not thread-safe */
    IDxcCompiler3 *dxcInstance = NULL;

    SDL_DxcCreateInstance(&CLSID_DxcCompiler,
                          IID_IDxcCompiler3,
                          (void **)&dxcInstance);
    if (dxcInstance == NULL) {
        return NULL;
    }

    source.Ptr = hlslSource;
    source.Size = SDL_strlen(hlslSource) + 1;
    source.Encoding = DXC_CP_ACP;

    if (SDL_strcmp(shaderProfile, "ps_6_0") == 0) {
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"ps_6_0";
    } else if (SDL_strcmp(shaderProfile, "vs_6_0") == 0) {
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"vs_6_0";
    } else if (SDL_strcmp(shaderProfile, "cs_6_0") == 0) {
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"cs_6_0";
    }

    if (spirv) {
        args[argCount++] = (LPCWSTR)L"-spirv";
    }

#ifdef _GAMING_XBOX
    args[argCount++] = L"-D__XBOX_DISABLE_PRECOMPILE=1";
#endif

    ret = dxcInstance->lpVtbl->Compile(
        dxcInstance,
        &source,
        args,
        argCount,
        NULL,
        IID_IDxcResult,
        (void **)&dxcResult);

    if (ret < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "IDxcShaderCompiler3::Compile failed: %X",
                     ret);
        dxcInstance->lpVtbl->Release(dxcInstance);
        return NULL;
    } else if (dxcResult == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "HLSL compilation failed with no IDxcResult");
        dxcInstance->lpVtbl->Release(dxcInstance);
        return NULL;
    }

    dxcResult->lpVtbl->GetOutput(dxcResult,
                                 DXC_OUT_ERRORS,
                                 IID_IDxcBlobUtf8,
                                 (void **)&errors,
                                 NULL);
    if (errors != NULL && errors->lpVtbl->GetBufferSize(errors) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "HLSL compilation failed: %s",
                     (char *)errors->lpVtbl->GetBufferPointer(errors));
        dxcResult->lpVtbl->Release(dxcResult);
        dxcInstance->lpVtbl->Release(dxcInstance);
        return NULL;
    }

    ret = dxcResult->lpVtbl->GetOutput(dxcResult,
                                       DXC_OUT_OBJECT,
                                       IID_IDxcBlob,
                                       (void **)&blob,
                                       NULL);
    if (ret < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "IDxcBlob fetch failed");
        dxcResult->lpVtbl->Release(dxcResult);
        dxcInstance->lpVtbl->Release(dxcInstance);
        return NULL;
    }

    *size = blob->lpVtbl->GetBufferSize(blob);
    void *buffer = SDL_malloc(*size);
    SDL_memcpy(buffer, blob->lpVtbl->GetBufferPointer(blob), *size);

    dxcResult->lpVtbl->Release(dxcResult);
    dxcInstance->lpVtbl->Release(dxcInstance);

    return buffer;
}

void *SDL_ShaderCross_CompileDXILFromHLSL(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile,
    size_t *size)
{
    return SDL_ShaderCross_INTERNAL_CompileUsingDXC(
        hlslSource,
        entrypoint,
        shaderProfile,
        false,
        size);
}

void *SDL_ShaderCross_CompileSPIRVFromHLSL(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile,
    size_t *size)
{
    return SDL_ShaderCross_INTERNAL_CompileUsingDXC(
        hlslSource,
        entrypoint,
        shaderProfile,
        true,
        size);
}

static void *SDL_ShaderCross_INTERNAL_CreateShaderFromDXC(
    SDL_GPUDevice *device,
    const void *createInfo,
    const char *hlslSource,
    const char *shaderProfile,
    bool spirv)
{
    void *result;
    size_t bytecodeSize;

    void *bytecode = SDL_ShaderCross_INTERNAL_CompileUsingDXC(
        hlslSource,
        ((const SDL_GPUShaderCreateInfo *)createInfo)->entrypoint,
        shaderProfile,
        spirv,
        &bytecodeSize);

    if (bytecode == NULL) {
        return NULL;
    }

    if (shaderProfile[0] == 'c' && shaderProfile[1] == 's') {
        SDL_GPUComputePipelineCreateInfo newCreateInfo;
        newCreateInfo = *(const SDL_GPUComputePipelineCreateInfo *)createInfo;
        newCreateInfo.code = (const Uint8 *)bytecode;
        newCreateInfo.code_size = bytecodeSize;
        newCreateInfo.format = spirv ? SDL_GPU_SHADERFORMAT_SPIRV : SDL_GPU_SHADERFORMAT_DXIL;

        result = SDL_CreateGPUComputePipeline(device, &newCreateInfo);
    } else {
        SDL_GPUShaderCreateInfo newCreateInfo;
        newCreateInfo = *(const SDL_GPUShaderCreateInfo *)createInfo;
        newCreateInfo.code = (const Uint8 *)bytecode;
        newCreateInfo.code_size = bytecodeSize;
        newCreateInfo.format = spirv ? SDL_GPU_SHADERFORMAT_SPIRV : SDL_GPU_SHADERFORMAT_DXIL;

        result = SDL_CreateGPUShader(device, &newCreateInfo);
    }

    SDL_free(bytecode);
    return result;
}

/* DXBC via FXC */

/* d3dcompiler Type Definitions */
typedef void D3D_SHADER_MACRO; /* hack, unused */
typedef void ID3DInclude;      /* hack, unused */

/* Dynamic Library / Linking */
#ifdef D3DCOMPILER_DLL
#undef D3DCOMPILER_DLL
#endif
#if defined(_WIN32)
#define D3DCOMPILER_DLL "d3dcompiler_47.dll"
#elif defined(__APPLE__)
#define D3DCOMPILER_DLL "libvkd3d-utils.1.dylib"
#else
#define D3DCOMPILER_DLL "libvkd3d-utils.so.1"
#endif

/* __stdcall declaration, largely taken from vkd3d_windows.h */
#ifndef _WIN32
#ifdef __stdcall
#undef __stdcall
#endif
#if defined(__x86_64__) || defined(__arm64__)
#define __stdcall __attribute__((ms_abi))
#else
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)) || defined(__APPLE__)
#define __stdcall __attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))
#else
#define __stdcall __attribute__((__stdcall__))
#endif
#endif
#endif

/* ID3DBlob definition, used by both D3DCompiler and DXCompiler */
typedef struct ID3DBlob ID3DBlob;
typedef struct ID3DBlobVtbl
{
    HRESULT(__stdcall *QueryInterface)
    (ID3DBlob *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)
    (ID3DBlob *This);
    ULONG(__stdcall *Release)
    (ID3DBlob *This);
    LPVOID(__stdcall *GetBufferPointer)
    (ID3DBlob *This);
    SIZE_T(__stdcall *GetBufferSize)
    (ID3DBlob *This);
} ID3DBlobVtbl;
struct ID3DBlob
{
    const ID3DBlobVtbl *lpVtbl;
};
#define ID3D10Blob ID3DBlob

/* D3DCompiler */
static SDL_SharedObject *d3dcompiler_dll = NULL;

typedef HRESULT(__stdcall *pfn_D3DCompile)(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    LPCSTR pSourceName,
    const D3D_SHADER_MACRO *pDefines,
    ID3DInclude *pInclude,
    LPCSTR pEntrypoint,
    LPCSTR pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob **ppCode,
    ID3DBlob **ppErrorMsgs);

static pfn_D3DCompile SDL_D3DCompile = NULL;

static ID3DBlob *SDL_ShaderCross_INTERNAL_CompileDXBC(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile)
{
    ID3DBlob *blob;
    ID3DBlob *errorBlob;
    HRESULT ret;

    ret = SDL_D3DCompile(
        hlslSource,
        SDL_strlen(hlslSource),
        NULL,
        NULL,
        NULL,
        entrypoint,
        shaderProfile,
        0,
        0,
        &blob,
        &errorBlob);

    if (ret < 0) {
        SDL_LogError(
            SDL_LOG_CATEGORY_GPU,
            "HLSL compilation failed: %s",
            (char *)errorBlob->lpVtbl->GetBufferPointer(errorBlob));
        return NULL;
    }

    return blob;
}

// Returns raw byte buffer
void *SDL_ShaderCross_CompileDXBCFromHLSL(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile,
    size_t *size) // filled in with number of bytes of returned buffer
{
    ID3DBlob *blob = SDL_ShaderCross_INTERNAL_CompileDXBC(
        hlslSource,
        entrypoint,
        shaderProfile);

    if (blob == NULL) {
        *size = 0;
        return NULL;
    }

    *size = blob->lpVtbl->GetBufferSize(blob);
    void *buffer = SDL_malloc(*size);
    SDL_memcpy(buffer, blob->lpVtbl->GetBufferPointer(blob), *size);
    blob->lpVtbl->Release(blob);

    return buffer;
}

static void *SDL_ShaderCross_INTERNAL_CreateShaderFromDXBC(
    SDL_GPUDevice *device,
    const void *createInfo,
    const char *hlslSource,
    const char *shaderProfile)
{
    void *result;
    size_t bytecodeSize;

    void *bytecode = SDL_ShaderCross_CompileDXBCFromHLSL(
        hlslSource,
        ((const SDL_GPUShaderCreateInfo *)createInfo)->entrypoint,
        shaderProfile,
        &bytecodeSize);

    if (bytecode == NULL) {
        return NULL;
    }

    if (shaderProfile[0] == 'c' && shaderProfile[1] == 's') {
        SDL_GPUComputePipelineCreateInfo newCreateInfo;
        newCreateInfo = *(const SDL_GPUComputePipelineCreateInfo *)createInfo;
        newCreateInfo.code = (const Uint8 *)bytecode;
        newCreateInfo.code_size = bytecodeSize;
        newCreateInfo.format = SDL_GPU_SHADERFORMAT_DXBC;

        result = SDL_CreateGPUComputePipeline(device, &newCreateInfo);
    } else {
        SDL_GPUShaderCreateInfo newCreateInfo;
        newCreateInfo = *(const SDL_GPUShaderCreateInfo *)createInfo;
        newCreateInfo.code = (const Uint8 *)bytecode;
        newCreateInfo.code_size = bytecodeSize;
        newCreateInfo.format = SDL_GPU_SHADERFORMAT_DXBC;

        result = SDL_CreateGPUShader(device, &newCreateInfo);
    }

    SDL_free(bytecode);
    return result;
}

static void *SDL_ShaderCross_INTERNAL_CreateShaderFromHLSL(SDL_GPUDevice *device,
                                             const void *createInfo,
                                             const char *hlslSource,
                                             const char *shaderProfile)
{
    SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(device);
    if (format & SDL_GPU_SHADERFORMAT_DXBC) {
        return SDL_ShaderCross_INTERNAL_CreateShaderFromDXBC(device, createInfo, hlslSource, shaderProfile);
    }
    if (format & SDL_GPU_SHADERFORMAT_DXIL) {
        return SDL_ShaderCross_INTERNAL_CreateShaderFromDXC(device, createInfo, hlslSource, shaderProfile, false);
    }
    if (format & SDL_GPU_SHADERFORMAT_SPIRV) {
        return SDL_ShaderCross_INTERNAL_CreateShaderFromDXC(device, createInfo, hlslSource, shaderProfile, true);
    }

    SDL_SetError("SDL_ShaderCross_INTERNAL_CreateShaderFromHLSL: Unexpected SDL_GPUShaderFormat");
    return NULL;
}

SDL_GPU_SHADERCROSS_EXPORT SDL_GPUShader *SDL_ShaderCross_CompileGraphicsShaderFromHLSL(
    SDL_GPUDevice *device,
    const SDL_GPUShaderCreateInfo *createInfo,
    const char *hlslSource,
    const char *shaderProfile)
{
    return (SDL_GPUShader *)SDL_ShaderCross_INTERNAL_CreateShaderFromHLSL(device, createInfo, hlslSource, shaderProfile);
}

SDL_GPU_SHADERCROSS_EXPORT SDL_GPUComputePipeline *SDL_ShaderCross_CompileComputePipelineFromHLSL(
    SDL_GPUDevice *device,
    const SDL_GPUComputePipelineCreateInfo *createInfo,
    const char *hlslSource,
    const char *shaderProfile)
{
    return (SDL_GPUComputePipeline *)SDL_ShaderCross_INTERNAL_CreateShaderFromHLSL(device, createInfo, hlslSource, shaderProfile);
}

#endif /* SDL_GPU_SHADERCROSS_HLSL */

#if SDL_GPU_SHADERCROSS_SPIRVCROSS

#if !SDL_GPU_SHADERCROSS_HLSL
#error SDL_GPU_SHADERCROSS_HLSL must be enabled for SDL_GPU_SHADERCROSS_SPIRVCROSS!
#endif /* !SDL_GPU_SHADERCROSS_HLSL */

#include "spirv_cross_c.h"

#ifndef SDL_GPU_SHADERCROSS_STATIC

#ifndef SDL_GPU_SPIRV_CROSS_DLL
#if defined(_WIN32)
#define SDL_GPU_SPIRV_CROSS_DLL "spirv-cross-c-shared.dll"
#elif defined(__APPLE__)
#define SDL_GPU_SPIRV_CROSS_DLL "libspirv-cross-c-shared.0.dylib"
#else
#define SDL_GPU_SPIRV_CROSS_DLL "libspirv-cross-c-shared.so.0"
#endif
#endif /* SDL_GPU_SPIRV_CROSS_DLL */

static SDL_SharedObject *spirvcross_dll = NULL;

typedef spvc_result (*pfn_spvc_context_create)(spvc_context *context);
typedef void (*pfn_spvc_context_destroy)(spvc_context);
typedef spvc_result (*pfn_spvc_context_parse_spirv)(spvc_context, const SpvId *, size_t, spvc_parsed_ir *);
typedef spvc_result (*pfn_spvc_context_create_compiler)(spvc_context, spvc_backend, spvc_parsed_ir, spvc_capture_mode, spvc_compiler *);
typedef spvc_result (*pfn_spvc_compiler_create_compiler_options)(spvc_compiler, spvc_compiler_options *);
typedef spvc_result (*pfn_spvc_compiler_options_set_uint)(spvc_compiler_options, spvc_compiler_option, unsigned);
typedef spvc_result (*pfn_spvc_compiler_install_compiler_options)(spvc_compiler, spvc_compiler_options);
typedef spvc_result (*pfn_spvc_compiler_compile)(spvc_compiler, const char **);
typedef const char *(*pfn_spvc_context_get_last_error_string)(spvc_context);
typedef SpvExecutionModel (*pfn_spvc_compiler_get_execution_model)(spvc_compiler compiler);
typedef const char *(*pfn_spvc_compiler_get_cleansed_entry_point_name)(spvc_compiler compiler, const char *name, SpvExecutionModel model);

static pfn_spvc_context_create SDL_spvc_context_create = NULL;
static pfn_spvc_context_destroy SDL_spvc_context_destroy = NULL;
static pfn_spvc_context_parse_spirv SDL_spvc_context_parse_spirv = NULL;
static pfn_spvc_context_create_compiler SDL_spvc_context_create_compiler = NULL;
static pfn_spvc_compiler_create_compiler_options SDL_spvc_compiler_create_compiler_options = NULL;
static pfn_spvc_compiler_options_set_uint SDL_spvc_compiler_options_set_uint = NULL;
static pfn_spvc_compiler_install_compiler_options SDL_spvc_compiler_install_compiler_options = NULL;
static pfn_spvc_compiler_compile SDL_spvc_compiler_compile = NULL;
static pfn_spvc_context_get_last_error_string SDL_spvc_context_get_last_error_string = NULL;
static pfn_spvc_compiler_get_execution_model SDL_spvc_compiler_get_execution_model = NULL;
static pfn_spvc_compiler_get_cleansed_entry_point_name SDL_spvc_compiler_get_cleansed_entry_point_name = NULL;

#else /* SDL_GPU_SHADERCROSS_STATIC */

#define SDL_spvc_context_create                         spvc_context_create
#define SDL_spvc_context_destroy                        spvc_context_destroy
#define SDL_spvc_context_parse_spirv                    spvc_context_parse_spirv
#define SDL_spvc_context_create_compiler                spvc_context_create_compiler
#define SDL_spvc_compiler_create_compiler_options       spvc_compiler_create_compiler_options
#define SDL_spvc_compiler_options_set_uint              spvc_compiler_options_set_uint
#define SDL_spvc_compiler_install_compiler_options      spvc_compiler_install_compiler_options
#define SDL_spvc_compiler_compile                       spvc_compiler_compile
#define SDL_spvc_context_get_last_error_string          spvc_context_get_last_error_string
#define SDL_spvc_compiler_get_execution_model           spvc_compiler_get_execution_model
#define SDL_spvc_compiler_get_cleansed_entry_point_name spvc_compiler_get_cleansed_entry_point_name

#endif /* SDL_GPU_SHADERCROSS_STATIC */

#define SPVC_ERROR(func) \
    SDL_SetError(#func " failed: %s", SDL_spvc_context_get_last_error_string(context))

// Returns a malloc'd CreateInfo struct.
// It also malloc's the bytecode pointer and entrypoint string.
static void *SDL_ShaderCross_INTERNAL_TranslateFromSPIRV(
    SDL_GPUShaderFormat shaderFormat,
    const void *originalCreateInfo,
    bool isCompute
) {
    const SDL_GPUShaderCreateInfo *createInfo;
    spvc_result result;
    spvc_backend backend;
    unsigned shadermodel;
    spvc_context context = NULL;
    spvc_parsed_ir ir = NULL;
    spvc_compiler compiler = NULL;
    spvc_compiler_options options = NULL;
    const char *translated_source;
    const char *cleansed_entrypoint;

    if (shaderFormat == SDL_GPU_SHADERFORMAT_DXBC) {
        backend = SPVC_BACKEND_HLSL;
        shadermodel = 50;
    } else if (shaderFormat == SDL_GPU_SHADERFORMAT_DXIL) {
        backend = SPVC_BACKEND_HLSL;
        shadermodel = 60;
    } else if (shaderFormat == SDL_GPU_SHADERFORMAT_MSL) {
        backend = SPVC_BACKEND_MSL;
    } else {
        SDL_SetError("SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV: Unexpected SDL_GPUBackend");
        return NULL;
    }

    /* Create the SPIRV-Cross context */
    result = SDL_spvc_context_create(&context);
    if (result < 0) {
        SDL_SetError("spvc_context_create failed: %X", result);
        return NULL;
    }

    /* SDL_GPUShaderCreateInfo and SDL_GPUComputePipelineCreateInfo
     * share the same struct layout for their first 3 members, which
     * is all we need to transpile them!
     */
    createInfo = (const SDL_GPUShaderCreateInfo *)originalCreateInfo;

    /* Parse the SPIR-V into IR */
    result = SDL_spvc_context_parse_spirv(context, (const SpvId *)createInfo->code, createInfo->code_size / sizeof(SpvId), &ir);
    if (result < 0) {
        SPVC_ERROR(spvc_context_parse_spirv);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    /* Create the cross-compiler */
    result = SDL_spvc_context_create_compiler(context, backend, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);
    if (result < 0) {
        SPVC_ERROR(spvc_context_create_compiler);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    /* Set up the cross-compiler options */
    result = SDL_spvc_compiler_create_compiler_options(compiler, &options);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_create_compiler_options);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    if (backend == SPVC_BACKEND_HLSL) {
        SDL_spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_SHADER_MODEL, shadermodel);
        SDL_spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_NONWRITABLE_UAV_TEXTURE_AS_SRV, 1);
        SDL_spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_FLATTEN_MATRIX_VERTEX_INPUT_SEMANTICS, 1);
    }

    result = SDL_spvc_compiler_install_compiler_options(compiler, options);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_install_compiler_options);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    /* Compile to the target shader language */
    result = SDL_spvc_compiler_compile(compiler, &translated_source);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_compile);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    /* Determine the "cleansed" entrypoint name (e.g. main -> main0 on MSL) */
    cleansed_entrypoint = SDL_spvc_compiler_get_cleansed_entry_point_name(
        compiler,
        createInfo->entrypoint,
        SDL_spvc_compiler_get_execution_model(compiler));

    void *translatedCreateInfo = NULL;

    /* Copy the original create info, but with the new source code */
    if (isCompute) {
        SDL_GPUComputePipelineCreateInfo *newCreateInfo = SDL_malloc(sizeof(SDL_GPUComputePipelineCreateInfo));
        SDL_memcpy(newCreateInfo, createInfo, sizeof(SDL_GPUComputePipelineCreateInfo));

        newCreateInfo->format = shaderFormat;

        size_t cleansed_entrypoint_length = SDL_utf8strlen(cleansed_entrypoint) + 1;
        newCreateInfo->entrypoint = SDL_malloc(cleansed_entrypoint_length);
        SDL_utf8strlcpy((char *)newCreateInfo->entrypoint, cleansed_entrypoint, cleansed_entrypoint_length);

        if (shaderFormat == SDL_GPU_SHADERFORMAT_DXBC) {
            newCreateInfo->code = SDL_ShaderCross_CompileDXBCFromHLSL(
                translated_source,
                cleansed_entrypoint,
                (shadermodel == 50) ? "cs_5_0" : "cs_6_0",
                &newCreateInfo->code_size);
        } else if (shaderFormat == SDL_GPU_SHADERFORMAT_DXIL) {
            newCreateInfo->code = SDL_ShaderCross_CompileDXILFromHLSL(
                translated_source,
                cleansed_entrypoint,
                (shadermodel == 50) ? "cs_5_0" : "cs_6_0",
                &newCreateInfo->code_size);
        } else { // MSL
            newCreateInfo->code_size = SDL_strlen(translated_source) + 1;
            newCreateInfo->code = SDL_malloc(newCreateInfo->code_size);
            SDL_strlcpy((char *)newCreateInfo->code, translated_source, newCreateInfo->code_size);
        }

        translatedCreateInfo = newCreateInfo;
    } else {
        SDL_GPUShaderCreateInfo *newCreateInfo = SDL_malloc(sizeof(SDL_GPUShaderCreateInfo));
        SDL_memcpy(newCreateInfo, createInfo, sizeof(SDL_GPUShaderCreateInfo));

        newCreateInfo->format = shaderFormat;

        size_t cleansed_entrypoint_length = SDL_utf8strlen(cleansed_entrypoint) + 1;
        newCreateInfo->entrypoint = SDL_malloc(cleansed_entrypoint_length);
        SDL_utf8strlcpy((char *)newCreateInfo->entrypoint, cleansed_entrypoint, cleansed_entrypoint_length);

        const char *profile;
        if (newCreateInfo->stage == SDL_GPU_SHADERSTAGE_VERTEX) {
            profile = (shadermodel == 50) ? "vs_5_0" : "vs_6_0";
        } else {
            profile = (shadermodel == 50) ? "ps_5_0" : "ps_6_0";
        }

        if (shaderFormat == SDL_GPU_SHADERFORMAT_DXBC) {
            newCreateInfo->code = SDL_ShaderCross_CompileDXBCFromHLSL(
                translated_source,
                cleansed_entrypoint,
                profile,
                &newCreateInfo->code_size);
        } else if (shaderFormat == SDL_GPU_SHADERFORMAT_DXIL) {
            newCreateInfo->code = SDL_ShaderCross_CompileDXILFromHLSL(
                translated_source,
                cleansed_entrypoint,
                profile,
                &newCreateInfo->code_size);
        } else { // MSL
            newCreateInfo->code_size = SDL_strlen(translated_source) + 1;
            newCreateInfo->code = SDL_malloc(newCreateInfo->code_size);
            SDL_strlcpy((char *)newCreateInfo->code, translated_source, newCreateInfo->code_size);
        }

        translatedCreateInfo = newCreateInfo;
    }

    SDL_spvc_context_destroy(context);
    return translatedCreateInfo;
}

static void *SDL_ShaderCross_INTERNAL_CompileFromSPIRV(
    SDL_GPUShaderFormat format,
    const Uint8 *bytecode,
    size_t bytecodeSize,
    const char *entrypoint,
    SDL_ShaderCross_ShaderStage shaderStage,
    size_t *size)
{
    if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_COMPUTE) {
        SDL_GPUComputePipelineCreateInfo createInfo;
        createInfo.code = bytecode;
        createInfo.code_size = bytecodeSize;
        createInfo.entrypoint = entrypoint;
        createInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;

        SDL_GPUComputePipelineCreateInfo *translatedCreateInfo = (SDL_GPUComputePipelineCreateInfo *)SDL_ShaderCross_INTERNAL_TranslateFromSPIRV(
            format,
            &createInfo,
            true);

        void *result = (void *)translatedCreateInfo->code;
        *size = translatedCreateInfo->code_size;
        SDL_free((void *)translatedCreateInfo->entrypoint);
        SDL_free(translatedCreateInfo);

        return result;
    } else {
        SDL_GPUShaderCreateInfo createInfo;
        createInfo.code = bytecode;
        createInfo.code_size = bytecodeSize;
        createInfo.entrypoint = entrypoint;
        createInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        createInfo.stage = (SDL_GPUShaderStage)shaderStage;

        SDL_GPUShaderCreateInfo *translatedCreateInfo = (SDL_GPUShaderCreateInfo *)SDL_ShaderCross_INTERNAL_TranslateFromSPIRV(
            format,
            &createInfo,
            false);

        void *result = (void *)translatedCreateInfo->code;
        *size = translatedCreateInfo->code_size;
        SDL_free((void *)translatedCreateInfo->entrypoint);
        SDL_free(translatedCreateInfo);

        return result;
    }
}

void *SDL_ShaderCross_TranspileMSLFromSPIRV(
    const Uint8 *bytecode,
    size_t bytecodeSize,
    const char *entrypoint,
    SDL_ShaderCross_ShaderStage shaderStage)
{
    size_t size;
    return SDL_ShaderCross_INTERNAL_CompileFromSPIRV(
        SDL_GPU_SHADERFORMAT_MSL,
        bytecode,
        bytecodeSize,
        entrypoint,
        shaderStage,
        &size);
}

void *SDL_ShaderCross_CompileDXBCFromSPIRV(
    const Uint8 *bytecode,
    size_t bytecodeSize,
    const char *entrypoint,
    SDL_ShaderCross_ShaderStage shaderStage,
    size_t *size)
{
    return SDL_ShaderCross_INTERNAL_CompileFromSPIRV(
        SDL_GPU_SHADERFORMAT_DXBC,
        bytecode,
        bytecodeSize,
        entrypoint,
        shaderStage,
        size);
}

void *SDL_ShaderCross_CompileDXILFromSPIRV(
    const Uint8 *bytecode,
    size_t bytecodeSize,
    const char *entrypoint,
    SDL_ShaderCross_ShaderStage shaderStage,
    size_t *size)
{
    return SDL_ShaderCross_INTERNAL_CompileFromSPIRV(
        SDL_GPU_SHADERFORMAT_DXIL,
        bytecode,
        bytecodeSize,
        entrypoint,
        shaderStage,
        size);
}

static void *SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV(
    SDL_GPUDevice *device,
    const void *originalCreateInfo,
    bool isCompute)
{
    const SDL_GPUShaderCreateInfo *createInfo;
    SDL_GPUShaderFormat format;
    void *compiledResult;

    SDL_GPUShaderFormat shader_formats = SDL_GetGPUShaderFormats(device);

    if (shader_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        if (isCompute) {
            return SDL_CreateGPUComputePipeline(device, (const SDL_GPUComputePipelineCreateInfo *)originalCreateInfo);
        } else {
            return SDL_CreateGPUShader(device, (const SDL_GPUShaderCreateInfo *)originalCreateInfo);
        }
    } else if (shader_formats & SDL_GPU_SHADERFORMAT_DXBC) {
        format = SDL_GPU_SHADERFORMAT_DXBC;
    } else if (shader_formats & SDL_GPU_SHADERFORMAT_DXIL) {
        format = SDL_GPU_SHADERFORMAT_DXIL;
    } else if (shader_formats & SDL_GPU_SHADERFORMAT_MSL) {
        format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        SDL_SetError("SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV: Unexpected SDL_GPUBackend");
        return NULL;
    }

    void *translatedCreateInfo = SDL_ShaderCross_INTERNAL_TranslateFromSPIRV(
        format,
        createInfo,
        isCompute);

    if (isCompute) {
        SDL_GPUComputePipelineCreateInfo *computeCreateInfo = (SDL_GPUComputePipelineCreateInfo *)translatedCreateInfo;
        compiledResult = SDL_CreateGPUComputePipeline(device, computeCreateInfo);
        SDL_free((void *)computeCreateInfo->entrypoint);
        SDL_free((void *)computeCreateInfo->code);
        SDL_free(computeCreateInfo);
    } else {
        SDL_GPUShaderCreateInfo *shaderCreateInfo = (SDL_GPUShaderCreateInfo *)translatedCreateInfo;
        compiledResult = SDL_CreateGPUShader(device, shaderCreateInfo);
        SDL_free((void *)shaderCreateInfo->entrypoint);
        SDL_free((void *)shaderCreateInfo->code);
        SDL_free(shaderCreateInfo);
    }

    return compiledResult;
}

SDL_GPU_SHADERCROSS_EXPORT SDL_GPUShader *SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
    SDL_GPUDevice *device,
    const SDL_GPUShaderCreateInfo *createInfo)
{
    return (SDL_GPUShader *)SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV(device, createInfo, false);
}

SDL_GPU_SHADERCROSS_EXPORT SDL_GPUComputePipeline *SDL_ShaderCross_CompileComputePipelineFromSPIRV(
    SDL_GPUDevice *device,
    const SDL_GPUComputePipelineCreateInfo *createInfo)
{
    return (SDL_GPUComputePipeline *)SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV(device, createInfo, true);
}

#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

SDL_GPU_SHADERCROSS_EXPORT bool SDL_ShaderCross_Init(void)
{
    dxcompiler_dll = SDL_LoadObject(DXCOMPILER_DLL);
    if (dxcompiler_dll != NULL) {
#ifndef _GAMING_XBOX
        /* Try to load DXIL, we don't need it directly but if it doesn't exist the code will not be loadable */
        SDL_SharedObject *dxil_dll = SDL_LoadObject(DXIL_DLL);
        if (dxil_dll == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "Failed to load DXIL library, this will cause pipeline creation failures!");

            SDL_UnloadObject(dxcompiler_dll);
            dxcompiler_dll = NULL;
        } else {
            SDL_UnloadObject(dxil_dll); /* Unload immediately, we don't actually need it */
        }
#endif
    }

    if (dxcompiler_dll != NULL) {
        SDL_DxcCreateInstance = (DxcCreateInstanceProc)SDL_LoadFunction(dxcompiler_dll, "DxcCreateInstance");

        if (SDL_DxcCreateInstance == NULL) {
            SDL_UnloadObject(dxcompiler_dll);
            dxcompiler_dll = NULL;
        }
    }

    d3dcompiler_dll = SDL_LoadObject(D3DCOMPILER_DLL);

    if (d3dcompiler_dll != NULL) {
        SDL_D3DCompile = (pfn_D3DCompile)SDL_LoadFunction(d3dcompiler_dll, "D3DCompile");

        if (SDL_D3DCompile == NULL) {
            SDL_UnloadObject(d3dcompiler_dll);
            d3dcompiler_dll = NULL;
        }
    }

    bool spvc_loaded = false;

#ifndef SDL_GPU_SHADERCROSS_STATIC
    spirvcross_dll = SDL_LoadObject(SDL_GPU_SPIRV_CROSS_DLL);
    if (spirvcross_dll != NULL) {
        spvc_loaded = true;
    }

    if (spvc_loaded) {
#define CHECK_FUNC(func)                                                  \
    if (SDL_##func == NULL) {                                             \
        SDL_##func = (pfn_##func)SDL_LoadFunction(spirvcross_dll, #func); \
        if (SDL_##func == NULL) {                                         \
            spvc_loaded = false;                                      \
        }                                                                 \
    }
        CHECK_FUNC(spvc_context_create)
        CHECK_FUNC(spvc_context_destroy)
        CHECK_FUNC(spvc_context_parse_spirv)
        CHECK_FUNC(spvc_context_create_compiler)
        CHECK_FUNC(spvc_compiler_create_compiler_options)
        CHECK_FUNC(spvc_compiler_options_set_uint)
        CHECK_FUNC(spvc_compiler_install_compiler_options)
        CHECK_FUNC(spvc_compiler_compile)
        CHECK_FUNC(spvc_context_get_last_error_string)
        CHECK_FUNC(spvc_compiler_get_execution_model)
        CHECK_FUNC(spvc_compiler_get_cleansed_entry_point_name)
#undef CHECK_FUNC
    }

    if (spirvcross_dll != NULL && !spvc_loaded) {
        SDL_UnloadObject(spirvcross_dll);
        spirvcross_dll = NULL;
    }
#endif /* SDL_GPU_SHADERCROSS_STATIC */

    return true;
}

SDL_GPU_SHADERCROSS_EXPORT void SDL_ShaderCross_Quit(void)
{
#ifdef SDL_GPU_SHADERCROSS_SPIRV
#ifndef SDL_GPU_SHADERCROSS_STATIC
    if (spirvcross_dll != NULL) {
        SDL_UnloadObject(spirvcross_dll);
        spirvcross_dll = NULL;

        SDL_spvc_context_create = NULL;
        SDL_spvc_context_destroy = NULL;
        SDL_spvc_context_parse_spirv = NULL;
        SDL_spvc_context_create_compiler = NULL;
        SDL_spvc_compiler_create_compiler_options = NULL;
        SDL_spvc_compiler_options_set_uint = NULL;
        SDL_spvc_compiler_install_compiler_options = NULL;
        SDL_spvc_compiler_compile = NULL;
        SDL_spvc_context_get_last_error_string = NULL;
        SDL_spvc_compiler_get_execution_model = NULL;
        SDL_spvc_compiler_get_cleansed_entry_point_name = NULL;
    }
#endif /* SDL_GPU_SHADERCROSS_STATIC */
#endif /* SDL_GPU_SHADERCROSS_SPIRV */

#if SDL_GPU_SHADERCROSS_HLSL
    if (d3dcompiler_dll != NULL) {
        SDL_UnloadObject(d3dcompiler_dll);
        d3dcompiler_dll = NULL;

        SDL_D3DCompile = NULL;
    }

    if (dxcompiler_dll != NULL) {
        SDL_UnloadObject(dxcompiler_dll);
        dxcompiler_dll = NULL;

        SDL_DxcCreateInstance = NULL;
    }
#endif /* SDL_GPU_SHADERCROSS_HLSL */
}

#ifdef SDL_GPU_SHADERCROSS_SPIRVCROSS

SDL_GPU_SHADERCROSS_EXPORT SDL_GPUShaderFormat SDL_ShaderCross_GetSPIRVShaderFormats(void)
{
    /* SPIRV can always be output as-is with no preprocessing */
    SDL_GPUShaderFormat supportedFormats = SDL_GPU_SHADERFORMAT_SPIRV;

    /* SPIRV-Cross allows us to cross compile to MSL */
    if (
#ifndef SDL_GPU_SHADERCROSS_STATIC
        spirvcross_dll != NULL
#else  /* SDL_GPU_SHADERCROSS_STATIC */
        true
#endif /* SDL_GPU_SHADERCROSS_STATIC */
    ) {
        supportedFormats |= SDL_GPU_SHADERFORMAT_MSL;

#if SDL_GPU_SHADERCROSS_HLSL
        /* SPIRV-Cross + DXC allows us to cross-compile to HLSL, then compile to DXIL */
        if (dxcompiler_dll != NULL) {
            supportedFormats |= SDL_GPU_SHADERFORMAT_DXIL;
        }

        /* SPIRV-Cross + FXC allows us to cross-compile to HLSL, then compile to DXBC */
        if (d3dcompiler_dll != NULL) {
            supportedFormats |= SDL_GPU_SHADERFORMAT_DXBC;
        }
#endif /* SDL_GPU_SHADERCROSS_HLSL */
    }

    return supportedFormats;
}

#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

#if SDL_GPU_SHADERCROSS_HLSL

SDL_GPU_SHADERCROSS_EXPORT SDL_GPUShaderFormat SDL_ShaderCross_GetHLSLShaderFormats(void)
{
    SDL_GPUShaderFormat supportedFormats = 0;

    /* DXC allows compilation from HLSL to DXIL and SPIRV */
    if (dxcompiler_dll != NULL) {
        supportedFormats |= SDL_GPU_SHADERFORMAT_DXIL;
        supportedFormats |= SDL_GPU_SHADERFORMAT_SPIRV;
    }

    /* FXC allows compilation of HLSL to DXBC */
    if (d3dcompiler_dll != NULL) {
        supportedFormats |= SDL_GPU_SHADERFORMAT_DXBC;
    }

    return supportedFormats;
}

#endif /* SDL_GPU_SHADERCROSS_HLSL */
