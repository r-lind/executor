cmake_minimum_required(VERSION 3.12...3.17)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source build not allowed (for your own good)")
endif()

# Use packaged CMake modules for use with find_package(...), if and as needed
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(Executor2000 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    add_compile_definitions(
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
add_compile_definitions(BOOST_ALL_NO_LIB)

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

if(WIN32)
set(CPACK_GENERATOR "ZIP")
else()
set(CPACK_GENERATOR "TBZ2")
endif()

set(CPACK_PACKAGE_CONTACT "Wolfgang Thaller <wolfgang.thaller@gmx.net>")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

macro(install_qt5_plugin _qt_plugin_name _qt_plugins_var _prefix)
    get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
    if(EXISTS "${_qt_plugin_path}")
        get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
        get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
        get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
        set(_qt_plugin_dest "${_prefix}/${_qt_plugin_type}")
        install(FILES "${_qt_plugin_path}"
            DESTINATION "${_qt_plugin_dest}")
        set(${_qt_plugins_var}
            "${${_qt_plugins_var}};\$ENV{DEST_DIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
    else()
        message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
    endif()
endmacro()

find_package(Qt5 COMPONENTS Gui)
find_package(Boost)

set(DIRS)
list(APPEND DIRS ${CMAKE_BINARY_DIR})
list(APPEND DIRS ${Boost_LIBRARY_DIRS})
get_target_property(QT_GUI_LOCATION Qt5::Gui LOCATION)
get_filename_component(QT_GUI_LOCATION "${QT_GUI_LOCATION}" PATH)
list(APPEND DIRS ${QT_GUI_LOCATION})
if(WIN32)
    install_qt5_plugin("Qt5::QWindowsIntegrationPlugin" QT_PLUGINS "bin/")

    set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/executor.exe")
    install(CODE "include(BundleUtilities)\nfixup_bundle(\"${APPS}\" \"${QT_PLUGINS}\" \"${DIRS}\")")

    if(NOT MSVC)
        set(CPACK_SYSTEM_NAME win64-mingw)
    endif()
elseif(APPLE)
    install_qt5_plugin("Qt5::QCocoaIntegrationPlugin" QT_PLUGINS "Executor 2000.app/Contents/PlugIns")

    set(APPS "\${CMAKE_INSTALL_PREFIX}/Executor\ 2000.app")
    install(CODE "include(BundleUtilities)\nfixup_bundle(\"${APPS}\" \"${QT_PLUGINS}\" \"${DIRS}\")")

    set(CPACK_SYSTEM_NAME macOS)
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT ${CMAKE_SOURCE_DIR}/icon/CMakeDSStore.applescript)
endif()

if(APPLE)
list(APPEND CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)
