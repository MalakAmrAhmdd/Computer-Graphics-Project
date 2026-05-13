#include "../headers/curves.h"
#include "../headers/lines.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#define _USE_MATH_DEFINES

// ════════════════════════════════════════════════════════════════════════════
// CURVES
//
// Hermite basis matrix:
//  | 2  1 -2  1 |
//  |-3 -2  3 -1 |
//  | 0  1  0  0 |
//  | 1  0  0  0 |
//
// x(t) = coeff[0]*t³ + coeff[1]*t² + coeff[2]*t + coeff[3]
// ════════════════════════════════════════════════════════════════════════════

// Compute cubic Hermite coefficients for one axis.
// p0/p1 = start/end positions, s0/s1 = start/end tangents.
// Fills coeff[0..3] as [t³, t², t¹, t⁰].
void GetHermiteCoeff(double p0, double s0, double p1, double s1, double coeff[4])
{
    coeff[0] = 2 * p0 + s0 - 2 * p1 + s1;  // t³
    coeff[1] = -3 * p0 - 2 * s0 + 3 * p1 - s1; // t²
    coeff[2] = s0;                       // t¹
    coeff[3] = p0;                       // t⁰
}

// Draw one Hermite curve segment from (x0,y0) to (x1,y1).
// (tx0,ty0) and (tx1,ty1) are the tangent vectors at each end.
// numpts controls smoothness (higher = smoother but slower).
void DrawHermiteSeg(HDC hdc, int x0, int y0, int tx0, int ty0,
                    int x1, int y1, int tx1, int ty1, COLORREF c, int numpts)
{
    double cx[4], cy[4];
    GetHermiteCoeff(x0, tx0, x1, tx1, cx);
    GetHermiteCoeff(y0, ty0, y1, ty1, cy);

    double dt = 1.0 / (numpts - 1);
    int prevX = x0, prevY = y0;

    for (int i = 1; i < numpts; i++)
    {
        double t = i * dt;
        double t2 = t * t, t3 = t2 * t;
        int nx = (int)round(cx[0] * t3 + cx[1] * t2 + cx[2] * t + cx[3]);
        int ny = (int)round(cy[0] * t3 + cy[1] * t2 + cy[2] * t + cy[3]);

        LineMidpoint(hdc, prevX, prevY, nx, ny, c);
        prevX = nx; prevY = ny;
    }
}

// Cardinal Spline through P[0..n-1].
// Draws through the inner points P[1] to P[n-2].
// Needs n >= 4 (P[0] and P[n-1] are tangent helpers, not drawn through).
// tension: 0 = very smooth (Catmull-Rom), 1 = straight lines between points.
// Formula from lecture: Ti = (1 - tension) * (P[i+1] - P[i-1])
void DrawCardinalSpline(HDC hdc, POINT P[], int n, double tension, COLORREF c)
{
    if (n < 4) return;

    double c1 = 1.0 - tension; // scale factor for tangents

    // Tangent at P[1] (the first drawn point)
    int tx0 = (int)round(c1 * (P[2].x - P[0].x));
    int ty0 = (int)round(c1 * (P[2].y - P[0].y));

    // Draw each segment P[i-1] → P[i] for i = 2 .. n-2
    for (int i = 2; i < n - 1; i++)
    {
        int tx1 = (int)round(c1 * (P[i + 1].x - P[i - 1].x));
        int ty1 = (int)round(c1 * (P[i + 1].y - P[i - 1].y));

        DrawHermiteSeg(hdc,
                       P[i - 1].x, P[i - 1].y, tx0, ty0,
                       P[i].x, P[i].y, tx1, ty1,
                       c);
        tx0 = tx1;
        ty0 = ty1;
    }
}
