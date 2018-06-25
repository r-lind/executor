if(CMAKE_SYSTEM_NAME MATCHES Retro.*)
    #add_subdirectory(googletest)
    #target_link_libraries(tests gtest)

    include_directories("googletest/googletest/include")
    include_directories("googletest/googletest/")
    add_definitions( -DGTEST_HAS_STREAM_REDIRECTION=0 -DGTEST_HAS_POSIX_RE=0 -DPATH_MAX=256)

    add_application(tests main.cpp files.cpp googletest/googletest/src/gtest-all.cc CONSOLE)
    set_target_properties(tests PROPERTIES LINK_FLAGS "-Wl,-gc-sections")

else()

endif()