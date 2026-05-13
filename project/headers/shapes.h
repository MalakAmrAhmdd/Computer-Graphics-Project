#ifndef LVL3_COMPUTER_GRAPHICS_SHAPES_H
#define LVL3_COMPUTER_GRAPHICS_SHAPES_H

#pragma once
#include <windows.h>
#include <vector>
#include <string>

struct Shape {
    std::string type;
    std::vector<int> params;
    COLORREF color;
};

//extern std::vector<Shape> shapes;

#endif //LVL3_COMPUTER_GRAPHICS_SHAPES_H
