cmake_minimum_required(VERSION 3.30)

add_library(memory_defender STATIC memory_defender.cxx)
target_include_directories(memory_defender PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(interceptor SHARED interceptor.cxx)
target_link_libraries(interceptor PRIVATE memory_defender)

add_library(memory_operations STATIC memory_operations.cxx)
target_include_directories(memory_operations PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(address_extractor address_extractor.cxx)
target_link_libraries(address_extractor PRIVATE memory_operations)