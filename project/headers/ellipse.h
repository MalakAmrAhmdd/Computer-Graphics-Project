#ifndef LVL3_COMPUTER_GRAPHICS_ELLIPSE_H
#define LVL3_COMPUTER_GRAPHICS_ELLIPSE_H

#pragma once
#include <windows.h>

void EllipseDirect(HDC hdc, int xc, int yc, int x, int y, COLORREF c);
void EllipseMidpoint(HDC hdc, int xc, int yc, int x, int y, COLORREF c);
void EllipsePolar(HDC hdc, int xc, int yc, int x, int y, COLORREF c);
void EllipseDraw(HDC hdc, int xc, int yc, int x, int y, COLORREF c);


#endif //LVL3_COMPUTER_GRAPHICS_ELLIPSE_H
