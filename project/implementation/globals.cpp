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
bool   circleClipWaitingRadius = false;
int clipState = 0;

POINT pts[5];
int pointCount = 0;
bool polygonDrawn = false;
bool useRecursive = true;

void RedrawShapes(HDC hdc)
{
    // Each type is checked independently with its own size guard.
    // Using chained if/else-if with an outer size guard caused shapes with
    // params.size() >= 4 (ellipses, curves, fills) to fall into the lines
    // block and never reach their own branch — fixed here.
    for (auto& s : shapes)
    {
        // ── Lines ─────────────────────────────────────────────────
        if (s.type == "LINE_DDA" && s.params.size() >= 4)
            LineDDA(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        else if (s.type == "LINE_MIDPOINT" && s.params.size() >= 4)
            LineMidpoint(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        else if (s.type == "LINE_PARAMETRIC" && s.params.size() >= 4)
            LineParametric(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);

            // ── Circles ───────────────────────────────────────────────
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
        else if (s.type == "ELLIPSE_DIRECT" && s.params.size() >= 4)
            EllipseDirect(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        else if (s.type == "ELLIPSE_Midpoint" && s.params.size() >= 4)
            EllipseMidpoint(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        else if (s.type == "ELLIPSE_Polar" && s.params.size() >= 4)
            EllipsePolar(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);

            // ── Cardinal Spline  ─────────────────────────────
            // Params layout: [tension*1000, n, x0,y0, x1,y1, ..., xn-1,yn-1]
        else if (s.type == "CURVE_CARDINAL" && s.params.size() >= 4)
        {
            double tension = s.params[0] / 1000.0;
            int n = s.params[1];
            if ((int)s.params.size() >= 2 + 2 * n)
            {
                std::vector<POINT> pts(n);
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
        else if (s.type == "Recursive_Flood_Fill" && s.params.size() >= 4)
            FillCircleWithCircles(hdc, s.params[0], s.params[1],
                                  s.params[2], s.params[3], s.color);
            // ── Smiley faces  — Params: [cx, cy, R] ─────────
        else if (s.type == "SMILEY_HAPPY" && s.params.size() >= 3)
            DrawSmileyHappy(hdc, s.params[0], s.params[1], s.params[2], s.color);
        else if (s.type == "SMILEY_SAD" && s.params.size() >= 3)
            DrawSmileySad(hdc, s.params[0], s.params[1], s.params[2], s.color);
            // ── Hermite square fill — params: [x1, y1, x2, y2] ──────────────
        else if (s.type == "FILL_SQUARE_HERMIT" && s.params.size() >= 4)
            FillSquareHermite(hdc, s.params[0], s.params[1],
                              s.params[2], s.params[3], s.color);

            // ── Bezier rectangle fill — params: [x1, y1, x2, y2] ────────────
        else if (s.type == "FILL_RECT_BEZIER" && s.params.size() >= 4)
            FillRectangleBezier(hdc, s.params[0], s.params[1],
                                s.params[2], s.params[3], s.color);

        else if (s.type == "Recursive_Flood_Fill" && s.params.size() >= 4)
            FillCircleWithCircles(hdc, s.params[0], s.params[1],
                                  s.params[2], s.params[3], s.color);

        else if (s.type == "SMILEY_HAPPY" && s.params.size() >= 3)
            DrawSmileyHappy(hdc, s.params[0], s.params[1], s.params[2], s.color);
        else if (s.type == "SMILEY_SAD" && s.params.size() >= 3)
            DrawSmileySad(hdc, s.params[0], s.params[1], s.params[2], s.color);

        // ── 5: Clipping doesn't need redraw entries —
        //    it operates on existing shapes using the clipping window
    }
}


