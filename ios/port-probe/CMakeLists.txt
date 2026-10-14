cmake_minimum_required(VERSION 3.24)

project(KytyPS5IOSPortProbe LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR "This probe must be configured with CMAKE_SYSTEM_NAME=iOS")
endif()

if(NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(FATAL_ERROR "This probe must target iPhone ARM64")
endif()

add_library(kyty_ios_arm64_probe STATIC port_probe.cpp)
target_compile_features(kyty_ios_arm64_probe PUBLIC cxx_std_20)
target_compile_options(kyty_ios_arm64_probe PRIVATE -Wall -Wextra -Werror)
