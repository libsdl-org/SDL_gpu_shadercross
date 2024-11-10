# CMake Project for SDL3_gpu_shadercross - Simple DirectMedia Layer Shader Cross Compiler
# Written by @thatcosmonaut
cmake_minimum_required(VERSION 3.22)

# Version
set(MAJOR_VERSION 3)
set(MINOR_VERSION 0)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION "3.1.3")

# option() honors normal variables.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(SDL3_gpu_shadercross LANGUAGES C VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CMakeDependentOption)
include("${CMAKE_CURRENT_LIST_DIR}/cmake/PrivateSdlFunctions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sdlcpu.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sdlplatform.cmake")

if(NOT TARGET SDL3::Headers OR NOT TARGET SDL3::SDL3-shared)
    find_package(SDL3 ${SDL_REQUIRED_VERSION} REQUIRED COMPONENTS Headers SDL3-shared)
endif()

if(BUILD_SHARED_LIBS)
	set(SDLGPUSHADERCROSS_SHARED_DEFAULT ON)
	set(SDLGPUSHADERCROSS_STATIC_DEFAULT OFF)
else()
	set(SDLGPUSHADERCROSS_SHARED_DEFAULT OFF)
	set(SDLGPUSHADERCROSS_STATIC_DEFAULT ON)
endif()

# Options
option(SDLGPUSHADERCROSS_SHARED "Build shared SDL_gpu_shadercross library" ${SDLGPUSHADERCROSS_SHARED_DEFAULT})
option(SDLGPUSHADERCROSS_STATIC "Build static SDL_gpu_shadercross library" ${SDLGPUSHADERCROSS_STATIC_DEFAULT})
option(SDLGPUSHADERCROSS_SPIRVCROSS_SHARED "Link to shared library variants of dependencies" ON)
option(SDLGPUSHADERCROSS_VENDORED "Use vendored dependencies" OFF)
option(SDLGPUSHADERCROSS_CLI "Build command line executable" ON)
cmake_dependent_option(SDLGPUSHADERCROSS_CLI_STATIC "Link CLI with static libraries" OFF "SDLGPUSHADERCROSS_CLI;SDLGPUSHADERCROSS_STATIC;TARGET SDL3::SDL3-static" OFF)
option(SDLGPUSHADERCROSS_WERROR "Enable Werror" OFF)
option(SDLGPUSHADERCROSS_INSTALL "Enable installation" OFF)
cmake_dependent_option(SDLGPUSHADERCROSS_INSTALL_CPACK "Enable CPack installation" OFF "SDLGPUSHADERCROSS_INSTALL" OFF)
cmake_dependent_option(SDLGPUSHADERCROSS_INSTALL_RUNTIME "Download, build and install runtime dependencies" OFF "SDLGPUSHADERCROSS_INSTALL" OFF)

sdl_calculate_derived_version_variables(${MAJOR_VERSION} ${MINOR_VERSION} ${MICRO_VERSION})
SDL_DetectTargetCPUArchitectures(SDL_CPU_NAMES)
SDL_DetectCMakePlatform()

if(SDLGPUSHADERCROSS_INSTALL)
	include(GNUInstallDirs)
endif()

if(NOT SDLGPUSHADERCROSS_SHARED AND NOT SDLGPUSHADERCROSS_STATIC)
	message(FATAL_ERROR "SDLGPUSHADERCROSS_SHARED and SDLGPUSHADERCROSS_STATIC cannot be both disabled")
endif()

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
endif()

if(NOT MSVC)
	add_compile_options(-pedantic) # -Wno-strict-aliasing
endif()

set(spirv_static_targets spirv-cross-c spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl spirv-cross-cpp spirv-cross-reflect spirv-cross-core)

