cmake_minimum_required(VERSION 3.10)
project(NetworkAnalyzer VERSION 1.0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Get current date for build information
string(TIMESTAMP PROJECT_BUILD_DATE "%Y-%m-%d")

# Try to get Git commit hash if Git is available
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
endif()

if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

# Create version header file
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in"
        "${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp"
)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include  # Add generated files directory
)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Source files
set(SOURCES
        src/main.cpp
        src/icmp.cpp
        src/tcp.cpp
        src/utils.cpp
        src/thread_pool.cpp
        src/scanner.cpp
        src/device_identifier.cpp
        src/network_info.cpp
)

# Create executable
add_executable(network-analyzer ${SOURCES})

# Link libraries
target_link_libraries(network-analyzer
        PRIVATE
        ${CURL_LIBRARIES}
        Threads::Threads
)

# Include directories for the executable
target_include_directories(network-analyzer
        PRIVATE
        ${CURL_INCLUDE_DIRS}
)

# Include GNUInstallDirs for standard installation directories
include(GNUInstallDirs)

# Installation rules
install(TARGETS network-analyzer
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create symlink from network-analyzer to network-scanner in binary directory
if(UNIX)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/network-analyzer
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/network-scanner
    )")
endif()

# Install man page
install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/docs/network-scanner.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
)

# Install bash completion
install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/completion/network-scanner-completion.bash"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/bash-completion/completions"
        RENAME "network-scanner"
)

# Install zsh completion
install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/completion/_network-scanner"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/zsh/site-functions"
)

# Create uninstall target
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
)

add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

# Output information about the build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "NetworkAnalyzer Version: ${PROJECT_VERSION}")
message(STATUS "Build Date: ${PROJECT_BUILD_DATE}")
message(STATUS "Git Commit: ${GIT_COMMIT_HASH}")