set(DXC_LINUX_X64_URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/linux_dxc_2024_07_31.x86_64.tar.gz")
set(DXC_LINUX_X64_HASH "SHA256=b58725ac191df0686994fb9d54d27ee8dd3f180b023d562733112a499ea1195c")
set(DXC_WINDOWS_X86_X64_ARM64_URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/dxc_2024_07_31.zip")
set(DXC_WINDOWS_X86_X64_ARM64_HASH "SHA256=e2627f004f0f9424d8c71ea1314d04f38c5a5096884ae9217f1f18bd320267b5")

get_filename_component(EXTERNAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../external" ABSOLUTE)
set(DXC_ROOT "${EXTERNAL_PATH}/DirectXShaderCompiler-binaries")

set(DOWNLOAD_LINUX ON)
set(DOWNLOAD_WINDOWS ON)
if(DEFINED CMAKE_SYSTEM_NAME)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(DOWNLOAD_Linux OFF)
    endif()
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(DOWNLOAD_WINDOWS OFF)
    endif()
endif()

if(DOWNLOAD_LINUX)
    include(FetchContent)
    FetchContent_Populate(
        dxc_linux
        URL  "${DXC_LINUX_X64_URL}"
        URL_HASH  "${DXC_LINUX_X64_HASH}"
        SOURCE_DIR "${DXC_ROOT}/linux"
    )
endif()

if(DOWNLOAD_WINDOWS)
    include(FetchContent)
    FetchContent_Populate(
        dxc_windows
        URL  "${DXC_WINDOWS_X86_X64_ARM64_URL}"
        URL_HASH  "${DXC_WINDOWS_X86_X64_ARM64_HASH}"
        SOURCE_DIR "${DXC_ROOT}/windows"
    )
endif()

message("To make use of the prebuilt DirectXShaderCompiler libraries, configure with:")
message("")
message("  -DSDLGPUSHADERCROSS_VENDORED=OFF")
message("")
message("and")
message("")
message("  -DDirectXShaderCompiler_ROOT=\"${DXC_ROOT}\"")
message("")
