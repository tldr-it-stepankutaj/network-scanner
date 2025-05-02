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
    # Use a custom command that doesn't fail if permissions are missing
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
    # Generate bash completion script if not found
    message(STATUS "Bash completion script not found, generating default")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/completion/network-scanner-completion.bash"
            "#!/bin/bash

_network-scanner_completions() {
  local cur=\"\${COMP_WORDS[COMP_CWORD]}\"
  local opts=\"--threads --mode --port --help --version\"

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
    # Generate zsh completion script if not found
    message(STATUS "Zsh completion script not found, generating default")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/completion/_network-scanner"
            "#compdef network-scanner

_arguments \\
  '--threads[Number of threads]:threads:_guard \"[0-9]*\" \"number\"' \\
  '--mode[Scan mode]:mode:(icmp tcp fallback)' \\
  '--port[TCP port]:port:_guard \"[0-9]*\" \"port number\"' \\
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
    # Create docs directory if it doesn't exist
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")

    # Generate man page if not found
    message(STATUS "Man page not found, generating default")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/docs/network-scanner.1"
            ".TH NETWORK-SCANNER 1 \"${PROJECT_BUILD_DATE}\" \"network-scanner v${PROJECT_VERSION}\" \"User Commands\"

.SH NAME
network-scanner \\- fast parallel ICMP/TCP ping scanner

.SH SYNOPSIS
.B network-scanner
[--threads N] [--mode icmp|tcp|fallback] [--port PORT]

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
.BR --help
Show this help message

.TP
.BR --version
Show version info

.SH EXAMPLES
.TP
network-scanner 10.0.0.0/24
.TP
network-scanner 192.168.1.0/16 --mode fallback --port 443 --threads 64

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

# Output information about the build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "NetworkAnalyzer Version: ${PROJECT_VERSION}")
message(STATUS "Build Date: ${PROJECT_BUILD_DATE}")
message(STATUS "Git Commit: ${GIT_COMMIT_HASH}")