set(pc_requires )
set(install_extra_targets )
if(SDLGPUSHADERCROSS_VENDORED)
	set(SPIRV_CROSS_SKIP_INSTALL ON)
	set(SPIRV_CROSS_CLI OFF)
	set(SPIRV_CROSS_ENABLE_TESTS OFF)
	if(SDLGPUSHADERCROSS_SPIRVCROSS_SHARED)
		set(SPIRV_CROSS_SHARED ON)
		set(SPIRV_CROSS_STATIC OFF)
	else()
		set(SPIRV_CROSS_SHARED OFF)
		set(SPIRV_CROSS_STATIC ON)
	endif()

	sdl_check_project_in_subfolder(external/SPIRV-Cross SPIRV-Cross SDLGPUSHADERCROSS_VENDORED)
	set(SPIRV_CROSS_ENABLE_TESTS ON)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	add_subdirectory(external/SPIRV-Cross EXCLUDE_FROM_ALL)

	if(SDLGPUSHADERCROSS_SPIRVCROSS_SHARED)
		list(APPEND install_extra_targets spirv-cross-c-shared)
		list(APPEND pc_requires "spirv-cross-c-shared")
	else()
		enable_language(CXX)
		if(SDLGPUSHADERCROSS_STATIC)
			foreach(extra IN LISTS spirv_static_targets)
				if(TARGET ${extra})
					list(APPEND install_extra_targets ${extra})
					list(APPEND pc_requires "${extra}")
				endif()
			endforeach()
		endif()
	endif()

	sdl_check_project_in_subfolder(external/SPIRV-Headers SPIRV-Headers SDLGPUSHADERCROSS_VENDORED)
	add_subdirectory(external/SPIRV-Headers EXCLUDE_FROM_ALL)
	sdl_check_project_in_subfolder(external/SPIRV-Tools SPIRV-Tools SDLGPUSHADERCROSS_VENDORED)
	add_subdirectory(external/SPIRV-Tools EXCLUDE_FROM_ALL)

	sdl_check_project_in_subfolder(external/DirectXShaderCompiler DirectXShaderCompiler SDLGPUSHADERCROSS_VENDORED)
	if(MINGW)
		set(CMAKE_C_COMPILER_ARCHITECTURE_ID "${SDL_CPU_NAMES}")
	endif()
	set(DXC_COVERAGE OFF)
	set(HLSL_INCLUDE_TESTS OFF)
	set(LLVM_INCLUDE_TESTS OFF)
	include(external/DirectXShaderCompiler/cmake/caches/PredefinedParams.cmake)
	add_subdirectory(external/DirectXShaderCompiler EXCLUDE_FROM_ALL)
	add_library(DirectXShaderCompiler::dxcompiler ALIAS dxcompiler)
	add_library(DirectXShaderCompiler::dxil ALIAS dxildll)
	list(APPEND install_extra_targets dxcompiler dxildll)
	add_dependencies(dxcompiler dxildll)
