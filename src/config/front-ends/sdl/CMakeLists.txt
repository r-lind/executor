find_package(SDL)
find_library(X11_LIBRARY X11)
find_path (X11_INCLUDE_PATH X11/Xresource.h)

if(SDL_LIBRARY AND SDL_INCLUDE_DIR AND X11_LIBRARY AND Xext_LIBRARY)
    set(OS_SPECIFIC)

    if(UNIX AND NOT APPLE)
        set(OS_SPECIFIC sdlX.cpp)
    endif()

    if(UNIX AND APPLE)
        set(OS_SPECIFIC macosx_main.m)
    endif()

    if(WIN32)
        set(OS_SPECIFIC winmain.c)
    endif()

    add_library(front-end-sdl
        default_vdriver.h
        sdlevents.cpp
        sdl_mem.cpp
        sdlscrap.cpp
        sdlwin.cpp
        sdlwm.cpp
        syswm_map.cpp
        for_sam.h
        sdl.h
        SDL_bmp.h
        sdl_mem.h
        sdlevents.h
        sdlk_to_mkv.h
        sdlscrap.h
        sdlX.h
        syswm_map.h
        syswm_vars.h
        paramline.h
        paramline.c
        sdlsound.h
        sdlsound.cpp
        ${OS_SPECIFIC}
        )

    target_link_libraries(front-end-sdl syn68k ${SDL_LIBRARY} romlib)

    if(UNIX AND NOT APPLE)
        target_link_libraries(front-end-sdl ${X11_LIBRARY})
    endif()

    target_include_directories(front-end-sdl PUBLIC . PRIVATE ${SDL_INCLUDE_DIR}/.. ${X11_INCLUDE_PATH})
endif()