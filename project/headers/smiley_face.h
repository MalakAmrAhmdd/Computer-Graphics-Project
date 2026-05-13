#ifndef LVL3_COMPUTER_GRAPHICS_SMILEY_FACE_H
#define LVL3_COMPUTER_GRAPHICS_SMILEY_FACE_H
#pragma once
#include <windows.h>

void DrawArc(HDC hdc, int cx, int cy, int rx, int ry,
             double tStart, double tEnd, COLORREF c);
void DrawSmileyHappy(HDC hdc, int cx, int cy, int R, COLORREF c);
void DrawSmileySad(HDC hdc, int cx, int cy, int R, COLORREF c);

#endif //LVL3_COMPUTER_GRAPHICS_SMILEY_FACE_H
