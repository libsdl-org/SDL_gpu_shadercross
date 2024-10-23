# CMake Project for SDL_gpu_shadercross - Simple DirectMedia Layer Shader Cross Compiler
# Written by @thatcosmonaut
cmake_minimum_required(VERSION 3.5)
project(SDL_gpu_shadercross C)
find_package(SDL3 REQUIRED)

# Options
option(BUILD_CLI "Build command line executable" ON)
option(BUILD_SHARED_LIBS "Build shared library" ON)

# Version
SET(LIB_MAJOR_VERSION "1")
SET(LIB_MINOR_VERSION "0")
SET(LIB_REVISION "0")
SET(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_REVISION}")

# Build Type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	# By default, we use Release
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE "Release" CACHE
		STRING "Choose the type of build." FORCE
	)

	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
		STRINGS "Debug" "Release" "RelWithDebInfo"
	)
endif()

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
	set(LOBJC "objc")
elseif(WIN32)
	# "SDL_gpu_shadercross.dll", not "libSDL_gpu_shadercross.dll"
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Source lists

file(GLOB SOURCE_FILES
	# Public Headers
	SDL_gpu_shadercross.h
	# Source Files
    src/SDL_gpu_shadercross.c
    src/spirv_cross_c.h
    src/spirv.h
)

if(BUILD_SHARED_LIBS)
    add_library(SDL_gpu_shadercross SHARED ${SOURCE_FILES})
else()
    add_library(SDL_gpu_shadercross STATIC ${SOURCE_FILES})
endif()

# Build flags
if(NOT MSVC)
	set_property(TARGET SDL_gpu_shadercross PROPERTY COMPILE_FLAGS "-std=gnu99 -Wall -Wno-strict-aliasing -pedantic")
endif()

# SDL_gpu_shadercross folders as includes, for other targets to consume
target_include_directories(SDL_gpu_shadercross PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# MinGW builds should statically link libgcc
if(MINGW)
	target_link_libraries(SDL_gpu_shadercross PRIVATE -static-libgcc)
endif()

# Soname
set_target_properties(SDL_gpu_shadercross PROPERTIES OUTPUT_NAME "SDL_gpu_shadercross"
	VERSION ${LIB_VERSION}
	SOVERSION ${LIB_MAJOR_VERSION}
)

target_link_libraries(SDL_gpu_shadercross PRIVATE
    SDL3::SDL3
    SDL3::Headers
)

if(BUILD_CLI)
    file(GLOB CLI_SOURCES
        src/cli.c
    )

    add_executable(SDL_gpu_shadercross_cli ${CLI_SOURCES})

    if(BUILD_SHARED_LIBS)
        target_link_libraries(SDL_gpu_shadercross_cli PUBLIC SDL_gpu_shadercross)
    else()
        target_link_libraries(SDL_gpu_shadercross_cli PRIVATE SDL_gpu_shadercross)
    endif()
endif()
