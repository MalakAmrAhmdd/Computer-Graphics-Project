#ifndef LVL3_COMPUTER_GRAPHICS_GLOBALS_H
#define LVL3_COMPUTER_GRAPHICS_GLOBALS_H

#include "shapes.h"
//#include "filling.h"
#include <windows.h>
#include <string>
#include <vector>

using namespace std;

extern std::vector<Shape> shapes;
extern COLORREF currentColor;
extern bool useCustomCursor;
extern bool whiteBg;
extern HBRUSH bgBrush;
extern std::string activeAlgorithm;

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

extern bool waitingForSecondClick;
extern int x1Line, y1Line;

extern bool circleWaitingForRadius;
extern int circleCX, circleCY;

extern bool ellipseWaiting;
extern int ellipseCX, ellipseCY;

extern bool curveCollecting;
extern double curveTension;
extern std::vector<POINT> curvePoints;

extern bool fillWaitingCenter;
extern bool fillWaitingEdge;
extern int fillCX, fillCY;
extern int fillQuarter;

// ── Convex / Non-Convex polygon fill click-state ──────────────────────────
// Vertices are collected with left-clicks; right-click triggers the fill.
struct FillPoint { int x, y; };
extern vector<FillPoint> fillPolyPoints;
extern bool fillPolyCollecting;

extern bool hermiteWaitingSecond;
extern int hermiteX1, hermiteY1;

extern bool bezierRectWaitingSecond;
extern int bezierX1, bezierY1;

extern bool smileyWaitingCenter;
extern bool smileyWaitingEdge;
extern int smileyCX, smileyCY;


// Rectangle clipping window bounds
extern int xLeft, xRight, yTop, yBottom;
// Line endpoints reused for clipping
extern double x1line, y1line, x2Line, y2Line;
// Square clipping window bounds
extern double sqLeft, sqRight, sqTop, sqBottom;
extern vector<Point> polyPoints;
// Circle clipping window (center + radius)
extern double clipCircleCX, clipCircleCY, clipCircleR;
extern bool   circleClipWaitingRadius;
extern int clipState;
extern bool polyCollect;

extern POINT pts[5];
extern int pointCount;
extern bool polygonDrawn;
extern bool useRecursive;


void RedrawShapes(HDC hdc);
#endif