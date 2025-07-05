cmake_minimum_required(VERSION 3.26)

project(
    cxx_project
    VERSION 1.0.0
    DESCRIPTION "example winapi trampoline hooks"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH cmake/cfg)

find_package(clang_format CONFIG REQUIRED)
find_package(clang_tidy CONFIG REQUIRED)
find_package(cpplint CONFIG REQUIRED)
find_package(doxygen CONFIG REQUIRED)

add_subdirectory(src)
