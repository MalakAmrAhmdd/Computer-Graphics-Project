#include "../headers/filling.h"
#include "../headers/lines.h"
#include "../headers/shapes.h"
#include "../headers/globals.h"
#include "../headers/circles.h"
#include <windows.h>
#include <cmath>
#include <stack>
#include <algorithm>
#include <string>
#include <vector>

using namespace std;
#define _USE_MATH_DEFINES

//bool fillPolyCollecting = false;
//vector<FillPoint> fillPolyPoints;


// ════════════════════════════════════════════════════════════════════════════
// CIRCLE FILLING
//
// Quarter numbering (screen coords, y increases downward):
//   Q1 = top-right   (x >= cx, y <= cy)
//   Q2 = top-left    (x <= cx, y <= cy)
//   Q3 = bottom-left (x <= cx, y >= cy)
//   Q4 = bottom-right(x >= cx, y >= cy)
// ════════════════════════════════════════════════════════════════════════════
// Fill the selected quarter of a circle with horizontal scan lines.
// Scan lines go from the circle boundary to the vertical diameter (center column).
// Fill the selected quarter of a circle with horizontal scan lines.
void FillCircleWithLines(HDC hdc, int xc, int yc, int R, int quarter, COLORREF c)
{
    int yStart = (quarter == 1 || quarter == 2) ? yc - R : yc;
    int yEnd = (quarter == 1 || quarter == 2) ? yc : yc + R;

    for (int y = yStart; y <= yEnd; y++)
    {
        int dy = y - yc;
        if (dy * dy > R * R) continue;
        int dx = (int)round(sqrt((double)(R * R - dy * dy)));
        int lx, rx;
        if (quarter == 1 || quarter == 4) { lx = xc;      rx = xc + dx; }
        else { lx = xc - dx; rx = xc; }
        LineMidpoint(hdc, lx, y, rx, y, c);
    }

    // Full circle outline
    CircleMidpoint(hdc, xc, yc, R, c);

    // Two bounding radii for the selected quarter
    // Q1=top-right, Q2=top-left, Q3=bottom-left, Q4=bottom-right
    int ex1, ey1, ex2, ey2;
    switch (quarter)
    {
    case 1: ex1 = xc;     ey1 = yc - R; ex2 = xc + R; ey2 = yc;     break; 
    case 2: ex1 = xc - R; ey1 = yc;     ex2 = xc;     ey2 = yc - R; break; 
    case 3: ex1 = xc;     ey1 = yc + R; ex2 = xc - R; ey2 = yc;     break; 
    default:ex1 = xc + R; ey1 = yc;     ex2 = xc;     ey2 = yc + R; break; 
    }
    LineMidpoint(hdc, xc, yc, ex1, ey1, c);
    LineMidpoint(hdc, xc, yc, ex2, ey2, c);
}

