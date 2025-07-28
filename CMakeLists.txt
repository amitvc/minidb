cmake_minimum_required(VERSION 3.22)
project(minidb)

set(CMAKE_CXX_STANDARD 17)


file(GLOB_RECURSE DB_SOURCES "src/*.cpp")
file(GLOB_RECURSE DB_HEADERS "src/*.h")

add_library(minidb ${DB_SOURCES} ${DB_HEADERS})


target_include_directories(minidb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)


enable_testing()
add_subdirectory(tests)