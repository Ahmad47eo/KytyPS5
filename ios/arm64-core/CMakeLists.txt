cmake_minimum_required(VERSION 3.24)
project(KytyIOSArm64Core LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" OR NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(FATAL_ERROR "Kyty iOS core must be built for iOS ARM64")
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

add_library(kyty_ios_x64_interpreter STATIC x64_interpreter.cpp)
target_include_directories(kyty_ios_x64_interpreter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kyty_ios_x64_interpreter PUBLIC Zydis)
target_compile_options(kyty_ios_x64_interpreter PRIVATE -Wall -Wextra -Werror)
