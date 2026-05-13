#include "../headers/smiley_face.h"
#include "../headers/circles.h"
#include "../headers/lines.h"
#include "../headers/globals.h"
#include <windows.h>
#include <cmath>
#include <stack>
#include <algorithm>
using namespace std;
// ════════════════════════════════════════════════════════════════════════════
// BONUS: SMILEY FACES
//
// Uses CircleMidpoint and LineMidpoint.
// Mouth is drawn as a parametric arc (portion of an ellipse).
// ════════════════════════════════════════════════════════════════════════════


void DrawArc(HDC hdc, int cx, int cy, int rx, int ry,
             double tStart, double tEnd, COLORREF c)
{
    double dtheta = 1.0 / max(rx, ry); // -> from polar circle lecture
    int prevX = cx + (int)round(rx * cos(tStart));
    int prevY = cy + (int)round(ry * sin(tStart));

    for (double t = tStart + dtheta; t <= tEnd + dtheta / 2.0; t += dtheta)
    {
        int x = cx + (int)round(rx * cos(t));
        int y = cy + (int)round(ry * sin(t));
        LineMidpoint(hdc, prevX, prevY, x, y, c);
        prevX = x; prevY = y;
    }
}

// Happy smiley face.
// Face + eyes: CircleMidpoint , Nose: LineMidpoint ,
// Smile: lower half of an ellipse arc (θ = 0 → π, bottom arc = U shape).
void DrawSmileyHappy(HDC hdc, int cx, int cy, int R, COLORREF c)
{
    // Face outline
    CircleMidpoint(hdc, cx, cy, R, c);

    // Eyes (two small circles, offset up and to each side)
    CircleMidpoint(hdc, cx - R / 3, cy - R / 4, R / 8, c);
    CircleMidpoint(hdc, cx + R / 3, cy - R / 4, R / 8, c);

    // Nose: two short diagonal lines meeting at a point
    LineMidpoint(hdc, cx, cy, cx - R / 10, cy + R / 6, c);
    LineMidpoint(hdc, cx, cy, cx + R / 10, cy + R / 6, c);

    // Smile: bottom half of ellipse centered below face center.
    // θ 0→π sweeps: right-corner → bottom-middle → left-corner (happy U curve).
    DrawArc(hdc, cx, cy + R / 4, R / 2, R / 4, 0, M_PI, c);
}

// Sad smiley face.
// Same structure as happy but frown = top half of ellipse arc (θ = π → 2π,
// which sweeps: left-corner → top-middle → right-corner, an upside-down U).
void DrawSmileySad(HDC hdc, int cx, int cy, int R, COLORREF c)
{
    // Face outline
    CircleMidpoint(hdc, cx, cy, R, c);

    // Eyes
    CircleMidpoint(hdc, cx - R / 3, cy - R / 4, R / 8, c);
    CircleMidpoint(hdc, cx + R / 3, cy - R / 4, R / 8, c);

    // Nose
    LineMidpoint(hdc, cx, cy, cx - R / 10, cy + R / 6, c);
    LineMidpoint(hdc, cx, cy, cx + R / 10, cy + R / 6, c);

    // Frown: top half of ellipse.
    // Center moved to cy + R/2 so the arch top sits at cy + R/3,
    // leaving a clear gap above the nose tip (cy + R/6).
    // θ π→2π sweeps: left-corner → top-middle → right-corner (sad ∩ curve).
    DrawArc(hdc, cx, cy + R / 2, R / 2, R / 6, M_PI, 2 * M_PI, c);
}
