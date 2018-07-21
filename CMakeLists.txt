cmake_minimum_required(VERSION 3.9)
project(Executor)
cmake_policy(SET CMP0020 NEW) # Automatically link Qt executables to qtmain target on Windows.

if( NOT EXISTS "${CMAKE_SOURCE_DIR}/syn68k/.git" )
    message( FATAL_ERROR
        "The git submodule 'syn68k' is not available. Please run\n"
        "git submodule update --init --recursive" )
endif()

add_subdirectory(syn68k)
add_subdirectory(PowerCore)
add_subdirectory(cxmon)
add_subdirectory(src)
