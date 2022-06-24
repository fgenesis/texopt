set(src
	imconfig.h
	imgui.cpp
	imgui.h
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_impl_sdl.cpp
	imgui_impl_sdl.h
	imgui_impl_sdlrenderer.cpp
	imgui_impl_sdlrenderer.h
	imgui_internal.h
	imgui_tables.cpp
	imgui_widgets.cpp
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
)

add_library(imgui ${src})
target_link_libraries(imgui ${SDL2_LIBRARY})
