cmake_minimum_required(VERSION 3.30)

add_library(memory_defender STATIC memory_defender.cxx)
target_include_directories(memory_defender PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(interceptor SHARED interceptor.cxx)
target_link_libraries(interceptor PRIVATE memory_defender)