// Fill the selected quarter of a circle with concentric circle arcs.
// Each ring is 5 pixels smaller than the last, down to radius 1.
void FillCircleWithCircles(HDC hdc, int xc, int yc, int R, int quarter, COLORREF c)
{
    double tStart, tEnd;
    switch (quarter)
    {
    case 1:  tStart = -M_PI / 2.0; tEnd = 0;              break; // top-right
    case 2:  tStart = M_PI;     tEnd = 3 * M_PI / 2.0;    break; // top-left
    case 3:  tStart = M_PI / 2.0; tEnd = M_PI;           break; // bottom-left
    default: tStart = 0;        tEnd = M_PI / 2.0;       break; // bottom-right (Q4)
    }
    for (int r = 1; r <= R; r++)
    {
        double dtheta = 1.0 / r;
        for (double theta = tStart; theta <= tEnd + dtheta / 2.0; theta += dtheta)
        {
            int x = xc + (int)round(r * cos(theta));
            int y = yc + (int)round(r * sin(theta));
            SetPixel(hdc, x, y, c);
        }
    }

    // Full circle outline
    CircleMidpoint(hdc, xc, yc, R, c);

    // Two bounding radii for the selected quarter
    int ex1, ey1, ex2, ey2;
    switch (quarter)
    {
    case 1: ex1 = xc;     ey1 = yc - R; ex2 = xc + R; ey2 = yc;     break;
    case 2: ex1 = xc - R; ey1 = yc;     ex2 = xc;     ey2 = yc - R; break;
    case 3: ex1 = xc;     ey1 = yc + R; ex2 = xc - R; ey2 = yc;     break;
    default:ex1 = xc + R; ey1 = yc;     ex2 = xc;     ey2 = yc + R; break;
    }
    LineMidpoint(hdc, xc, yc, ex1, ey1, c);
    LineMidpoint(hdc, xc, yc, ex2, ey2, c);
}
// ════════════════════════════════════════════════════════════════════════════
// FILL SQUARE WITH HERMITE CURVES
//
// Fills the bounding rectangle [x1,y1]-[x2,y2] with vertical cubic Hermite
// curves, exactly as the standalone version in document 2.
//
// Each curve runs from (xCol, top) to (xCol, bottom) with a horizontal
// tension tangent — this keeps every curve perfectly straight vertically
// (tangent=(tension,0) means "pull horizontally by `tension` pixels with
//  zero vertical component"), producing solid vertical fill lines.
//
// numCurves controls fill density; tension controls the S-bend width.
// Both are tunable constants at the top of the function.
//
// Params saved/loaded: [x1, y1, x2, y2]
// ════════════════════════════════════════════════════════════════════════════
void FillSquareHermite(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    const double H[4][4] = {
        { 1,  0,  0,  0},
        { 0,  1,  0,  0},
        {-3, -2,  3, -1},
        { 2,  1, -2,  1}
    };

    const int    numCurves = 80;
    const double tension = 100.0;

    int left = min(x1, x2);
    int top = min(y1, y2);
   
    int side = min(abs(x2 - x1), abs(y2 - y1));
    int right = left + side;
    int bottom = top + side;
   

    if (side <= 0) return;

    for (int i = 0; i < numCurves; i++)
    {
        double xCol = left + ((double)i / (numCurves - 1)) * side;

        double G[4][2] = {
            {xCol,     (double)top},
            {tension,  0.0},
            {xCol,     (double)bottom},
            {-tension, 0.0}
        };

        double C[4][2] = {};
        for (int r = 0; r < 4; r++)
            for (int col = 0; col < 2; col++)
                for (int k = 0; k < 4; k++)
                    C[r][col] += H[r][k] * G[k][col];

        for (double t = 0.0; t <= 1.0; t += 0.001)
        {
            double V[4] = { 1.0, t, t * t, t * t * t };
            double px = 0, py = 0;
            for (int k = 0; k < 4; k++) { px += V[k] * C[k][0]; py += V[k] * C[k][1]; }
            SetPixel(hdc, (int)round(px), (int)round(py), c);
        }
    }
}
// Evaluate a single cubic Bezier point at parameter t.
// Uses the standard explicit form: B(t) = (1-t)^3*P0 + 3t(1-t)^2*P1
//                                        + 3t^2(1-t)*P2 + t^3*P3
static void BezierPoint(double p0x, double p0y,
                        double p1x, double p1y,
                        double p2x, double p2y,
                        double p3x, double p3y,
                        double t, double& outX, double& outY)
{
    double u = 1.0 - t;
    double u2 = u * u, u3 = u2 * u;
    double t2 = t * t, t3 = t2 * t;
    outX = u3 * p0x + 3 * t * u2 * p1x + 3 * t2 * u * p2x + t3 * p3x;
    outY = u3 * p0y + 3 * t * u2 * p1y + 3 * t2 * u * p2y + t3 * p3y;
}

