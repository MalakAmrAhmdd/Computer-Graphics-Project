#ifndef LVL3_COMPUTER_GRAPHICS_CLIPPING_H
#define LVL3_COMPUTER_GRAPHICS_CLIPPING_H
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <vector>
#include "globals.h"

using namespace std;

// ──click-state variables ────────────────────
union outcode {
    struct { unsigned L : 1, R : 1, B : 1, T : 1; };
    unsigned all : 4;
};
typedef bool  (*InF)(Point& p, double edge);
typedef Point(*InterF)(Point& p1, Point& p2, double edge);
typedef vector<Point> polygonn;

static int p1x, p1y;
static int p2x, p2y;

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
