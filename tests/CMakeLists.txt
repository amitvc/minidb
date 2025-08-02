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

add_executable(minidb_test sql/lexer_test.cpp
        sql/parser_test.cpp
        sql/test_utils.h)

target_link_libraries(minidb_test PRIVATE minidb
        gtest_main
)


include(GoogleTest)
gtest_discover_tests(minidb_test)