cmake_minimum_required(VERSION 3.10)

function(add_compilefail_tests filename)
    configure_file(${filename} ${CMAKE_BINARY_DIR}/${filename}.junk)
    file(READ ${filename} text)
    string(REGEX MATCHALL "#ifdef COMPILEFAIL_[a-zA-Z0-9_]+" ifdefs ${text})
    get_filename_component(base ${filename} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" base ${base})

    foreach(ifdef ${ifdefs})
        string(REPLACE "#ifdef COMPILEFAIL_" "" testcase ${ifdef})

        set(target compilefail_${base}_${testcase})
        add_library(${target} EXCLUDE_FROM_ALL ${filename})
        target_compile_definitions(${target} PRIVATE -DCOMPILEFAIL -DCOMPILEFAIL_${testcase})

        target_link_libraries(${target} gtest romlib)
        target_compile_definitions(${target} PRIVATE -DEXECUTOR)
    
        add_test(NAME ${base}.${testcase}
            COMMAND ${CMAKE_COMMAND} --build . --target ${target} --config $<CONFIGURATION>
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        set_tests_properties(${base}.${testcase} PROPERTIES WILL_FAIL TRUE)
    endforeach()

    set(target compilefail_${base}_preamble)
    add_library(${target} EXCLUDE_FROM_ALL ${filename})
    target_compile_definitions(${target} PRIVATE -DCOMPILEFAIL)

    target_link_libraries(${target} gtest romlib)
    target_compile_definitions(${target} PRIVATE -DEXECUTOR)

    add_test(NAME ${base}_preamble
        COMMAND ${CMAKE_COMMAND} --build . --target ${target} --config $<CONFIGURATION>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

endfunction()

if(CMAKE_SYSTEM_NAME MATCHES Retro.*)
    #add_subdirectory(googletest)
    #target_link_libraries(tests gtest)

    include_directories("googletest/googletest/include")
    include_directories("googletest/googletest/")
    add_definitions( -DGTEST_HAS_STREAM_REDIRECTION=0 -DGTEST_HAS_POSIX_RE=0 -DPATH_MAX=256)

    add_application(tests main.cpp files.cpp quickdraw.cpp googletest/googletest/src/gtest-all.cc size.r)
    set_target_properties(tests PROPERTIES LINK_FLAGS "-Wl,-gc-sections")

else()
    set(INSTALL_GTEST FALSE)
    include(GoogleTest)

    add_executable(tests main_executor.cpp files.cpp files_internal.cpp quickdraw.cpp guestvalues.cpp)
    add_subdirectory(googletest)
    target_link_libraries(tests gtest romlib)
    target_compile_definitions(tests PRIVATE -DEXECUTOR)
    if(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(tests PRIVATE -Wno-multichar)
    endif()

    gtest_discover_tests(tests)

    add_compilefail_tests(guestvalues.compfail.cpp)
endif()