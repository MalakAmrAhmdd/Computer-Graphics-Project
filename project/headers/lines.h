#ifndef LVL3_COMPUTER_GRAPHICS_LINES_H
#define LVL3_COMPUTER_GRAPHICS_LINES_H
#pragma once
#include <windows.h>

void LineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
void LineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
void LineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
void DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);


#endif //LVL3_COMPUTER_GRAPHICS_LINES_H
