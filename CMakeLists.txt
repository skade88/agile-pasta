cmake_minimum_required(VERSION 3.16)

project(agile-pasta VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable multithreading
find_package(Threads REQUIRED)

# Include directories
include_directories(include)
include_directories(external/indicators/include)

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

# Source files
set(SOURCES
    src/main.cpp
    src/command_line_parser.cpp
    src/file_scanner.cpp
    src/psv_parser.cpp
    src/database.cpp
    src/query_engine.cpp
    src/transformation_engine.cpp
    src/csv_writer.cpp
    src/progress_manager.cpp
)

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
)

# Create executable
add_executable(agile-pasta ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(agile-pasta PRIVATE Threads::Threads)

# Filesystem library for older compilers
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    target_link_libraries(agile-pasta PRIVATE stdc++fs)
endif()

# Set output directory
set_target_properties(agile-pasta PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS agile-pasta DESTINATION bin)