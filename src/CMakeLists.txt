set(common_src
    array2d.h
    image2d.cpp
    image2d.h
    util.cpp
    util.h
    texture.cpp
    texture.h
)

add_library(common ${common_src})
target_link_libraries(common dep)

add_executable(textoolgui textoolgui.cpp)
target_link_libraries(textoolgui common imgui nfd ${SDL2_LIBRARY})

set(texpack_src
    accessor2d.h
    algo2d.h
    texpack.cpp
    polygon.cpp
    polygon.h
)

add_executable(texpack ${texpack_src})
target_link_libraries(texpack common ${SDL2_LIBRARY})
