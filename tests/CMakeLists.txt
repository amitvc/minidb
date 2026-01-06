cmake_minimum_required(VERSION 3.22)
project(minidb)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)

# Make the gtest and gtest_main targets available
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE DB_SOURCES "*.cpp")
file(GLOB_RECURSE DB_HEADERS "*.h")

add_executable(minidb_test ${DB_SOURCES} ${DB_HEADERS})

target_link_libraries(minidb_test PRIVATE minidb
        gtest
)

target_include_directories(minidb_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(minidb_test)