#ifndef LVL3_COMPUTER_GRAPHICS_CLIPPING_H
#define LVL3_COMPUTER_GRAPHICS_CLIPPING_H
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <vector>
#include "globals.h"

using namespace std;

// ── 5: Add clipping click-state variables here ────────────────────
union outcode {
    struct { unsigned L : 1, R : 1, B : 1, T : 1; };
    unsigned all : 4;
};
typedef bool  (*InF)(Point& p, double edge);
typedef Point(*InterF)(Point& p1, Point& p2, double edge);
typedef vector<Point> polygonn;

//// Rectangle clipping window bounds
//extern double xLeft, xRight, yTop, yBottom;
//// Line endpoints reused for clipping
//extern double x1line, y1line, x2Line, y2Line;
//// Square clipping window bounds
//extern double sqLeft, sqRight, sqTop, sqBottom;
//
//// Circle clipping window (center + radius)
//extern double clipCircleCX, clipCircleCY, clipCircleR;
//extern bool   circleClipWaitingRadius;

// FIX 6: single unified clipState variable (the original had both
//        clipStage and clipState, which are different variables — only
//        clipState is actually read in the click handler).
//extern int clipState;

static int p1x, p1y;
static int p2x, p2y;

//extern vector<Point> polyPoints;
//extern bool polyCollect;

outcode GetOutCode(double x, double y, double xleft, double xright,
                   double ybottom, double ytop);
void VIntersect(double xedge, double x1, double y1, double x2, double y2,
                double& xi, double& yi);
void HIntersect(double yedge, double x1, double y1, double x2, double y2,
                double& xi, double& yi);
void CoheSuth(HDC hdc, double& x1, double& y1, double& x2, double& y2,
              double xleft, double xright, double ybottom, double ytop);
bool pointclip(double x, double y, double xleft, double xright,
               double ybottom, double ytop);

void DrawRectangleWindow(HDC hdc);
void DrawSquareWindow(HDC hdc);
void DrawCircleClipWindow(HDC hdc);

// Circle-window clipping helpers
bool  PointInsideCircleWindow(double x, double y);
bool  ClipLineToCircle(double x1, double y1, double x2, double y2,
                       double& ox1, double& oy1, double& ox2, double& oy2);

void polygonclip(HDC hdc, Point* p, int n,
                 double xleft, double xright, double ybottom, double ytop);

#endif //LVL3_COMPUTER_GRAPHICS_CLIPPING_H
