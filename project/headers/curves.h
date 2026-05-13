#ifndef LVL3_COMPUTER_GRAPHICS_CURVES_H
#define LVL3_COMPUTER_GRAPHICS_CURVES_H
#pragma once
#include <windows.h>
#include <string>
#include <vector>


void GetHermiteCoeff(double p0, double s0, double p1, double s1, double coeff[4]);
void DrawHermiteSeg(HDC hdc, int x0, int y0, int tx0, int ty0,
                    int x1, int y1, int tx1, int ty1, COLORREF c, int numpts = 200);
void DrawCardinalSpline(HDC hdc, POINT P[], int n, double tension, COLORREF c);

#endif //LVL3_COMPUTER_GRAPHICS_CURVES_H
