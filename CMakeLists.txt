cmake_minimum_required(VERSION 3.10...3.15)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source build not allowed (for your own good)")
endif()

# Use packaged CMake modules for use with find_package(...), if and as needed
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(Executor2000 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    add_compile_definitions(
        # Prevent currently-non-compiling assembly functions from being included 
        NO_FAST_CC_FUNCS

        # Don't warn if standard C library functions get used.
        # The alternatives suggested by Microsoft are an *optional* part of the
        # standard and many of them are actually not useful for us.
        _CRT_SECURE_NO_WARNINGS

        # Dont't warn if POSIX functions are used by their proper name.
        # There is really no need to rename POSIX standard functions that
        # aren't defined in C standard headers anyway and thus aren't subject
        # to the C standard's rules for naming non-standard identifiers.
        # Microsoft should know better, after all they're supplying <windows.h> as well...
        _CRT_NONSTDC_NO_WARNINGS
    )
endif()

if( NOT EXISTS "${CMAKE_SOURCE_DIR}/syn68k/.git" )
    message( FATAL_ERROR
        "The git submodule 'syn68k' is not available. Please run\n"
        "git submodule update --init --recursive" )
endif()

enable_testing()

set(Boost_USE_STATIC_LIBS YES)

add_library(lmdb
    lmdb/libraries/liblmdb/lmdb.h lmdb/libraries/liblmdb/mdb.c
    lmdb/libraries/liblmdb/midl.h lmdb/libraries/liblmdb/midl.c
    lmdbxx/lmdb++.h)
target_include_directories(lmdb PUBLIC lmdb/libraries/liblmdb lmdbxx)

include(cmrc/CMakeRC.cmake)
cmrc_add_resource_library(resources
        WHENCE res
        res/System
        res/System.ad
        res/Browser
        res/Browser.ad
        res/Printer
        res/Printer.ad
        res/godata.sav
        res/printdef.ini
        res/printers.ini

        res/about/1_License.txt
        res/about/2_Credits.txt
        res/about/3_Tips.txt
    )

add_subdirectory(syn68k)
add_subdirectory(PowerCore)
add_subdirectory(cxmon)
add_subdirectory(src)
add_subdirectory(tests)

set(CPACK_GENERATOR "TBZ2")
set(CPACK_PACKAGE_CONTACT "Wolfgang Thaller <wolfgang.thaller@gmx.net>")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

if(APPLE)
list(APPEND CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)
