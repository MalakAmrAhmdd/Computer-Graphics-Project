#ifndef LVL3_COMPUTER_GRAPHICS_FILLING_H
#define LVL3_COMPUTER_GRAPHICS_FILLING_H
#include "../headers/globals.h"
#pragma once
#include <windows.h>
#include <string>
#include <vector>

using namespace std;

// 4 clicks define the polygon; the 4th edge auto-closes back to point 1.
extern POINT fpts[4];

// ──  Circle fill functions ──────────────────────────────
void FillCircleWithLines(HDC hdc, int xc, int yc, int R, int quarter, COLORREF c);
void FillCircleWithCircles(HDC hdc, int xc, int yc, int R, int quarter, COLORREF c);

// ── Hermite&Bezier fill functions ──────────────────────────────
void FillSquareHermite(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
static void BezierPoint(double p0x, double p0y,double p1x, double p1y,double p2x, double p2y,double p3x, double p3y,
                        double t, double& outX, double& outY);
void FillRectangleBezier(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
void FillSquareHermiteLines(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);

// ── Convex / Non-Convex fill ────────────────────────────────────────
struct Edge {
    float x;
    int   yMax;
    float mInv;
};

static void EdgeToTable(FillPoint p1, FillPoint p2, vector<vector<Edge>>& table);
static void PolygonToTable(vector<FillPoint>& polygon, vector<vector<Edge>>& table);
void ConvexFill(HDC hdc, vector<FillPoint>& polygon, COLORREF c);
void NonConvexFill(HDC hdc, vector<FillPoint>& polygon, COLORREF c);

// ──  flood fill functions ──────────────────────────────
void DrawPolygon(HDC hdc);
void RecursiveFloodFill(HDC hdc, int x, int y, COLORREF bc, COLORREF fc);
void NonRecursiveFloodFill(HDC hdc, int x, int y, COLORREF bc, COLORREF fc);
bool IsPointInsidePolygon(int x, int y);


#endif //LVL3_COMPUTER_GRAPHICS_FILLING_H
