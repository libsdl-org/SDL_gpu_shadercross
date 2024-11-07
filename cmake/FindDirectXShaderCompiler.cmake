set(required_vars)

if(WIN32)
    find_path(DirectXShaderCompiler_INCLUDE_PATH NAMES "dxcapi.h" PATH_SUFFIXES "inc" "windows/inc" HINTS ${DirectXShaderCompiler_ROOT})
    if(SDL_CPU_ARM64)
        set(extra_bin_suffix "bin/arm64" "windows/bin/arm64")
        set(extra_lib_suffix "lib/arm64" "windows/lib/arm64")
    elseif(SDL_CPU_X86)
        set(extra_bin_suffix "bin/x86" "windows/bin/x86")
        set(extra_lib_suffix "lib/x86" "windows/lib/x86")
    elseif(SDL_CPU_X64)
        set(extra_bin_suffix "bin/x64" "windows/bin/x64")
        set(extra_lib_suffix "lib/x64" "windows/lib/x64")
    endif()
    find_file(DirectXShaderCompiler_dxcompiler_BINARY NAMES "dxcompiler.dll" PATH_SUFFIXES "bin" ${extra_bin_suffix} HINTS ${DirectXShaderCompiler_ROOT})
    find_library(DirectXShaderCompiler_dxcompiler_LIBRARY NAMES "dxcompiler" "dxcompiler.lib" PATH_SUFFIXES "lib" ${extra_lib_suffix} HINTS ${DirectXShaderCompiler_ROOT})
    find_file(DirectXShaderCompiler_dxil_BINARY NAMES "dxil.dll" PATH_SUFFIXES "bin" ${extra_bin_suffix} HINTS ${DirectXShaderCompiler_ROOT})
    set(required_vars
        DirectXShaderCompiler_INCLUDE_PATH
        DirectXShaderCompiler_dxcompiler_BINARY
        DirectXShaderCompiler_dxcompiler_LIBRARY
        DirectXShaderCompiler_dxil_BINARY
    )
else()
    find_path(DirectXShaderCompiler_INCLUDE_PATH NAMES "dxcapi.h" PATH_SUFFIXES "include" "include/dxc" "linux/include" "linux/include/dxc")
    find_library(DirectXShaderCompiler_dxcompiler_LIBRARY NAMES "dxcompiler" PATH_SUFFIXES "lib" "linux/lib" HINTS ${DirectXShaderCompiler_ROOT})
    find_library(DirectXShaderCompiler_dxil_LIBRARY NAMES "dxil" PATH_SUFFIXES "lib" "linux/lib" HINTS ${DirectXShaderCompiler_ROOT})
    set(required_vars
        DirectXShaderCompiler_INCLUDE_PATH
        DirectXShaderCompiler_dxcompiler_LIBRARY
        DirectXShaderCompiler_dxil_LIBRARY
    )
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(DirectXShaderCompiler
    REQUIRED_VARS ${required_vars}
)

if(DirectXShaderCompiler_FOUND)
    if(NOT TARGET DirectXShaderCompiler::dxcompiler)
        add_library(DirectXShaderCompiler::dxcompiler IMPORTED SHARED)
        set_property(TARGET DirectXShaderCompiler::dxcompiler PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DirectXShaderCompiler_INCLUDE_PATH}")
        if(WIN32)
            set_property(TARGET DirectXShaderCompiler::dxcompiler PROPERTY IMPORTED_LOCATION "${DirectXShaderCompiler_dxcompiler_BINARY}")
            set_property(TARGET DirectXShaderCompiler::dxcompiler PROPERTY IMPORTED_IMPLIB "${DirectXShaderCompiler_dxcompiler_LIBRARY}")
        else()
            set_property(TARGET DirectXShaderCompiler::dxcompiler PROPERTY IMPORTED_LOCATION "${DirectXShaderCompiler_dxcompiler_LIBRARY}")
        endif()
    endif()
    if(NOT TARGET DirectXShaderCompiler::dxil)
        add_library(DirectXShaderCompiler::dxil IMPORTED SHARED)
        if(WIN32)
            set_property(TARGET DirectXShaderCompiler::dxil PROPERTY IMPORTED_LOCATION "${DirectXShaderCompiler_dxil_BINARY}")
        else()
            set_property(TARGET DirectXShaderCompiler::dxil PROPERTY IMPORTED_LOCATION "${DirectXShaderCompiler_dxil_LIBRARY}")
        endif()
    endif()
endif()