void FillRectangleBezier(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int left = min(x1, x2);
    int right = max(x1, x2);
    int top = min(y1, y2);
    int bottom = max(y1, y2);
    int width = right - left;
    int height = bottom - top;
    if (width <= 0 || height <= 0) return;

    // Draw the rectangle border in the chosen color
    for (int x = left; x <= right; x++) { SetPixel(hdc, x, top, c); SetPixel(hdc, x, bottom, c); }
    for (int y = top; y <= bottom; y++) { SetPixel(hdc, left, y, c); SetPixel(hdc, right, y, c); }

    // Fill: one horizontal Bezier per row
    // Control points all on the same row → curve is a straight horizontal line
    for (int row = top; row <= bottom; row++)
    {
        double P0x = left, P0y = row;
        double P1x = left + width / 3.0, P1y = row;
        double P2x = left + 2.0 * width / 3, P2y = row;
        double P3x = right, P3y = row;

        for (double t = 0.0; t <= 1.0; t += 0.0001)
        {
            double px, py;
            BezierPoint(P0x, P0y, P1x, P1y, P2x, P2y, P3x, P3y, t, px, py);
            SetPixel(hdc, (int)round(px), (int)round(py), c);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// CONVEX / NON-CONVEX POLYGON FILL  (Scan-line / Active Edge Table)
// ════════════════════════════════════════════════════════════════════════════

// Helper: add one edge to the edge table.
static void EdgeToTable(FillPoint p1, FillPoint p2, vector<vector<Edge>>& table)
{
    if (p1.y == p2.y) return; // skip horizontal edges
    if (p1.y > p2.y) swap(p1, p2); // ensure p1 is the lower-y endpoint

    Edge e;
    e.x = (float)p1.x;
    e.yMax = p2.y;
    e.mInv = (float)(p2.x - p1.x) / (float)(p2.y - p1.y);
    table[p1.y].push_back(e);
}

// Helper: build the full edge table from the polygon.
static void PolygonToTable(vector<FillPoint>& polygon, vector<vector<Edge>>& table)
{
    int n = (int)polygon.size();
    for (int i = 0; i < n; i++)
        EdgeToTable(polygon[i], polygon[(i + 1) % n], table);
}

// Convex fill — always exactly two active edges per scan-line.
void ConvexFill(HDC hdc, vector<FillPoint>& polygon, COLORREF c)
{
    const int HEIGHT = 800;
    vector<vector<Edge>> table(HEIGHT);
    PolygonToTable(polygon, table);

    vector<Edge> active;

    for (int y = 0; y < HEIGHT; y++)
    {
        for (auto& edge : table[y]) active.push_back(edge);

        active.erase(
                remove_if(active.begin(), active.end(),
                          [y](const Edge& e) { return y >= e.yMax; }),
                active.end());

        sort(active.begin(), active.end(),
             [](const Edge& a, const Edge& b) { return a.x < b.x; });

        if (active.size() >= 2)
        {
            int xStart = (int)ceil(active[0].x);
            int xEnd = (int)floor(active[1].x);
            for (int x = xStart; x <= xEnd; x++)
                SetPixel(hdc, x, y, c);
        }

        for (auto& edge : active) edge.x += edge.mInv;
    }
}

// Non-convex fill — pairs of active edges per scan-line.
void NonConvexFill(HDC hdc, vector<FillPoint>& polygon, COLORREF c)
{
    const int HEIGHT = 800;
    vector<vector<Edge>> table(HEIGHT);
    PolygonToTable(polygon, table);

    vector<Edge> active;

    for (int y = 0; y < HEIGHT; y++)
    {
        for (auto& edge : table[y]) active.push_back(edge);

        active.erase(
                remove_if(active.begin(), active.end(),
                          [y](const Edge& e) { return y >= e.yMax; }),
                active.end());

        sort(active.begin(), active.end(),
             [](const Edge& a, const Edge& b) { return a.x < b.x; });

        for (int i = 0; i + 1 < (int)active.size(); i += 2)
        {
            int xStart = (int)ceil(active[i].x);
            int xEnd = (int)floor(active[i + 1].x);
            for (int x = xStart; x <= xEnd; x++)
                SetPixel(hdc, x, y, c);
        }

        for (auto& edge : active) edge.x += edge.mInv;
    }
}


// ════════════════════════════════════════════════════════════════════════════
// FLOOD FILL
// ════════════════════════════════════════════════════════════════════════════
void DrawPolygon(HDC hdc)
{
    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4;
        LineMidpoint(hdc, pts[i].x, pts[i].y, pts[next].x, pts[next].y, RGB(0, 0, 0));
    }
}

void RecursiveFloodFill(HDC hdc, int x, int y, COLORREF bc, COLORREF fc)
{
    COLORREF c = GetPixel(hdc, x, y);
    if (c == bc || c == fc) return;
    SetPixel(hdc, x, y, fc);
    RecursiveFloodFill(hdc, x + 1, y, bc, fc);
    RecursiveFloodFill(hdc, x - 1, y, bc, fc);
    RecursiveFloodFill(hdc, x, y + 1, bc, fc);
    RecursiveFloodFill(hdc, x, y - 1, bc, fc);
}

void NonRecursiveFloodFill(HDC hdc, int x, int y, COLORREF bc, COLORREF fc)
{
    stack<Point> s;
    s.push(Point(x, y));
    while (!s.empty())
    {
        Point p = s.top(); s.pop();
        COLORREF c = GetPixel(hdc, p.x, p.y);
        if (c == bc || c == fc) continue;
        SetPixel(hdc, p.x, p.y, fc);
        s.push(Point(p.x + 1, p.y));
        s.push(Point(p.x - 1, p.y));
        s.push(Point(p.x, p.y + 1));
        s.push(Point(p.x, p.y - 1));
    }
}

bool IsPointInsidePolygon(int x, int y)
{
    bool inside = false;
    for (int i = 0, j = 3; i < 4; j = i++)
    {
        int xi = pts[i].x, yi = pts[i].y;
        int xj = pts[j].x, yj = pts[j].y;
        bool intersect = ((yi > y) != (yj > y)) &&
                         (x < (xj - xi) * (y - yi) / (double)(yj - yi) + xi);
        if (intersect) inside = !inside;
    }
    return inside;
}

void FillSquareHermiteLines(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int numCurves = 80;
    double tension = 100.0;

    double left   = min(x1, x2);
    double right  = max(x1, x2);
    double top    = min(y1, y2);
    double bottom = max(y1, y2);
    double width  = right - left;

    if (width <= 0 || bottom - top <= 0) return;

    for (int i = 0; i < numCurves; i++)
    {
        double xCol = left + ((double)i / (numCurves - 1)) * width;

        // Hermite matrix
        const double H[4][4] = {
            { 1,  0,  0,  0},
            { 0,  1,  0,  0},
            {-3, -2,  3, -1},
            { 2,  1, -2,  1}
        };

        // Vertical curve: start=(xCol,top), end=(xCol,bottom)

        double safeTension = min(tension, width / 2.5);
        double G[4][2] = {
            {xCol,           top},
            {safeTension,    0.0},
            {xCol,           bottom},
            {-safeTension,   0.0}
        };

        double C[4][2] = {};
        for (int r = 0; r < 4; r++)
            for (int col = 0; col < 2; col++)
                for (int k = 0; k < 4; k++)
                    C[r][col] += H[r][k] * G[k][col];

        for (double t = 0.0; t <= 1.0; t += 0.001)
        {
            double V[4] = { 1.0, t, t * t, t * t * t };
            double px = 0, py = 0;
            for (int k = 0; k < 4; k++) { px += V[k] * C[k][0]; py += V[k] * C[k][1]; }
            SetPixel(hdc, (int)round(px), (int)round(py), c);
        }
    }
}