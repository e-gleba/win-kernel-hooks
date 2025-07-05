cmake_minimum_required(VERSION 3.30)

add_library(hooks STATIC hooks.cxx)
target_include_directories(hooks PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(dll_main SHARED dll_main.cxx)
target_link_libraries(dll_main PRIVATE hooks)