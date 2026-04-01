cmake_minimum_required(VERSION 3.10)
project(NetworkAnalyzer VERSION 2.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build unit tests" OFF)

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
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Library sources (everything except main.cpp)
set(LIB_SOURCES
        src/icmp.cpp
        src/tcp.cpp
        src/utils.cpp
        src/thread_pool.cpp
        src/scanner.cpp
        src/device_identifier.cpp
        src/network_info.cpp
)

# Create static library for reuse by tests
add_library(network-analyzer-lib STATIC ${LIB_SOURCES})
target_link_libraries(network-analyzer-lib
        PUBLIC
        ${CURL_LIBRARIES}
        Threads::Threads
)
target_include_directories(network-analyzer-lib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CURL_INCLUDE_DIRS}
)

# Option to statically link libstdc++/libgcc (fixes GLIBCXX_3.4.30 issue on older distros)
option(STATIC_LIBCXX "Statically link libstdc++ and libgcc" OFF)

# Create executable
add_executable(network-analyzer src/main.cpp)
target_link_libraries(network-analyzer PRIVATE network-analyzer-lib)

if(STATIC_LIBCXX AND UNIX AND NOT APPLE)
    target_link_options(network-analyzer PRIVATE -static-libstdc++ -static-libgcc)
endif()

# Include GNUInstallDirs for standard installation directories
include(GNUInstallDirs)

