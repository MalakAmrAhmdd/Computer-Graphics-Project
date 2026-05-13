cmake_minimum_required(VERSION 3.27)
project(lvl3_Computer_Graphics)

set(CMAKE_CXX_STANDARD 23)

add_executable(lvl3_Computer_Graphics project/main.cpp
        project/implementation/ellipse.cpp
        project/implementation/circles.cpp
        project/implementation/lines.cpp
        project/implementation/curves.cpp
        project/implementation/filling.cpp
        project/implementation/globals.cpp
        project/implementation/smiley_face.cpp
        project/implementation/clipping.cpp)


target_link_libraries(lvl3_Computer_Graphics
        gdi32
        user32
)
