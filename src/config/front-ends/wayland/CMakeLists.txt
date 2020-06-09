find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(WaylandPP IMPORTED_TARGET wayland-client++ wayland-client-extra++)

    if(WaylandPP_FOUND)

        add_library(front-end-wayland
            default_vdriver.h
            wayland.h
            wayland.cpp
            ../x/x_keycodes.cpp
        )
        target_include_directories(front-end-wayland PUBLIC .)
        target_link_libraries(front-end-wayland syn68k romlib PkgConfig::WaylandPP)

    endif()
endif()
