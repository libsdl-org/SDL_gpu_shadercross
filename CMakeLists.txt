# CMake Project for SDL_gpu_shadercross - Simple DirectMedia Layer Shader Cross Compiler
# Written by @thatcosmonaut
cmake_minimum_required(VERSION 3.22)

# Version
set(MAJOR_VERSION "1")
set(MINOR_VERSION "0")
set(MICRO_VERSION "0")
set(SDL_REQUIRED_VERSION "3.1.3")

project(SDL_gpu_shadercross LANGUAGES C VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/PrivateSdlFunctions.cmake")
include(CMakeDependentOption)

find_package(SDL3 REQUIRED COMPONENTS SDL3-shared)

# Options
option(BUILD_STATIC "Build static library" ON)
option(BUILD_CLI "Build command line executable" ON)
option(ENABLE_WERROR "Enable Werror" OFF)
option(ENABLE_INSTALL "Enable installation" OFF)
cmake_dependent_option(BUILD_CLI_STATIC "Link CLI with static libraries" OFF "BUILD_CLI;BUILD_STATIC;TARGET SDL3::SDL3-static" OFF)

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
	include/SDL_gpu_shadercross.h
	# Source Files
	src/SDL_gpu_shadercross.c
	src/spirv_cross_c.h
	src/spirv.h
)

add_library(SDL_gpu_shadercross-shared SHARED ${SOURCE_FILES})
add_library(SDL_gpu_shadercross::SDL_gpu_shadercross ALIAS SDL_gpu_shadercross-shared)

set_property(TARGET SDL_gpu_shadercross-shared PROPERTY DEFINE_SYMBOL DLL_EXPORT)
sdl_add_warning_options(SDL_gpu_shadercross-shared WARNING_AS_ERROR ${ENABLE_WERROR})
sdl_target_link_option_version_file(SDL_gpu_shadercross-shared "${CMAKE_CURRENT_SOURCE_DIR}/src/SDL_gpu_shadercross.sym")

# Build flags
if(WIN32)
	target_sources(SDL_gpu_shadercross-shared PRIVATE "src/version.rc")
	set_property(TARGET SDL_gpu_shadercross-shared PROPERTY PREFIX "")
endif()
target_compile_features(SDL_gpu_shadercross-shared PRIVATE c_std_99)

