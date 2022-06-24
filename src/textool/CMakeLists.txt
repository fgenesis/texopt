set(src
    main.cpp
)

add_executable(textoolgui ${src})
target_link_libraries(textoolgui dep imgui ${SDL2_LIBRARY})
