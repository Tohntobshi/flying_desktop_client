cmake_minimum_required(VERSION 2.8)
project( ImGUI )

include_directories( ./include/imgui )

set(
  TARGETS
    imgui_draw.cpp imgui_impl_opengl3.cpp imgui_impl_sdl.cpp
    imgui_widgets.cpp imgui.cpp imgui_demo.cpp
)

add_library( imgui STATIC ${TARGETS} )



