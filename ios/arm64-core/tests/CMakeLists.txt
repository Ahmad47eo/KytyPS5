cmake_minimum_required(VERSION 3.24)
project(KytyIOSCoreTests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)
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

get_filename_component(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
add_library(kyty_ios_core_under_test STATIC
    "${CORE_DIR}/x64_interpreter.cpp"
)
target_include_directories(kyty_ios_core_under_test PUBLIC "${CORE_DIR}")
target_link_libraries(kyty_ios_core_under_test PUBLIC Zydis)
target_compile_options(kyty_ios_core_under_test PRIVATE -Wall -Wextra -Werror)

add_executable(kyty_ios_core_tests interpreter_tests.cpp)
target_link_libraries(kyty_ios_core_tests PRIVATE kyty_ios_core_under_test)
target_compile_options(kyty_ios_core_tests PRIVATE -Wall -Wextra -Werror)
add_test(NAME kyty_ios_core_tests COMMAND kyty_ios_core_tests)
