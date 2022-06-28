CMAKE_MINIMUM_REQUIRED(VERSION 2.6...3.20)
PROJECT(texopt)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Recommended compiler flags that cmake doesn't set automatically
if(MSVC)
    # /MP: parallel builds
    # /GS-: disable security cookie (emits calls into vcrt)
    # /Oi: enable intrinsic functions
    # /fp:fast: -ffast-math
    set(EXTRA_COMPILE_FLAGS "/MP /GS- /Oi" CACHE STRING "Extra compiler flags for MSVC")
    
    option(MSVC_DEBUG_EDIT_AND_CONTINUE "MSVC: Enable edit+continue for debug builds?" TRUE)
    if(MSVC_DEBUG_EDIT_AND_CONTINUE)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set(EXTRA_COMPILE_FLAGS "" CACHE STRING "Extra compiler flags for GCC/Clang")
else()
    set(EXTRA_COMPILE_FLAGS "" CACHE STRING "Extra compiler flags")
endif()

if(EXTRA_COMPILE_FLAGS)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${AQUARIA_EXTRA_COMPILE_FLAGS}")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${AQUARIA_EXTRA_COMPILE_FLAGS}")
endif()

find_package(SDL2 REQUIRED)
message(STATUS "SDL include dir: ${SDL2_INCLUDE_DIR}")
include_directories(${SDL2_INCLUDE_DIR})
include_directories(dep/imgui)
include_directories(dep)

add_subdirectory(dep)
add_subdirectory(src)
