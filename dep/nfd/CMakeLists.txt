set(nfd_libs "")

if(WIN32)
    set(nfd_plat nfd_win.cpp)
elseif(MACOSX)
    set(nfd_plat nfd_cocoa.m)
elseif(UNIX)
    set(nfd_plat nfd_gtk.c)

    find_package(GTK3)
    if(GTK3_FOUND)
        set(XGTK_INCLUDE_DIRS ${GTK3_INCLUDE_DIRS})
        set(XGTK_LIBRARIES ${GTK3_LIBRARIES})
    else()
        find_package(GTK2)
        if(GTK2_FOUND)
            set(XGTK_INCLUDE_DIRS ${GTK2_INCLUDE_DIRS})
            set(XGTK_LIBRARIES ${GTK2_LIBRARIES})
        endif()
    endif()

    include_directories(${XGTK_INCLUDE_DIRS})

    message(STATUS "Using GTK includes: ${XGTK_INCLUDE_DIRS}")
    message(STATUS "Using GTK libs: ${XGTK_LIBRARIES}")

    set(nfd_libs "${XGTK_LIBRARIES}")
else()
    message(SEND_ERROR "nativefiledialog: Unsupported platform")
endif()

if(MSVC)
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()

set(nfd_src
    common.h
    nfd_common.c
    nfd_common.h
    ../nfd.h
    ${nfd_plat}
)

add_library(nfd ${nfd_src})
target_link_libraries(nfd ${nfd_libs})
