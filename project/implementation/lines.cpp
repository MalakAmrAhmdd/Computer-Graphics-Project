#include "../headers/lines.h"
#include "../headers/globals.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#define _USE_MATH_DEFINES

// DDA (Digital Differential Analyzer)
// Calculates how much x and y change per step using floating point.
// Steps = the longer axis (more horizontal → step in x, more vertical → step in y).
// Each iteration moves by xInc and yInc, rounding to nearest pixel.
void LineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    float xInc = (float)dx / steps; // how much to move in x each step
    float yInc = (float)dy / steps; // how much to move in y each step

    float x = x1, y = y1;
    for (int i = 0; i <= steps; i++)
    {
        SetPixel(hdc, (int)round(x), (int)round(y), c);
        x += xInc;
        y += yInc;
    }
}

// Midpoint Line Algorithm
// Uses only integer arithmetic (faster than DDA, no floating point).
// Decision variable d tells us whether to stay on the same row/column
// or move diagonally. d > 0 means move diagonally, d <= 0 stay straight.
void LineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x2 > x1) ? 1 : -1; // step direction in x (+1 right, -1 left)
    int sy = (y2 > y1) ? 1 : -1; // step direction in y (+1 down,  -1 up)
    int x = x1, y = y1;

    if (dx >= dy)
    {                           // more horizontal: step along x
        int d = 2 * dy - dx;    // initial decision variable
        int d1 = 2 * (dy - dx); // increment when d > 0 (diagonal step)
        int d2 = 2 * dy;        // increment when d <= 0 (horizontal step)
        for (int i = 0; i <= dx; i++)
        {
            SetPixel(hdc, x, y, c);
            if (d > 0)
            {
                y += sy;
                d += d1;
            }
            else
                d += d2;
            x += sx;
        }
    }
    else
    { // more vertical: step along y
        int d = 2 * dx - dy;
        int d1 = 2 * (dx - dy);
        int d2 = 2 * dx;
        for (int i = 0; i <= dy; i++)
        {
            SetPixel(hdc, x, y, c);
            if (d > 0)
            {
                x += sx;
                d += d1;
            }
            else
                d += d2;
            y += sy;
        }
    }
}

// Parametric Line Algorithm
// Uses parameter t that goes from 0.0 to 1.0.
// At t=0: point is (x1,y1). At t=1: point is (x2,y2).
// Formula: P(t) = P1 + t*(P2-P1) = (x1 + t*dx, y1 + t*dy)
void LineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    for (int i = 0; i <= steps; i++)
    {
        float t = (steps == 0) ? 0 : (float)i / steps; // t from 0 to 1
        int x = (int)round(x1 + t * dx);
        int y = (int)round(y1 + t * dy);
        SetPixel(hdc, x, y, c);
    }
}

// DrawLine — dispatcher that calls the correct algorithm based on activeAlgorithm.
// This is what WM_LBUTTONDOWN calls after getting both click points.
void DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    if (activeAlgorithm == "DDA")
        LineDDA(hdc, x1, y1, x2, y2, c);
    else if (activeAlgorithm == "MIDPOINT")
        LineMidpoint(hdc, x1, y1, x2, y2, c);
    else if (activeAlgorithm == "PARAMETRIC")
        LineParametric(hdc, x1, y1, x2, y2, c);
}
