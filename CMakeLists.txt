cmake_minimum_required(VERSION 3.16)

project(agile-pasta VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable multithreading
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings for Visual Studio 2022
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    endif()
else()
    # Linux-specific settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    endif()
endif()

# Header files
set(HEADERS
    include/command_line_parser.h
    include/file_scanner.h
    include/psv_parser.h
    include/database.h
    include/query_engine.h
    include/transformation_engine.h
    include/csv_writer.h
    include/progress_manager.h
    include/custom_progress_bar.h
)

# Create executable with just main.cpp
add_executable(agile-pasta src/main.cpp ${HEADERS})

# Set output directory
set_target_properties(agile-pasta PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS agile-pasta DESTINATION bin)

# Enable testing only if requested
option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    include(CTest)
    enable_testing()

    # Find Google Test
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GTEST gtest)
        pkg_check_modules(GTEST_MAIN gtest_main)
    endif()

    if(NOT GTEST_FOUND)
        # Try to find system-installed Google Test
        find_package(GTest QUIET)
        if(GTest_FOUND OR GTEST_FOUND)
            set(GTEST_LIBRARIES ${GTEST_LIBRARIES} gtest)
            set(GTEST_MAIN_LIBRARIES ${GTEST_MAIN_LIBRARIES} gtest_main)
        else()
            message(WARNING "Google Test not found. Tests will not be built.")
            set(BUILD_TESTING OFF)
        endif()
    endif()
endif()

# Test executable
set(TEST_SOURCES
    tests/test_main.cpp
    tests/test_command_line_parser.cpp
    tests/test_psv_parser.cpp
    tests/test_database.cpp
    tests/test_query_engine.cpp
    tests/test_transformation_engine.cpp
    tests/test_csv_writer.cpp
    tests/test_file_scanner.cpp
    tests/test_integration.cpp
)

# Create separate object library for the main application code (excluding main.cpp)
set(LIB_SOURCES
    src/command_line_parser.cpp
    src/file_scanner.cpp
    src/psv_parser.cpp
    src/database.cpp
    src/query_engine.cpp
    src/transformation_engine.cpp
    src/csv_writer.cpp
    src/progress_manager.cpp
    src/custom_progress_bar.cpp
)

add_library(agile-pasta-lib STATIC ${LIB_SOURCES} ${HEADERS})
target_link_libraries(agile-pasta-lib PRIVATE Threads::Threads)

# Link filesystem library for older compilers
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    target_link_libraries(agile-pasta-lib PRIVATE stdc++fs)
endif()

# Update main executable to use the library
target_link_libraries(agile-pasta PRIVATE agile-pasta-lib)

# Create test executable only if BUILD_TESTING is enabled and Google Test is available
if(BUILD_TESTING AND (GTEST_FOUND OR GTest_FOUND))
    add_executable(agile-pasta-tests ${TEST_SOURCES})
    target_link_libraries(agile-pasta-tests 
        PRIVATE 
        agile-pasta-lib
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
        Threads::Threads
    )
    
    # Add tests to CTest
    add_test(NAME AgileProTastaUnitTests COMMAND agile-pasta-tests)
endif()