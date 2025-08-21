cmake_minimum_required(VERSION 3.22)
project(minidb)

set(CMAKE_CXX_STANDARD 17)


file(GLOB_RECURSE DB_SOURCES "src/*.cpp")
file(GLOB_RECURSE DB_HEADERS "src/*.h")

add_library(minidb ${DB_SOURCES} ${DB_HEADERS}
        src/sql/ast.h)


target_include_directories(minidb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(minidb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include)


# Documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    # Set Doxygen configuration options
    set(DOXYGEN_PROJECT_NAME "MiniDB")
    set(DOXYGEN_PROJECT_BRIEF "A SQL-compliant database written in C++")
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
    set(DOXYGEN_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_HTML_OUTPUT "html")
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(DOXYGEN_FILE_PATTERNS "*.h *.hpp *.cpp *.cc *.cxx")
    set(DOXYGEN_EXCLUDE_PATTERNS "*/build/* */cmake-build-*/* */tests/*")
    set(DOXYGEN_COLLABORATION_GRAPH YES)
    set(DOXYGEN_HAVE_DOT YES)
    set(DOXYGEN_UML_LOOK YES)
    set(DOXYGEN_TEMPLATE_RELATIONS YES)
    set(DOXYGEN_INCLUDE_GRAPH YES)
    set(DOXYGEN_INCLUDED_BY_GRAPH YES)
    set(DOXYGEN_CALL_GRAPH YES)
    set(DOXYGEN_CALLER_GRAPH YES)

    # Configure Doxyfile
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    
    # Create Doxygen target
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
    message(STATUS "Documentation will be generated in: ${DOXYGEN_OUTPUT_DIRECTORY}")
    message(STATUS "Run 'make docs' or 'cmake --build . --target docs' to generate documentation")
else()
    message(WARNING "Doxygen not found - documentation target will not be available")
endif()

enable_testing()
add_subdirectory(tests)