#include "../headers/globals.h"
#include "../headers/shapes.h"
#include "../headers/lines.h"
#include "../headers/circles.h"
#include "../headers/ellipse.h"
#include "../headers/curves.h"
#include "../headers/filling.h"
#include "../headers/clipping.h"
#include "../headers/smiley_face.h"
#include <windows.h>
#include <cmath>
#include <stack>
#include <string>
#include <vector>

#include "../headers/globals.h"

std::vector<Shape> shapes;
COLORREF currentColor = RGB(0, 0, 0);
bool useCustomCursor = false;
bool whiteBg = false;
HBRUSH bgBrush = NULL;
std::string activeAlgorithm = "";

bool waitingForSecondClick = false;
int x1Line = 0, y1Line = 0;

bool circleWaitingForRadius = false;
int circleCX = 0, circleCY = 0;

bool ellipseWaiting = false;
int ellipseCX = 0, ellipseCY = 0;

bool curveCollecting = false;
double curveTension = 0.5;
std::vector<POINT> curvePoints;

bool fillWaitingCenter = false;
bool fillWaitingEdge = false;
int fillCX = 0, fillCY = 0;
int fillQuarter = 1;

bool fillPolyCollecting = false;
vector<FillPoint> fillPolyPoints;

bool hermiteWaitingSecond = false;
int hermiteX1 = 0, hermiteY1 = 0;

bool bezierRectWaitingSecond = false;
int bezierX1 = 0, bezierY1 = 0;

bool smileyWaitingCenter = false;
bool smileyWaitingEdge = false;
int smileyCX = 0, smileyCY = 0;

// clipping rectangle window
int xLeft = 0, xRight = 0, yTop = 0, yBottom = 0;
// Line endpoints reused for clipping
double x1line = 0, y1line = 0, x2Line = 0, y2Line = 0;
// clipping square window
double sqLeft = 0, sqRight = 0, sqTop = 0, sqBottom = 0;
vector<Point> polyPoints;
// Circle clipping window (center + radius)
double clipCircleCX = 0, clipCircleCY = 0, clipCircleR = 0;
bool circleClipWaitingRadius = false;
int clipState = 0;

POINT pts[4];
int pointCount = 0;
bool polygonDrawn = false;
bool useRecursive = true;

