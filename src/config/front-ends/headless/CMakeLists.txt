add_library(front-end-headless
    default_vdriver.h
    headless.h
    headless.cpp
)
target_include_directories(front-end-headless
PUBLIC .)
target_link_libraries(front-end-headless syn68k)