# CMake Project for SDL3_gpu_shadercross - Simple DirectMedia Layer Shader Cross Compiler
# Written by @thatcosmonaut
cmake_minimum_required(VERSION 3.22)

# Version
set(MAJOR_VERSION 3)
set(MINOR_VERSION 0)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION "3.1.3")

project(SDL3_gpu_shadercross LANGUAGES C VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}")

include(CMakeDependentOption)
include("${CMAKE_CURRENT_LIST_DIR}/cmake/PrivateSdlFunctions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sdlcpu.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sdlplatform.cmake")

find_package(SDL3 REQUIRED COMPONENTS SDL3-shared)

# Options
option(BUILD_STATIC "Build static library" ON)
option(BUILD_CLI "Build command line executable" ON)
cmake_dependent_option(BUILD_CLI_STATIC "Link CLI with static libraries" OFF "BUILD_CLI;BUILD_STATIC;TARGET SDL3::SDL3-static" OFF)
option(ENABLE_WERROR "Enable Werror" OFF)
option(ENABLE_INSTALL "Enable installation" OFF)
cmake_dependent_option(ENABLE_INSTALL_CPACK "Enable CPack installation" OFF "ENABLE_INSTALL" OFF)

sdl_calculate_derived_version_variables(${MAJOR_VERSION} ${MINOR_VERSION} ${MICRO_VERSION})

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
endif()

if(NOT MSVC)
	add_compile_options(-pedantic) # -Wno-strict-aliasing
endif()

# Source lists
set(SOURCE_FILES
	# Public Headers
	include/SDL3_gpu_shadercross/SDL_gpu_shadercross.h
	# Source Files
	src/SDL_gpu_shadercross.c
	src/spirv_cross_c.h
	src/spirv.h
)

add_library(SDL3_gpu_shadercross-shared SHARED ${SOURCE_FILES})
add_library(SDL3_gpu_shadercross::SDL3_gpu_shadercross ALIAS SDL3_gpu_shadercross-shared)

set_property(TARGET SDL3_gpu_shadercross-shared PROPERTY DEFINE_SYMBOL DLL_EXPORT)
sdl_add_warning_options(SDL3_gpu_shadercross-shared WARNING_AS_ERROR ${ENABLE_WERROR})
sdl_target_link_option_version_file(SDL3_gpu_shadercross-shared "${CMAKE_CURRENT_SOURCE_DIR}/src/SDL_gpu_shadercross.sym")

# Build flags
if(WIN32)
	target_sources(SDL3_gpu_shadercross-shared PRIVATE "src/version.rc")
	set_property(TARGET SDL3_gpu_shadercross-shared PROPERTY PREFIX "")
endif()
target_compile_features(SDL3_gpu_shadercross-shared PRIVATE c_std_99)

# SDL3_gpu_shadercross folders as includes, for other targets to consume
target_include_directories(SDL3_gpu_shadercross-shared PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_include_directories(SDL3_gpu_shadercross-shared PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# MinGW builds should statically link libgcc
if(MINGW)
	target_link_options(SDL3_gpu_shadercross-shared PRIVATE "-static-libgcc")
endif()

# Soname
set_target_properties(SDL3_gpu_shadercross-shared PROPERTIES
	OUTPUT_NAME "SDL3_gpu_shadercross"
	SOVERSION "${SO_VERSION_MAJOR}"
	VERSION "${SO_VERSION}"
)

target_link_libraries(SDL3_gpu_shadercross-shared PRIVATE
	SDL3::SDL3-shared
)

if(BUILD_STATIC)
	find_package(SDL3 REQUIRED COMPONENTS Headers)

	add_library(SDL3_gpu_shadercross-static STATIC ${SOURCE_FILES})
	add_library(SDL3_gpu_shadercross::SDL3_gpu_shadercross-static ALIAS SDL3_gpu_shadercross-static)
	sdl_add_warning_options(SDL3_gpu_shadercross-static WARNING_AS_ERROR ${ENABLE_WERROR})
	target_compile_features(SDL3_gpu_shadercross-static PRIVATE c_std_99)

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

if(NOT TARGET SDL3_gpu_shadercross::SDL3_gpu_shadercross)
	if(TARGET SDL3_gpu_shadercross-shared)
		add_library(SDL3_gpu_shadercross::SDL3_gpu_shadercross ALIAS SDL3_gpu_shadercross-shared)
	else()
		add_library(SDL3_gpu_shadercross::SDL3_gpu_shadercross ALIAS SDL3_gpu_shadercross-static)
	endif()
endif()

if(BUILD_CLI)
	add_executable(shadercross src/cli.c)

	if(BUILD_CLI_STATIC)
		target_link_libraries(shadercross PRIVATE SDL3_gpu_shadercross::SDL3_gpu_shadercross-static)
		target_link_libraries(shadercross PRIVATE SDL3::SDL3-static)
	else()
		target_link_libraries(shadercross PRIVATE SDL3_gpu_shadercross::SDL3_gpu_shadercross)
		target_link_libraries(shadercross PRIVATE SDL3::SDL3)
	endif()
endif()

if(ENABLE_INSTALL)
	include(GNUInstallDirs)
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
	install(
		FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3_gpu_shadercross/SDL_gpu_shadercross.h"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL3_gpu_shadercross" COMPONENT DEVEL
	)
	if(BUILD_CLI)
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

	file(RELATIVE_PATH SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${CMAKE_INSTALL_PREFIX}/${SDLGPUSHADERCROSS_PKGCONFIG_INSTALLDIR}" "${CMAKE_INSTALL_PREFIX}")
	string(REGEX REPLACE "[/]+$" "" SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
	set(SDL_PKGCONFIG_PREFIX "\${pcfiledir}/${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
	set(PC_REQUIRED "")
	set(PC_LIBS "")
	configure_file(cmake/sdl3-gpu-shadercross.pc.in sdl3-gpu-shadercross.pc @ONLY)

	# Always install sdl3-gpu-shadercross.pc file: libraries might be different between config modes
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdl3-gpu-shadercross.pc"
			DESTINATION "${SDLGPUSHADERCROSS_PKGCONFIG_INSTALLDIR}" COMPONENT devel)

	install(FILES "LICENSE.txt"
		DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
		COMPONENT library
	)
	if(ENABLE_INSTALL_CPACK)
		SDL_DetectTargetCPUArchitectures(SDL_CPU_NAMES)
		SDL_DetectCMakePlatform()
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
