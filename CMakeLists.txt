cmake_minimum_required(VERSION 2.8)
project( FlyingClient )

set(
  TARGETS
    src/main.cpp
    src/connection.cpp
    src/frameCapturer.cpp
    src/controlsSender.cpp
    src/separateLoop.cpp
    src/window.cpp
    src/debugReceiver.cpp
)

include_directories(libs/rapidjson/include)
include_directories(libs/imgui/include)

add_executable( flying_client ${TARGETS} )

add_subdirectory( libs/imgui )

target_link_libraries(flying_client SDL2)
target_link_libraries(flying_client bluetooth)
target_link_libraries(flying_client pthread)
target_link_libraries(flying_client avcodec)
target_link_libraries(flying_client avutil)
target_link_libraries(flying_client swscale)
target_link_libraries(flying_client avformat)
target_link_libraries(flying_client imgui)
target_link_libraries(flying_client GLESv2)