# SDL_gpu_shadercross folders as includes, for other targets to consume
target_include_directories(SDL_gpu_shadercross-shared PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# MinGW builds should statically link libgcc
if(MINGW)
	target_link_libraries(SDL_gpu_shadercross-shared PRIVATE -static-libgcc)
endif()

# Soname
set_target_properties(SDL_gpu_shadercross-shared PROPERTIES OUTPUT_NAME "SDL_gpu_shadercross"
	SOVERSION "${SO_VERSION_MAJOR}"
	VERSION "${SO_VERSION}"
)

target_link_libraries(SDL_gpu_shadercross-shared PRIVATE
	SDL3::SDL3-shared
)

if(BUILD_STATIC)
	find_package(SDL3 REQUIRED COMPONENTS Headers)

	add_library(SDL_gpu_shadercross-static STATIC ${SOURCE_FILES})
	add_library(SDL_gpu_shadercross::SDL_gpu_shadercross-static ALIAS SDL_gpu_shadercross-static)
	sdl_add_warning_options(SDL_gpu_shadercross-static WARNING_AS_ERROR ${ENABLE_WERROR})
	target_compile_features(SDL_gpu_shadercross-static PRIVATE c_std_99)

	if(NOT MSVC)
		set_property(TARGET SDL_gpu_shadercross-static PROPERTY OUTPUT_NAME "SDL_gpu_shadercross")
	endif()

	# SDL_gpu_shadercross folders as includes, for other targets to consume
	target_include_directories(SDL_gpu_shadercross-static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

	target_link_libraries(SDL_gpu_shadercross-static PUBLIC
		SDL3::Headers
	)
endif()

if(NOT TARGET SDL_gpu_shadercross::SDL_gpu_shadercross)
	if(TARGET SDL_gpu_shadercross-shared)
		add_library(SDL_gpu_shadercross::SDL_gpu_shadercross ALIAS SDL_gpu_shadercross-shared)
	else()
		add_library(SDL_gpu_shadercross::SDL_gpu_shadercross ALIAS SDL_gpu_shadercross-static)
	endif()
endif()

if(BUILD_CLI)
	add_executable(shadercross src/cli.c)

	if(BUILD_CLI_STATIC)
		target_link_libraries(shadercross PRIVATE SDL_gpu_shadercross::SDL_gpu_shadercross-static)
		target_link_libraries(shadercross PRIVATE SDL3::SDL3-static)
	else()
		target_link_libraries(shadercross PRIVATE SDL_gpu_shadercross::SDL_gpu_shadercross)
		target_link_libraries(shadercross PRIVATE SDL3::SDL3)
	endif()
endif()

if(ENABLE_INSTALL)
	if(WIN32 AND NOT MINGW)
		set(INSTALL_CMAKEDIR_ROOT_DEFAULT "cmake")
	else()
		set(INSTALL_CMAKEDIR_ROOT_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake")
	endif()
	set(SDLGPUSHADERCROSS_INSTALL_CMAKEDIR_ROOT "${INSTALL_CMAKEDIR_ROOT_DEFAULT}" CACHE STRING "Root folder where to install SDL_gpu_shadercross cmake related files (SDL_gpu_shadercross subfolder for MSVC projects)")
	set(SDLGPUSHADERCROSS_PKGCONFIG_INSTALLDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

	if(WIN32 AND NOT MINGW)
		set(SDLGPUSHADERCROSS_INSTALL_CMAKEDIR "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR_ROOT}")
	else()
		set(SDLGPUSHADERCROSS_INSTALL_CMAKEDIR "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR_ROOT}/SDL_gpu_shadercross")
	endif()

	include(GNUInstallDirs)
	if(TARGET SDL_gpu_shadercross-shared)
		install(TARGETS SDL_gpu_shadercross-shared EXPORT SDL_gpu_shadercross-shared-export
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
			RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
		)
		install(EXPORT SDL_gpu_shadercross-shared-export
			FILE SDL_gpu_shadercross-shared-targets.cmake
			NAMESPACE SDL_gpu_shadercross::
			DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
			COMPONENT devel
		)
	endif()
	if(TARGET SDL_gpu_shadercross-static)
		install(TARGETS SDL_gpu_shadercross-static EXPORT SDL_gpu_shadercross-static-export
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
		)
		install(EXPORT SDL_gpu_shadercross-static-export
			FILE SDL_gpu_shadercross-static-targets.cmake
			NAMESPACE SDL_gpu_shadercross::
			DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
			COMPONENT devel
		)
	endif()
	install(
		FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL_gpu_shadercross.h"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT DEVEL
	)

	include(CMakePackageConfigHelpers)
	configure_package_config_file(cmake/SDL_gpu_shadercrossConfig.cmake.in SDL_gpu_shadercrossConfig.cmake
		NO_SET_AND_CHECK_MACRO
		INSTALL_DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
	)
	write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL_gpu_shadercrossConfigVersion.cmake"
		COMPATIBILITY AnyNewerVersion
	)
	install(
		FILES
			"${CMAKE_CURRENT_BINARY_DIR}/SDL_gpu_shadercrossConfig.cmake"
			"${CMAKE_CURRENT_BINARY_DIR}/SDL_gpu_shadercrossConfigVersion.cmake"
		DESTINATION "${SDLGPUSHADERCROSS_INSTALL_CMAKEDIR}"
		COMPONENT devel
	)

	file(RELATIVE_PATH SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${CMAKE_INSTALL_PREFIX}/${SDLGPUSHADERCROSS_PKGCONFIG_INSTALLDIR}" "${CMAKE_INSTALL_PREFIX}")
	string(REGEX REPLACE "[/]+$" "" SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
	set(SDL_PKGCONFIG_PREFIX "\${pcfiledir}/${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
	set(PC_REQUIRED "")
	set(PC_LIBS "")
	configure_file(cmake/sdl-gpu-shadercross.pc.in sdl-gpu-shadercross.pc @ONLY)

	# Always install sdl-gpu-shadercross.pc file: libraries might be different between config modes
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdl-gpu-shadercross.pc"
			DESTINATION "${SDLGPUSHADERCROSS_PKGCONFIG_INSTALLDIR}" COMPONENT devel)

	install(FILES "LICENSE.txt"
		DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
		COMPONENT library
	)
endif()
