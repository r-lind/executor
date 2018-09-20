cmake_minimum_required(VERSION 3.10)
project(Executor)

set(CMAKE_CXX_STANDARD 17)

cmake_policy(SET CMP0020 NEW) # Automatically link Qt executables to qtmain target on Windows.

if( NOT EXISTS "${CMAKE_SOURCE_DIR}/syn68k/.git" )
    message( FATAL_ERROR
        "The git submodule 'syn68k' is not available. Please run\n"
        "git submodule update --init --recursive" )
endif()

enable_testing()

add_subdirectory(syn68k)
add_subdirectory(PowerCore)
add_subdirectory(cxmon)
add_subdirectory(src)
add_subdirectory(tests)
