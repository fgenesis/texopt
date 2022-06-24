add_subdirectory(imgui)

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
)

add_library(dep ${src})

