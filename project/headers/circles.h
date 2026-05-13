#ifndef LVL3_COMPUTER_GRAPHICS_CIRCLES_H
#define LVL3_COMPUTER_GRAPHICS_CIRCLES_H

#pragma once
#include <windows.h>

void CircleDirect(HDC hdc, int xc, int yc, int r, COLORREF c);
void CirclePolar(HDC hdc, int xc, int yc, int r, COLORREF c);
void CircleIterPolar(HDC hdc, int xc, int yc, int r, COLORREF c);
void CircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF c);
void CircleModMid(HDC hdc, int xc, int yc, int r, COLORREF c);
void CircleDraw(HDC hdc, int cx, int cy, int r, COLORREF c);

#endif //LVL3_COMPUTER_GRAPHICS_CIRCLES_H