void RedrawShapes(HDC hdc)
{
    // Each type is checked independently with its own size guard.
    for (auto &s : shapes)
    {
        // ── Lines ─────────────────────────────────────────────────
        if (s.type == "LINE_DDA" && s.params.size() >= 4)
            LineDDA(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        else if (s.type == "LINE_MIDPOINT" && s.params.size() >= 4)
            LineMidpoint(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        else if (s.type == "LINE_PARAMETRIC" && s.params.size() >= 4)
            LineParametric(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);

        // ── Circles ───────────────────────────────────────────────
        // FIX 4: correct full type strings so circles survive repaint/load
        else if (s.type == "CIRCLE_DIRECT" && s.params.size() >= 3)
            CircleDirect(hdc, s.params[0], s.params[1], s.params[2], s.color);
        else if (s.type == "CIRCLE_POLAR" && s.params.size() >= 3)
            CirclePolar(hdc, s.params[0], s.params[1], s.params[2], s.color);
        else if (s.type == "CIRCLE_ITER_POLAR" && s.params.size() >= 3)
            CircleIterPolar(hdc, s.params[0], s.params[1], s.params[2], s.color);
        else if (s.type == "CIRCLE_MIDPOINT" && s.params.size() >= 3)
            CircleMidpoint(hdc, s.params[0], s.params[1], s.params[2], s.color);
        else if (s.type == "CIRCLE_MOD_MIDPOINT" && s.params.size() >= 3)
            CircleModMid(hdc, s.params[0], s.params[1], s.params[2], s.color);

        // ── Ellipses ──────────────────────────────────────────────
        // FIX 5: type strings match what is saved in the click handler
        else if (s.type == "ELLIPSE_DIRECT" && s.params.size() >= 4)
            EllipseDirect(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        else if (s.type == "ELLIPSE_MIDPOINT" && s.params.size() >= 4)
            EllipseMidpoint(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        else if (s.type == "ELLIPSE_POLAR" && s.params.size() >= 4)
            EllipsePolar(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);

        // ── Cardinal Spline  ─────────────────────────────
        // Params layout: [tension*1000, n, x0,y0, x1,y1, ..., xn-1,yn-1]
        else if (s.type == "CURVE_CARDINAL" && s.params.size() >= 4)
        {
            double tension = s.params[0] / 1000.0;
            int n = s.params[1];
            if ((int)s.params.size() >= 2 + 2 * n)
            {
                vector<POINT> pts(n);
                for (int i = 0; i < n; i++)
                {
                    pts[i].x = s.params[2 + 2 * i];
                    pts[i].y = s.params[3 + 2 * i];
                }
                DrawCardinalSpline(hdc, pts.data(), n, tension, s.color);
            }
        }

        // ── Circle fill  — Params: [cx, cy, R, quarter] ─
        else if (s.type == "FILL_CIRCLE_LINES" && s.params.size() >= 4)
            FillCircleWithLines(hdc, s.params[0], s.params[1],
                                s.params[2], s.params[3], s.color);
        else if (s.type == "FILL_CIRCLE_CIRCLES" && s.params.size() >= 4)
            FillCircleWithCircles(hdc, s.params[0], s.params[1],
                                  s.params[2], s.params[3], s.color);

        // ── Hermite square fill — params: [x1, y1, x2, y2] ──────────────
        else if (s.type == "FILL_SQUARE_HERMIT" && s.params.size() >= 4)
            FillSquareHermite(hdc, s.params[0], s.params[1],
                              s.params[2], s.params[3], s.color);

        // ── Bezier rectangle fill — params: [x1, y1, x2, y2] ────────────
        else if (s.type == "FILL_RECT_BEZIER" && s.params.size() >= 4)
            FillRectangleBezier(hdc, s.params[0], s.params[1],
                                s.params[2], s.params[3], s.color);
        // ── Convex / Non-Convex fill — params: [n, x0,y0, x1,y1, ...] ──
        else if ((s.type == "FILL_CONVEX" || s.type == "FILL_NONCONVEX") && s.params.size() >= 1)
        {
            int n = s.params[0];
            if ((int)s.params.size() >= 1 + 2 * n)
            {
                vector<FillPoint> poly(n);
                for (int i = 0; i < n; i++)
                {
                    poly[i].x = s.params[1 + 2 * i];
                    poly[i].y = s.params[2 + 2 * i];
                }
                if (s.type == "FILL_CONVEX")
                    ConvexFill(hdc, poly, s.color);
                else
                    NonConvexFill(hdc, poly, s.color);
            }
        }

        else if ((s.type == "FLOOD_RECURSIVE" || s.type == "FLOOD_NONRECURSIVE") && s.params.size() >= 10)
        {
            // Restore the 4 polygon vertices into the global pts[] array
            for (int i = 0; i < 4; i++)
            {
                pts[i].x = s.params[i * 2];
                pts[i].y = s.params[i * 2 + 1];
            }
            // Redraw the polygon border (flood fill reads pixel colors so border must exist first)
            DrawPolygon(hdc);

            int seedX = s.params[8];
            int seedY = s.params[9];
            COLORREF bc = RGB(0, 0, 0), fc = s.color;

            if (s.type == "FLOOD_RECURSIVE")
                RecursiveFloodFill(hdc, seedX, seedY, bc, fc);
            else
                NonRecursiveFloodFill(hdc, seedX, seedY, bc, fc);
        }

        // ── Smiley faces  — Params: [cx, cy, R] ─────────
        else if (s.type == "SMILEY_HAPPY" && s.params.size() >= 3)
            DrawSmileyHappy(hdc, s.params[0], s.params[1], s.params[2], s.color);
        else if (s.type == "SMILEY_SAD" && s.params.size() >= 3)
            DrawSmileySad(hdc, s.params[0], s.params[1], s.params[2], s.color);

        // ── 5: Clipping redraw — replays saved clipped results ──────────
        // Each entry stores the window bounds + the already-clipped geometry,
        // so we just re-draw the result directly (no re-clipping needed).

        // CLIP_RECT_LINE — params: [xL, yT, xR, yB, lx1, ly1, lx2, ly2]
        // Re-draws the clipping window border in black, then re-clips and
        // draws the surviving segment in the saved color.
        else if (s.type == "CLIP_RECT_LINE" && s.params.size() >= 8)
        {
            double wL = s.params[0], wT = s.params[1],
                   wR = s.params[2], wB = s.params[3];
            // Redraw the rectangle window border in black
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wR, (int)wT, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wB, (int)wR, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wL, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wR, (int)wT, (int)wR, (int)wB, RGB(0, 0, 0));
            // Re-clip the original line with the saved window and draw in saved color
            double lx1 = s.params[4], ly1 = s.params[5],
                   lx2 = s.params[6], ly2 = s.params[7];
            COLORREF saved = currentColor;
            currentColor = s.color;
            CoheSuth(hdc, lx1, ly1, lx2, ly2, wL, wR, wB, wT);
            currentColor = saved;
        }

        // CLIP_RECT_POINT — params: [xL, yT, xR, yB, px, py]
        // Re-draws the clipping window border in black, then draws the
        // accepted point as a filled circle in the saved color.
        else if (s.type == "CLIP_RECT_POINT" && s.params.size() >= 6)
        {
            double wL = s.params[0], wT = s.params[1],
                   wR = s.params[2], wB = s.params[3];
            // Redraw the rectangle window border in black
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wR, (int)wT, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wB, (int)wR, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wL, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wR, (int)wT, (int)wR, (int)wB, RGB(0, 0, 0));
            // Redraw the accepted point as a small filled ellipse in saved color
            int px = s.params[4], py = s.params[5];
            HBRUSH br = CreateSolidBrush(s.color);
            HBRUSH old = (HBRUSH)SelectObject(hdc, br);
            Ellipse(hdc, px - 3, py - 3, px + 3, py + 3);
            SelectObject(hdc, old);
            DeleteObject(br);
        }

        // CLIP_SQ_LINE — params: [sqL, sqT, sqR, sqB, lx1, ly1, lx2, ly2]
        // Re-draws the square clipping window border in black, then re-clips
        // and draws the surviving line segment in the saved color.
        else if (s.type == "CLIP_SQ_LINE" && s.params.size() >= 8)
        {
            double wL = s.params[0], wT = s.params[1],
                   wR = s.params[2], wB = s.params[3];
            // Redraw the square window border in black
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wR, (int)wT, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wB, (int)wR, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wL, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wR, (int)wT, (int)wR, (int)wB, RGB(0, 0, 0));
            // Re-clip and draw in saved color
            double lx1 = s.params[4], ly1 = s.params[5],
                   lx2 = s.params[6], ly2 = s.params[7];
            COLORREF saved = currentColor;
            currentColor = s.color;
            CoheSuth(hdc, lx1, ly1, lx2, ly2, wL, wR, wB, wT);
            currentColor = saved;
        }

        // CLIP_SQ_POINT — params: [sqL, sqT, sqR, sqB, px, py]
        // Re-draws the square clipping window border in black, then draws the
        // accepted point as a filled circle in the saved color.
        else if (s.type == "CLIP_SQ_POINT" && s.params.size() >= 6)
        {
            double wL = s.params[0], wT = s.params[1],
                   wR = s.params[2], wB = s.params[3];
            // Redraw the square window border in black
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wR, (int)wT, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wB, (int)wR, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wL, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wR, (int)wT, (int)wR, (int)wB, RGB(0, 0, 0));
            // Redraw the accepted point as a small filled ellipse in saved color
            int px = s.params[4], py = s.params[5];
            HBRUSH br = CreateSolidBrush(s.color);
            HBRUSH old = (HBRUSH)SelectObject(hdc, br);
            Ellipse(hdc, px - 3, py - 3, px + 3, py + 3);
            SelectObject(hdc, old);
            DeleteObject(br);
        }

        // CLIP_CIRCLE_LINE — params: [cx, cy, R, ox1, oy1, ox2, oy2]
        // Re-draws the circle clipping window in black, then re-draws the
        // already-clipped line segment directly in the saved color (no re-clip).
        else if (s.type == "CLIP_CIRCLE_LINE" && s.params.size() >= 7)
        {
            // Redraw the circle window border in black
            CircleMidpoint(hdc, s.params[0], s.params[1], s.params[2], RGB(0, 0, 0));
            // Redraw the clipped segment in saved color
            LineMidpoint(hdc, s.params[3], s.params[4],
                         s.params[5], s.params[6], s.color);
        }

        // CLIP_CIRCLE_POINT — params: [cx, cy, R, px, py]
        // Re-draws the circle clipping window in black, then draws the
        // accepted point as a filled circle in the saved color.
        else if (s.type == "CLIP_CIRCLE_POINT" && s.params.size() >= 5)
        {
            // Redraw the circle window border in black
            CircleMidpoint(hdc, s.params[0], s.params[1], s.params[2], RGB(0, 0, 0));
            // Redraw the accepted point as a small filled ellipse in saved color
            int px = s.params[3], py = s.params[4];
            HBRUSH br = CreateSolidBrush(s.color);
            HBRUSH old = (HBRUSH)SelectObject(hdc, br);
            Ellipse(hdc, px - 3, py - 3, px + 3, py + 3);
            SelectObject(hdc, old);
            DeleteObject(br);
        }

        // CLIP_RECT_POLY — params: [xL, yT, xR, yB, n, x0,y0, x1,y1, ...]
        // Re-draws the rectangle clipping window in black, then re-clips the
        // polygon and draws the surviving inside portion in the saved color.
        else if (s.type == "CLIP_RECT_POLY" && s.params.size() >= 5)
        {
            double wL = s.params[0], wT = s.params[1],
                   wR = s.params[2], wB = s.params[3];
            // Redraw the rectangle window border in black
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wR, (int)wT, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wB, (int)wR, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wL, (int)wT, (int)wL, (int)wB, RGB(0, 0, 0));
            LineMidpoint(hdc, (int)wR, (int)wT, (int)wR, (int)wB, RGB(0, 0, 0));
            int n = s.params[4];
            if ((int)s.params.size() >= 5 + 2 * n)
            {
                vector<Point> poly(n);
                for (int i = 0; i < n; i++)
                {
                    poly[i].x = s.params[5 + 2 * i];
                    poly[i].y = s.params[6 + 2 * i];
                }
                // Re-clip and draw inside portion in saved color
                COLORREF saved = currentColor;
                currentColor = s.color;
                polygonclip(hdc, poly.data(), n, wL, wR, wB, wT);
                currentColor = saved;
            }
        }
        else if (s.type == "FILL_SQUARE_HERMIT_LINES" && s.params.size() >= 4)
        {
            FillSquareHermiteLines(hdc, s.params[0], s.params[1],
                                   s.params[2], s.params[3], s.color);
        }
    }
}
