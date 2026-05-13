#include "../headers/ellipse.h"
#include "../headers/globals.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#define _USE_MATH_DEFINES

using namespace std;

void EllipseDraw(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c) {
    if (activeAlgorithm == "Ellipse_Direct")
        EllipseDirect(hdc, xc, yc, rx, ry, c);
    else if (activeAlgorithm == "Ellipse_Midpoint")
        EllipseMidpoint(hdc, xc, yc, rx, ry, c);
    else if (activeAlgorithm == "Ellipse_Polar")
        EllipsePolar(hdc, xc, yc, rx, ry, c);
}

void ellipsePoints(HDC hdc, int xc, int yc, int x, int y, COLORREF c) {
    SetPixel(hdc, xc + x, yc + y, c);
    SetPixel(hdc, xc - x, yc + y, c);
    SetPixel(hdc, xc - x, yc - y, c);
    SetPixel(hdc, xc + x, yc - y, c);
}

// x^2/a^2 + y^2/b^2 = 1  (a: half width, b:half height)
//horizontal ellipse (x-h)^2/a^2 + (y-k)^2/b^2 = 1
// Vertical ellipse (x-k)^2/a^2 + (y-h)^2/b^2 = 1
//(h,K) center
void EllipseDirect(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c) {
    //loop on x from 0 to rx, calc y
    for (int x = 0; x <= rx; x++) {
        double y = ry * sqrt(1.0 - (x * x) / ((double)rx * rx));
        ellipsePoints(hdc, xc, yc, (int)round(x), (int)round(y), c);
    }

    //sweep y from 0 to ry, compute x
    for (int y = 0; y <= ry; y++) {
        double x = rx * sqrt(1.0 - (y * y) / ((double)ry * ry));
        ellipsePoints(hdc, xc, yc, (int)round(x), (int)round(y), c);
    }
}

void EllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c) {
    int x = 0;
    int y = ry;
    double d1 = (ry * ry) - (rx * rx * ry) + (0.25 * rx * rx);

    //slope < 1 curve more horizontal
    while (2.0 * ry * ry * x < 2.0 * rx * rx * y) {
        ellipsePoints(hdc, xc, yc, x, y, c);
        if (d1 < 0) {
            x++;
            d1 += 2 * ry * ry * x + ry * ry;
        }
        else {
            x++;
            y--;
            d1 += 2 * ry * ry * x - 2 * rx * rx * y + ry * ry;
        }

    }

    double d2 = (ry * ry * (x + 0.5) * (x + 0.5)) + (rx * rx * (y - 1) * (y - 1)) - (rx * rx * ry * ry);
    //slope > 1 more vertical
    while (y >= 0) {
        ellipsePoints(hdc, xc, yc, x, y, c);
        if (d2 > 0) {
            y--;
            d2 += rx * rx - 2 * rx * rx * y;
        }
        else {
            y--;
            x++;
            d2 += 2 * ry * ry * x - 2 * rx * rx * y + rx * rx;
        }
    }
}

void EllipsePolar(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c) {
    double theta = 0;
    double d_theta = 1.0 / max(rx, ry);

    //from 0 to 90 degree
    while (theta <= M_PI / 2.0)
    {
        double x = rx * cos(theta);
        double y = ry * sin(theta);
        ellipsePoints(hdc, xc, yc, (int)round(x), (int)round(y), c);
        theta += d_theta;
    }
}

