add_subdirectory(imgui)
add_subdirectory(nfd)

set(src
    rg_etc1.cpp
    rg_etc1.h
    libs.cpp
    stb_image.h
    stb_image_write.h
    stb_image_resize.h
    stb_rect_pack.h
    miniz.cpp
    miniz.h
    nfd.h
    polypartition.cpp
    polypartition.h
    tws_thread.h
    dt.h
    filesystem.cpp
    filesystem.h
    vec.h
)

add_library(dep ${src})

