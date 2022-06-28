set(src
    array2d.h
    image2d.cpp
    image2d.h
    util.h
    textoolgui.cpp
)

add_executable(textoolgui ${src})
target_link_libraries(textoolgui dep imgui ${SDL2_LIBRARY})
