#include "../headers/circles.h"
#include "../headers/globals.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#define _USE_MATH_DEFINES

void CircleDraw(HDC hdc, int cx, int cy, int r, COLORREF c) {
    if (activeAlgorithm == "MID")
        CircleMidpoint(hdc, cx, cy, r, c);
    else if (activeAlgorithm == "Modified_Midpoint")
        CircleModMid(hdc, cx, cy, r, c);
    else if (activeAlgorithm == "DIRECT")
        CircleDirect(hdc, cx, cy, r, c);
    else if (activeAlgorithm == "POLAR")
        CirclePolar(hdc, cx, cy, r, c);
    else if (activeAlgorithm == "Iterative_POLAR")
        CircleIterPolar(hdc, cx, cy, r, c);

}

//Draw with the 8 points of symmetry
void drawPoints(HDC hdc, int xc, int yc, int x, int y, COLORREF c) {
    SetPixel(hdc, xc + x, yc + y, c);
    SetPixel(hdc, xc - x, yc + y, c);
    SetPixel(hdc, xc - x, yc - y, c);
    SetPixel(hdc, xc + x, yc - y, c);
    SetPixel(hdc, xc + y, yc + x, c);
    SetPixel(hdc, xc + y, yc - x, c);
    SetPixel(hdc, xc - y, yc - x, c);
    SetPixel(hdc, xc - y, yc + x, c);
}

void CircleDirect(HDC hdc, int xc, int yc, int R, COLORREF c) {
    int x = 0;
    int y = R;
    drawPoints(hdc, xc, yc, x, y, c);

    while (x < y) {
        x++;
        y = round(sqrt((pow(R, 2)) - (pow(x, 2))));
        drawPoints(hdc, xc, yc, x, y, c);
    }

}

//using first order difference (DDA)
void CircleMidpoint(HDC hdc, int xc, int yc, int R, COLORREF c) {
    int x = 0;
    int y = R;
    int d = 1 - R;
    drawPoints(hdc, xc, yc, x, y, c);

    while (x < y) {
        if (d < 0) {
            d += 2 * x + 3;
        }
        else {
            d += 2 * x - 2 * y + 5;
            y--;
        }
        x++;
        drawPoints(hdc, xc, yc, x, y, c);
    }
}

//using second order (fewer calculations, no multiplication only addition)
void CircleModMid(HDC hdc, int xc, int yc, int R, COLORREF c) {
    int x = 0;
    int y = R;
    int d = 1 - R;
    int c1 = 3, c2 = 5 - (2 * R);

    drawPoints(hdc, xc, yc, x, y, c);

    while (x < y) {
        if (d < 0) {
            d += c1;
            c2 += 2;
        }
        else {
            d += c2;
            c2 += 4;
            y--;
        }
        x++;
        c1 += 2;
        drawPoints(hdc, xc, yc, x, y, c);
    }
}

void CirclePolar(HDC hdc, int xc, int yc, int R, COLORREF c) {
    int x = R;
    int y = 0;
    double theta = 0;
    double d_theta = 1.0 / R;

    drawPoints(hdc, xc, yc, x, y, c);

    while (x > y) {
        theta += d_theta;
        x = (int)round(R * cos(theta));
        y = (int)round(R * sin(theta));
        drawPoints(hdc, xc, yc, x, y, c);
    }
}

void CircleIterPolar(HDC hdc, int xc, int yc, int R, COLORREF c) {
    double x = R;
    double y = 0;
    double d_theta = 1.0 / R;
    double cosine = cos(d_theta);
    double sine = sin(d_theta);

    drawPoints(hdc, xc, yc, (int)round(x), (int)round(y), c);

    while (x > y) {
        double x1 = x * cosine - y * sine;
        y = x * sine + y * cosine;
        x = x1;
        drawPoints(hdc, xc, yc, (int)round(x), (int)round(y), c);

    }

}
