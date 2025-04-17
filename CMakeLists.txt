cmake_minimum_required(VERSION 3.10)
project(network-scanner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# OS detection
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_compile_definitions(OS_MAC)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_compile_definitions(OS_LINUX)
endif()

# Source files
file(GLOB SOURCES "src/*.cpp")
add_executable(network-scanner ${SOURCES})

# Optional: clang-tidy integration
option(ENABLE_CLANG_TIDY "Enable clang-tidy checks" ON)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*")
    endif()
endif()

# Install binary
install(TARGETS network-scanner DESTINATION bin)

# Install man page
install(FILES docs/network-scanner.1 DESTINATION share/man/man1)

# Install completions
install(FILES completions/network-scanner.bash DESTINATION share/bash-completion/completions)
install(FILES completions/_network-scanner DESTINATION share/zsh/site-functions)