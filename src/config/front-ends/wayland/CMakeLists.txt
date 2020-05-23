add_library(front-end-wayland
    default_vdriver.h
    wayland.h
    wayland.cpp
)
target_include_directories(front-end-wayland PUBLIC .)
target_link_libraries(front-end-wayland syn68k romlib -lwayland-client++ -lwayland-client-extra++)