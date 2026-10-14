cmake_minimum_required(VERSION 3.24)

project(KytyPS5IOSPortDependencies LANGUAGES C CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR "This dependency probe must target iOS")
endif()

if(NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(FATAL_ERROR "This dependency probe must target iPhone ARM64")
endif()

include(FetchContent)

set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_DOXYGEN OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_DECODER ON CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_ENCODER OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_FORMATTER OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    zydis
    GIT_REPOSITORY https://github.com/zyantific/zydis.git
    GIT_TAG 120e0e705f8e3b507dc49377ac2879979f0d545c
    GIT_SUBMODULES_RECURSE TRUE
    GIT_SHALLOW FALSE
)

FetchContent_MakeAvailable(zydis)

if(NOT TARGET Zydis)
    message(FATAL_ERROR "Zydis target was not created; check upstream CMake options")
endif()
