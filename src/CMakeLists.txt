set(src
    array2d.h
    image2d.cpp
    image2d.h
    util.cpp
    util.h
    texture.cpp
    texture.h
    textoolgui.cpp
)

add_executable(textoolgui ${src})
target_link_libraries(textoolgui dep imgui nfd ${SDL2_LIBRARY})