else()
	if(SDLGPUSHADERCROSS_SPIRVCROSS_SHARED)
		if(NOT TARGET spirv_cross_c_shared)
			find_package(spirv_cross_c_shared REQUIRED)
		endif()
	else()
		enable_language(CXX)
		if(NOT TARGET spirv_cross_c)
			find_package(spirv_cross_core QUIET)
			find_package(spirv_cross_glsl QUIET)
			find_package(spirv_cross_hlsl QUIET)
			find_package(spirv_cross_msl QUIET)
			find_package(spirv_cross_cpp QUIET)
			find_package(spirv_cross_reflect QUIET)
			find_package(spirv_cross_c REQUIRED)
		endif()
	endif()

	set(DirectXShaderCompiler_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/DirectXShaderCompiler-binaries")
	find_package(DirectXShaderCompiler REQUIRED)
endif()

# Source lists
set(SOURCE_FILES
	# Public Headers
	include/SDL3_gpu_shadercross/SDL_gpu_shadercross.h
	# Source Files
	src/SDL_gpu_shadercross.c
)

set(SDL3_gpu_shadercross_targets)
if(SDLGPUSHADERCROSS_SHARED)
	list(APPEND SDL3_gpu_shadercross_targets SDL3_gpu_shadercross-shared)
	add_library(SDL3_gpu_shadercross-shared SHARED ${SOURCE_FILES})
	add_library(SDL3_gpu_shadercross::SDL3_gpu_shadercross ALIAS SDL3_gpu_shadercross-shared)

	set_property(TARGET SDL3_gpu_shadercross-shared PROPERTY DEFINE_SYMBOL DLL_EXPORT)
	sdl_target_link_option_version_file(SDL3_gpu_shadercross-shared "${CMAKE_CURRENT_SOURCE_DIR}/src/SDL_gpu_shadercross.sym")
	sdl_target_link_options_no_undefined(SDL3_gpu_shadercross-shared)

	# Build flags
	if(WIN32)
		target_sources(SDL3_gpu_shadercross-shared PRIVATE "src/version.rc")
		set_property(TARGET SDL3_gpu_shadercross-shared PROPERTY PREFIX "")
	endif()

	# SDL3_gpu_shadercross folders as includes, for other targets to consume
	target_include_directories(SDL3_gpu_shadercross-shared PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
	target_include_directories(SDL3_gpu_shadercross-shared PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

	# Soname
	set_target_properties(SDL3_gpu_shadercross-shared PROPERTIES
		OUTPUT_NAME "SDL3_gpu_shadercross"
		SOVERSION "${SO_VERSION_MAJOR}"
		VERSION "${SO_VERSION}"
	)

	target_link_libraries(SDL3_gpu_shadercross-shared PRIVATE
		SDL3::SDL3-shared
	)
endif()


if(SDLGPUSHADERCROSS_STATIC)
	list(APPEND SDL3_gpu_shadercross_targets SDL3_gpu_shadercross-static)
	if(NOT TARGET SDL3::Headers OR NOT TARGET SDL3::SDL3)
		find_package(SDL3 ${SDL_REQUIRED_VERSION} REQUIRED COMPONENTS Headers)
	endif()

	add_library(SDL3_gpu_shadercross-static STATIC ${SOURCE_FILES})
	add_library(SDL3_gpu_shadercross::SDL3_gpu_shadercross-static ALIAS SDL3_gpu_shadercross-static)

	if(NOT MSVC)
		set_property(TARGET SDL3_gpu_shadercross-static PROPERTY OUTPUT_NAME "SDL3_gpu_shadercross")
	endif()

	# SDL3_gpu_shadercross folders as includes, for other targets to consume
	target_include_directories(SDL3_gpu_shadercross-static PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
	target_include_directories(SDL3_gpu_shadercross-static PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

	target_link_libraries(SDL3_gpu_shadercross-static PUBLIC
		SDL3::Headers
	)
endif()

foreach(target IN LISTS SDL3_gpu_shadercross_targets)
	sdl_add_warning_options(${target} WARNING_AS_ERROR ${SDLGPUSHADERCROSS_WERROR})
	target_compile_features(${target} PRIVATE c_std_99)

	if(SDLGPUSHADERCROSS_SPIRVCROSS_SHARED)
		target_link_libraries(${target} PRIVATE spirv-cross-c-shared)
	else()
		target_link_libraries(${target} PRIVATE spirv-cross-c)
	endif()
	target_link_libraries(${target} PRIVATE DirectXShaderCompiler::dxcompiler)
	if(NOT SDLGPUSHADERCROSS_SPIRVCROSS_SHARED)
		# spirv-cross uses C++
		set_property(TARGET ${target} PROPERTY LINKER_LANGUAGE CXX)
	endif()
endforeach()

if(NOT TARGET SDL3_gpu_shadercross::SDL3_gpu_shadercross)
	if(TARGET SDL3_gpu_shadercross-shared)
		add_library(SDL3_gpu_shadercross::SDL3_gpu_shadercross ALIAS SDL3_gpu_shadercross-shared)
	else()
		add_library(SDL3_gpu_shadercross::SDL3_gpu_shadercross ALIAS SDL3_gpu_shadercross-static)
	endif()
endif()

if(SDLGPUSHADERCROSS_CLI)
	add_executable(shadercross src/cli.c)
	sdl_add_warning_options(shadercross WARNING_AS_ERROR ${SDLGPUSHADERCROSS_WERROR})
	sdl_target_link_options_no_undefined(shadercross)

	if(SDLGPUSHADERCROSS_CLI_STATIC)
		target_link_libraries(shadercross PRIVATE SDL3_gpu_shadercross::SDL3_gpu_shadercross-static)
		target_link_libraries(shadercross PRIVATE SDL3::SDL3-static)
	else()
		target_link_libraries(shadercross PRIVATE SDL3_gpu_shadercross::SDL3_gpu_shadercross)
		target_link_libraries(shadercross PRIVATE SDL3::SDL3)
	endif()
endif()

if(SDLGPUSHADERCROSS_INSTALL)
	if(WIN32 AND NOT MINGW)
		set(INSTALL_CMAKEDIR_ROOT_DEFAULT "cmake")
	else()
		set(INSTALL_CMAKEDIR_ROOT_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake")
	endif()
	set(SDLGPUSHADERCROSS_INSTALL_CMAKEDIR_ROOT "${INSTALL_CMAKEDIR_ROOT_DEFAULT}" CACHE STRING "Root folder where to install SDL3_gpu_shadercross cmake related files (SDL3_gpu_shadercross subfolder for MSVC projects)")
	set(SDLGPUSHADERCROSS_PKGCONFIG_INSTALLDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

	if(WIN32 AND NOT MINGW)
		set(SDLGPUSHADERCROSS_INSTALL_CMAKEDIR "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR_ROOT}")
	else()
		set(SDLGPUSHADERCROSS_INSTALL_CMAKEDIR "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR_ROOT}/SDL3_gpu_shadercross")
	endif()

	if(TARGET SDL3_gpu_shadercross-shared)
		install(TARGETS SDL3_gpu_shadercross-shared EXPORT SDL3_gpu_shadercross-shared-export
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
			RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
		)
		install(EXPORT SDL3_gpu_shadercross-shared-export
			FILE SDL3_gpu_shadercross-shared-targets.cmake
			NAMESPACE SDL3_gpu_shadercross::
			DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
			COMPONENT devel
		)
	endif()
	if(TARGET SDL3_gpu_shadercross-static)
		install(TARGETS SDL3_gpu_shadercross-static EXPORT SDL3_gpu_shadercross-static-export
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
		)
		install(EXPORT SDL3_gpu_shadercross-static-export
			FILE SDL3_gpu_shadercross-static-targets.cmake
			NAMESPACE SDL3_gpu_shadercross::
			DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
			COMPONENT devel
		)
	endif()
	if(install_extra_targets)
		install(TARGETS ${install_extra_targets} EXPORT SDL3_gpu_shadercross-vendored
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
			RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
		)
		install(EXPORT SDL3_gpu_shadercross-vendored
			FILE SDL3_gpu_shadercross-vendored-targets.cmake
			NAMESPACE SDL3_gpu_shadercross::vendored::
			DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
			COMPONENT devel
		)
	endif()
	install(
		FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3_gpu_shadercross/SDL_gpu_shadercross.h"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL3_gpu_shadercross" COMPONENT DEVEL
	)
	if(SDLGPUSHADERCROSS_CLI)
		install(TARGETS shadercross RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
	endif()

	include(CMakePackageConfigHelpers)
	configure_package_config_file(cmake/SDL3_gpu_shadercrossConfig.cmake.in SDL3_gpu_shadercrossConfig.cmake
		NO_SET_AND_CHECK_MACRO
		INSTALL_DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
	)
	write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL3_gpu_shadercrossConfigVersion.cmake"
		COMPATIBILITY AnyNewerVersion
	)
	install(
		FILES
			"${CMAKE_CURRENT_BINARY_DIR}/SDL3_gpu_shadercrossConfig.cmake"
			"${CMAKE_CURRENT_BINARY_DIR}/SDL3_gpu_shadercrossConfigVersion.cmake"
		DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
		COMPONENT devel
	)
	if(NOT SDLGPUSHADERCROSS_VENDORED)
		install(
			FILES
				"cmake/sdlcpu.cmake"
				"cmake/FindDirectXShaderCompiler.cmake"
			DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
			COMPONENT devel
		)
	endif()

	file(RELATIVE_PATH SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${CMAKE_INSTALL_PREFIX}/${SDLGPUSHADERCROSS_PKGCONFIG_INSTALLDIR}" "${CMAKE_INSTALL_PREFIX}")
	string(REGEX REPLACE "[/]+$" "" SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
	set(SDL_PKGCONFIG_PREFIX "\${pcfiledir}/${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
	if(NOT SDLGPUSHADERCROSS_VENDORED)
		if(SDLGPUSHADERCROSS_SPIRVCROSS_SHARED)
			set(PC_REQUIRES "spirv-cross-c-shared")
		else()
			set(PC_REQUIRES "spirv-cross-c")
		endif()
	endif()
	set(PC_LIBS "-ldxcompiler")
	configure_file(cmake/sdl3-gpu-shadercross.pc.in sdl3-gpu-shadercross.pc @ONLY)

	# Always install sdl3-gpu-shadercross.pc file: libraries might be different between config modes
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdl3-gpu-shadercross.pc"
			DESTINATION "${SDLGPUSHADERCROSS_PKGCONFIG_INSTALLDIR}" COMPONENT devel)

	install(FILES "LICENSE.txt"
		DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
		COMPONENT library
	)
	if(SDLGPUSHADERCROSS_INSTALL_CPACK)
		if(MSVC)
			set(CPACK_GENERATOR "ZIP")
		else()
			set(CPACK_GENERATOR "TGZ")
		endif()
		configure_file(cmake/CPackProjectConfig.cmake.in CPackProjectConfig.cmake @ONLY)
		set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/CPackProjectConfig.cmake")
		# CPACK_SOURCE_PACKAGE_FILE_NAME must end with "-src" (so we can block creating a source archive)
		set(CPACK_SOURCE_PACKAGE_FILE_NAME "SDL3_gpu_shadercross-${PROJECT_VERSION}-src")
		set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
		include(CPack)
	endif()
endif()

set(DXC_LINUX_X64_URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/linux_dxc_2024_07_31.x86_64.tar.gz")
set(DXC_LINUX_X64_HASH "SHA256=b58725ac191df0686994fb9d54d27ee8dd3f180b023d562733112a499ea1195c")
set(DXC_WINDOWS_X86_X64_ARM64_URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/dxc_2024_07_31.zip")
set(DXC_WINDOWS_X86_X64_ARM64_HASH "SHA256=e2627f004f0f9424d8c71ea1314d04f38c5a5096884ae9217f1f18bd320267b5")

if(SDLGPUSHADERCROSS_INSTALL_RUNTIME)
	set(chmod_0755 OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	if(APPLE)
		set(rpath_origin "@executable_path")
	else()
		set(rpath_origin "\$ORIGIN")
	endif()

	if(NOT SDLGPUSHADERCROSS_VENDORED)
		install(IMPORTED_RUNTIME_ARTIFACTS DirectXShaderCompiler::dxcompiler DirectXShaderCompiler::dxil RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
		if(SDLGPUSHADERCROSS_SPIRVCROSS_SHARED)
			install(IMPORTED_RUNTIME_ARTIFACTS spirv-cross-c-shared RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
		endif()
	endif()

	if(TARGET shadercross)
		file(RELATIVE_PATH bin_to_lib "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_LIBDIR}")
		set_property(TARGET shadercross PROPERTY INSTALL_RPATH "${rpath_origin}/${bin_to_lib}")
	endif()

	# Install SDL3
	if(SDLGPUSHADERCROSS_CLI AND NOT SDLGPUSHADERCROSS_CLI_STATIC)
		install(IMPORTED_RUNTIME_ARTIFACTS SDL3::SDL3-shared RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
	endif()
	include(ExternalProject)

	if(NOT WIN32)
		ExternalProject_Add(spirv_headers
			GIT_REPOSITORY 		"https://github.com/KhronosGroup/SPIRV-Headers.git"
			GIT_TAG 			"main"
			CMAKE_ARGS 			"-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_INSTALL_LIBDIR=lib" "-DCMAKE_INSTALL_BINDIR=bin" "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>" "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
			BUILD_COMMAND 		"${CMAKE_COMMAND}" "--build" "<BINARY_DIR>" --config "Release"
			INSTALL_COMMAND 	"${CMAKE_COMMAND}" "--install" "<BINARY_DIR>" --config "Release"
		)
		ExternalProject_Get_property(spirv_headers INSTALL_DIR)
		set(spirv_headers_install_dir "${INSTALL_DIR}")

		ExternalProject_Add(vulkan_headers
			GIT_REPOSITORY 		"https://github.com/KhronosGroup/Vulkan-Headers"
			GIT_TAG 			"main"
			CMAKE_ARGS 			"-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_INSTALL_LIBDIR=lib" "-DCMAKE_INSTALL_BINDIR=bin" "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>" "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
			BUILD_COMMAND 		"${CMAKE_COMMAND}" "--build" "<BINARY_DIR>" --config "Release"
			INSTALL_COMMAND 	"${CMAKE_COMMAND}" "--install" "<BINARY_DIR>" --config "Release"
		)
		ExternalProject_Get_property(vulkan_headers INSTALL_DIR)
		set(vulkan_headers_install_dir "${INSTALL_DIR}")

		ExternalProject_Add(vulkan_loader
			DEPENDS 			spirv_headers vulkan_headers
			GIT_REPOSITORY 		"https://github.com/KhronosGroup/Vulkan-Loader.git"
			GIT_TAG 			"main"
			CMAKE_ARGS 			"-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_INSTALL_LIBDIR=lib" "-DCMAKE_INSTALL_BINDIR=bin" "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>" "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DCMAKE_PREFIX_PATH=${vulkan_headers_install_dir}"
			BUILD_COMMAND 		"${CMAKE_COMMAND}" "--build" "<BINARY_DIR>" --config "Release"
			INSTALL_COMMAND 	"${CMAKE_COMMAND}" "--install" "<BINARY_DIR>" --config "Release"
		)
		ExternalProject_Get_property(vulkan_loader INSTALL_DIR)
		set(vulkan_loader_install_dir "${INSTALL_DIR}")

		find_package(BISON REQUIRED)
		ExternalProject_Add(vkd3d
			DEPENDS 			spirv_headers vulkan_headers vulkan_loader
			URL					"https://dl.winehq.org/vkd3d/source/vkd3d-1.13.tar.xz"
			URL_HASH			"SHA256=cf637873695fcc02fab308f68608f0bbb90481332a2ff0a9597c7c9fd97d363d"
			DOWNLOAD_EXTRACT_TIMESTAMP "1"
			CONFIGURE_COMMAND 	"sh" "<SOURCE_DIR>/configure" "--prefix=<INSTALL_DIR>" "--enable-tests=no" "--enable-demos=no" "--disable-doxygen-doc" "CFLAGS=-I${spirv_headers_install_dir}/include -I${vulkan_headers_install_dir}/include -I${vulkan_loader_install_dir}/include" "LDFLAGS=-L${vulkan_loader_install_dir}/lib" "BISON=${BISON_EXECUTABLE}"
			BUILD_COMMAND	 	"make"
			INSTALL_COMMAND	 	"make" "install"
		)
		ExternalProject_Get_property(vkd3d SOURCE_DIR)
		ExternalProject_Get_property(vkd3d INSTALL_DIR)
		install(DIRECTORY "${INSTALL_DIR}/lib/" DESTINATION "${CMAKE_INSTALL_LIBDIR}" FILES_MATCHING PATTERN "*.so*" PERMISSIONS ${chmod_0755})
		install(DIRECTORY "${INSTALL_DIR}/lib/" DESTINATION "${CMAKE_INSTALL_LIBDIR}" FILES_MATCHING PATTERN "*.dylib*" PERMISSIONS ${chmod_0755})
		install(FILES "${SOURCE_DIR}/COPYING" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/vkd3d")
	endif()
	if(LINUX)
		find_program(PATCHELF_BIN NAMES "patchelf" REQUIRED)
		install(CODE "file(GLOB so_paths \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/*so*\")\n  foreach(so_path \${so_paths})\n    if(NOT IS_SYMLINK \${so_path})\n      message(STATUS \"Adding \\\"\$ORIGIN\\\" to RPATH of \${so_path}\")\n      execute_process(COMMAND ${PATCHELF_BIN} \"\${so_path}\" --add-rpath \"\$ORIGIN\")\n    endif()\n  endforeach()")
	elseif(APPLE)
		# FIXME: Apple probably needs to do something similar as Linux, but using otool
	endif()
endif()