# Installation rules
install(TARGETS network-analyzer
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create symlink from network-analyzer to network-scanner in binary directory
if(UNIX)
    install(CODE "
        message(STATUS \"Creating symlink ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/network-scanner\")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                network-analyzer
                network-scanner
            WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
            RESULT_VARIABLE LINK_RESULT
            ERROR_VARIABLE LINK_ERROR
        )
        if(LINK_RESULT)
            message(WARNING \"Failed to create symlink: \${LINK_ERROR}\")
        endif()
    ")
endif()

# Check for man page and install if exists
set(MAN_PAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/docs/network-scanner.1")
if(EXISTS ${MAN_PAGE_PATH})
    install(
            FILES ${MAN_PAGE_PATH}
            DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    )
else()
    message(STATUS "Man page not found at ${MAN_PAGE_PATH}, skipping installation")
endif()

# Create completion directories if they don't exist
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/completion")

# Check for bash completion script and install if exists
set(BASH_COMPLETION_PATH "${CMAKE_CURRENT_SOURCE_DIR}/completion/network-scanner-completion.bash")
if(NOT EXISTS ${BASH_COMPLETION_PATH})
    message(STATUS "Bash completion script not found, generating default")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/completion/network-scanner-completion.bash"
            "#!/bin/bash

_network-scanner_completions() {
  local cur=\"\${COMP_WORDS[COMP_CWORD]}\"
  local opts=\"--threads --mode --port --timeout --json --no-color --verbose --debug --thorough --skip-scan --show-all --no-banner --no-clear --help --version\"

  # shellcheck disable=SC2207
  COMPREPLY=($(compgen -W \"\${opts}\" -- \"\$cur\"))

  # Add mode options
  if [[ \${cur} == --mode=* ]]; then
    local modes=\"icmp tcp fallback\"
    local prefix=\${cur%=*}=
    COMPREPLY=($(compgen -P \"\$prefix\" -W \"\$modes\" -- \"\${cur#*=}\"))
  fi
}

complete -F _network-scanner_completions network-scanner
")
    set(BASH_COMPLETION_PATH "${CMAKE_CURRENT_BINARY_DIR}/completion/network-scanner-completion.bash")
endif()

install(
        FILES ${BASH_COMPLETION_PATH}
        DESTINATION "${CMAKE_INSTALL_DATADIR}/bash-completion/completions"
        RENAME "network-scanner"
)

# Check for zsh completion script and install if exists
set(ZSH_COMPLETION_PATH "${CMAKE_CURRENT_SOURCE_DIR}/completion/_network-scanner")
if(NOT EXISTS ${ZSH_COMPLETION_PATH})
    message(STATUS "Zsh completion script not found, generating default")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/completion/_network-scanner"
            "#compdef network-scanner

_arguments \\
  '--threads[Number of threads]:threads:_guard \"[0-9]*\" \"number\"' \\
  '--mode[Scan mode]:mode:(icmp tcp fallback)' \\
  '--port[TCP port]:port:_guard \"[0-9]*\" \"port number\"' \\
  '--timeout[Timeout in ms]:timeout:_guard \"[0-9]*\" \"milliseconds\"' \\
  '--json[Output as JSON]' \\
  '--no-color[Disable colors]' \\
  '--verbose[Verbose output]' \\
  '--debug[Debug output]' \\
  '--thorough[Thorough scan]' \\
  '--skip-scan[Skip scanning]' \\
  '--show-all[Show all hosts]' \\
  '--no-banner[No ASCII banner]' \\
  '--no-clear[No screen clear]' \\
  '--help[Show help]' \\
  '--version[Show version]'
")
    set(ZSH_COMPLETION_PATH "${CMAKE_CURRENT_BINARY_DIR}/completion/_network-scanner")
endif()

install(
        FILES ${ZSH_COMPLETION_PATH}
        DESTINATION "${CMAKE_INSTALL_DATADIR}/zsh/site-functions"
)

# Check for man page and generate if not exists
set(MAN_PAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/docs/network-scanner.1")
if(NOT EXISTS ${MAN_PAGE_PATH})
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")

    message(STATUS "Man page not found, generating default")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/docs/network-scanner.1"
            ".TH NETWORK-SCANNER 1 \"${PROJECT_BUILD_DATE}\" \"network-scanner v${PROJECT_VERSION}\" \"User Commands\"

.SH NAME
network-scanner \\- fast parallel ICMP/TCP ping scanner

.SH SYNOPSIS
.B network-scanner
[--threads N] [--mode icmp|tcp|fallback] [--port PORT] [--timeout MS] [--json] [--no-color] [--verbose] [--debug]

.SH DESCRIPTION
.B network-scanner
is a high-performance ping scanner for large network ranges. It uses raw ICMP sockets or TCP connect to identify live hosts.

.SH OPTIONS
.TP
.BR --threads \" N\"
Number of parallel threads (default: CPU core count)

.TP
.BR --mode \" icmp|tcp|fallback\"
Choose scan method: ICMP, TCP connect or ICMP with TCP fallback

.TP
.BR --port \" PORT\"
TCP port used when mode is tcp or fallback (default: 80)

.TP
.BR --timeout \" MS\"
Probe timeout in milliseconds (default: 1000)

.TP
.BR --json
Output results as JSON (non-interactive mode)

.TP
.BR --no-color
Disable colored output

.TP
.BR --verbose
Show informational messages on stderr

.TP
.BR --debug
Show debug messages on stderr

.TP
.BR --thorough
Use thorough scanning for higher accuracy

.TP
.BR --show-all
Show all hosts including unconfirmed ones

.TP
.BR --help
Show this help message

.TP
.BR --version
Show version info

.SH EXAMPLES
.TP
network-scanner
.TP
network-scanner --mode tcp --port 443 --threads 64
.TP
network-scanner --json --no-color | jq .

.SH AUTHOR
Written by Stepan Kutaj (stepan.kutaj@tldr-it.com).

.SH LICENSE
GPL-3.0 License.
")
    set(MAN_PAGE_PATH "${CMAKE_CURRENT_BINARY_DIR}/docs/network-scanner.1")
endif()

install(
        FILES ${MAN_PAGE_PATH}
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
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

# Unit tests
if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(network-analyzer-tests
        tests/test_utils.cpp
        tests/test_network_info.cpp
        tests/test_icmp.cpp
    )
    target_link_libraries(network-analyzer-tests
        PRIVATE
        network-analyzer-lib
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(network-analyzer-tests)
endif()

# Output information about the build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "NetworkAnalyzer Version: ${PROJECT_VERSION}")
message(STATUS "Build Date: ${PROJECT_BUILD_DATE}")
message(STATUS "Git Commit: ${GIT_COMMIT_HASH}")
if(BUILD_TESTS)
    message(STATUS "Unit tests: ENABLED")
endif()
