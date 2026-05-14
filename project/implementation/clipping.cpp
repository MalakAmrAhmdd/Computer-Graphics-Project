#include "../headers/clipping.h"
#include "../headers/lines.h"
#include "../headers/circles.h"
#include "../headers/globals.h"
#include <cmath>
#include <algorithm>

using namespace std;

// Draw the rectangle clipping window outline in black.
void DrawRectangleWindow(HDC hdc)
{
    COLORREF c = RGB(0, 0, 0);
    LineMidpoint(hdc, (int)xLeft, (int)yTop, (int)xRight, (int)yTop, c);
    LineMidpoint(hdc, (int)xLeft, (int)yBottom, (int)xRight, (int)yBottom, c);
    LineMidpoint(hdc, (int)xLeft, (int)yTop, (int)xLeft, (int)yBottom, c);
    LineMidpoint(hdc, (int)xRight, (int)yTop, (int)xRight, (int)yBottom, c);
}

// Draw the square clipping window outline in black.
void DrawSquareWindow(HDC hdc)
{
    COLORREF c = RGB(0, 0, 0);
    LineMidpoint(hdc, (int)sqLeft, (int)sqTop, (int)sqRight, (int)sqTop, c);
    LineMidpoint(hdc, (int)sqLeft, (int)sqBottom, (int)sqRight, (int)sqBottom, c);
    LineMidpoint(hdc, (int)sqLeft, (int)sqTop, (int)sqLeft, (int)sqBottom, c);
    LineMidpoint(hdc, (int)sqRight, (int)sqTop, (int)sqRight, (int)sqBottom, c);
}

// Draw the circle clipping window outline using CircleMidpoint.
void DrawCircleClipWindow(HDC hdc)
{
    CircleMidpoint(hdc, (int)clipCircleCX, (int)clipCircleCY,
                   (int)clipCircleR, RGB(0, 0, 0));
}

outcode GetOutCode(double x, double y,
                   double xleft, double xright, double ybottom, double ytop)
{
    outcode out;
    out.all = 0;
    if (x < xleft)   out.L = 1;
    if (x > xright)  out.R = 1;
    if (y < ytop)    out.T = 1; // above the window (smaller y in screen coords)
    if (y > ybottom) out.B = 1; // below the window (larger y in screen coords)
    return out;
}

void VIntersect(double xedge, double x1, double y1, double x2, double y2,
                double& xi, double& yi)
{
    xi = xedge;
    yi = y1 + (xedge - x1) * (y2 - y1) / (x2 - x1);
}

void HIntersect(double yedge, double x1, double y1, double x2, double y2,
                double& xi, double& yi)
{
    yi = yedge;
    xi = x1 + (yedge - y1) * (x2 - x1) / (y2 - y1);
}

// Cohen-Sutherland line clipper.

void CoheSuth(HDC hdc, double& x1, double& y1, double& x2, double& y2,
              double xleft, double xright, double ybottom, double ytop)
{
    outcode out1 = GetOutCode(x1, y1, xleft, xright, ybottom, ytop);
    outcode out2 = GetOutCode(x2, y2, xleft, xright, ybottom, ytop);

    while ((out1.all || out2.all) && !(out1.all & out2.all))
    {
        double xi, yi;
        if (out1.all)
        {
            if (out1.T) HIntersect(ytop, x1, y1, x2, y2, xi, yi);
            else if (out1.B) HIntersect(ybottom, x1, y1, x2, y2, xi, yi);
            else if (out1.L) VIntersect(xleft, x1, y1, x2, y2, xi, yi);
            else             VIntersect(xright, x1, y1, x2, y2, xi, yi);
            x1 = xi; y1 = yi;
            out1 = GetOutCode(x1, y1, xleft, xright, ybottom, ytop);
        }
        else
        {
            if (out2.T) HIntersect(ytop, x1, y1, x2, y2, xi, yi);
            else if (out2.B) HIntersect(ybottom, x1, y1, x2, y2, xi, yi);
            else if (out2.L) VIntersect(xleft, x1, y1, x2, y2, xi, yi);
            else             VIntersect(xright, x1, y1, x2, y2, xi, yi);
            x2 = xi; y2 = yi;
            out2 = GetOutCode(x2, y2, xleft, xright, ybottom, ytop);
        }
    }
    // Draw only if both endpoints are now inside — uses currentColor
    if (!out1.all && !out2.all)
        LineMidpoint(hdc, (int)round(x1), (int)round(y1),
                     (int)round(x2), (int)round(y2), currentColor);
}

bool pointclip(double x, double y,
               double xleft, double xright, double ybottom, double ytop)
{
    return (x >= xleft && x <= xright && y >= ytop && y <= ybottom);
}

// ── Sutherland-Hodgman polygon helpers ──────────────────────────────────
bool Inleft(Point& p, double xleft) { return p.x >= xleft; }
bool Inright(Point& p, double xright) { return p.x <= xright; }
bool Intop(Point& p, double ytop) { return p.y >= ytop; }
bool Inbottom(Point& p, double ybottom) { return p.y <= ybottom; }

Point VIntersect(Point& p1, Point& p2, double xedge)
{
    Point r;
    r.x = (int)xedge;
    r.y = (int)(p1.y + (xedge - p1.x) * (double)(p2.y - p1.y) / (p2.x - p1.x));
    return r;
}

Point HIntersect(Point& p1, Point& p2, double yedge)
{
    Point r;
    r.y = (int)yedge;
    r.x = (int)(p1.x + (yedge - p1.y) * (double)(p2.x - p1.x) / (p2.y - p1.y));
    return r;
}

polygonn clipEdge(polygonn p, double edge, InF In, InterF Intersect)
{
    polygonn result;
    int n = (int)p.size();
    Point v1 = p[n - 1];
    bool  In1 = In(v1, edge);
    for (int i = 0; i < n; i++)
    {
        Point v2 = p[i];
        bool  In2 = In(v2, edge);
        if (!In1 && In2) { result.push_back(Intersect(v1, v2, edge)); result.push_back(v2); }
        else if (In1 && In2) { result.push_back(v2); }
        else if (In1) { result.push_back(Intersect(v1, v2, edge)); }
        v1 = v2; In1 = In2;
    }
    return result;
}

// ════════════════════════════════════════════════════════════════════════════
// polygonclip — Sutherland-Hodgman polygon clipping.
// ════════════════════════════════════════════════════════════════════════════
void polygonclip(HDC hdc, Point* p, int n,
                 double xleft, double xright, double ybottom, double ytop)
{
    polygonn vlist;
    for (int i = 0; i < n; i++) vlist.push_back(Point(p[i].x, p[i].y));

    // Clip against left edge  — keep points where x >= xleft
    vlist = clipEdge(vlist, xleft, Inleft, VIntersect);
    // Clip against right edge — keep points where x <= xright
    vlist = clipEdge(vlist, xright, Inright, VIntersect);
    // FIX: clip top edge first (smaller y in screen coords = top of window)
    //      keep points where p.y >= ytop  (Intop checks p.y >= edge)
    vlist = clipEdge(vlist, ytop, Intop, HIntersect);
    // FIX: clip bottom edge last (larger y in screen coords = bottom of window)
    //      keep points where p.y <= ybottom  (Inbottom checks p.y <= edge)
    vlist = clipEdge(vlist, ybottom, Inbottom, HIntersect);

    if (vlist.empty()) return; // entire polygon was outside — draw nothing

    // Draw the surviving (inside) polygon in the original drawing color
    Point v1 = vlist[vlist.size() - 1];
    for (int i = 0; i < (int)vlist.size(); i++)
    {
        Point v2 = vlist[i];
        LineMidpoint(hdc, v1.x, v1.y, v2.x, v2.y, currentColor);
        v1 = v2;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// CIRCLE CLIPPING WINDOW — helpers
// ════════════════════════════════════════════════════════════════════════════
bool PointInsideCircleWindow(double x, double y)
{
    double dx = x - clipCircleCX, dy = y - clipCircleCY;
    return (dx * dx + dy * dy) <= clipCircleR * clipCircleR;
}

// Clip a line segment to the circle window.
bool ClipLineToCircle(double x1, double y1, double x2, double y2,
                      double& ox1, double& oy1, double& ox2, double& oy2)
{
    double dx = x2 - x1, dy = y2 - y1;
    double fx = x1 - clipCircleCX, fy = y1 - clipCircleCY;
    double R = clipCircleR;

    double a = dx * dx + dy * dy;
    double b = 2.0 * (fx * dx + fy * dy);
    double c = fx * fx + fy * fy - R * R;

    double disc = b * b - 4.0 * a * c;

    bool p1in = PointInsideCircleWindow(x1, y1);
    bool p2in = PointInsideCircleWindow(x2, y2);

    // Both inside — draw the whole segment
    if (p1in && p2in)
    {
        ox1 = x1; oy1 = y1;
        ox2 = x2; oy2 = y2;
        return true;
    }

    // No real intersection at all — entirely outside
    if (disc < 0) return false;

    double sqrtD = sqrt(disc);
    double t1 = (-b - sqrtD) / (2.0 * a);
    double t2 = (-b + sqrtD) / (2.0 * a);

    // Clamp t values to [0,1]
    double tMin = max(0.0, min(t1, t2));
    double tMax = min(1.0, max(t1, t2));
    if (tMin > tMax) return false;

    ox1 = x1 + tMin * dx;  oy1 = y1 + tMin * dy;
    ox2 = x1 + tMax * dx;  oy2 = y1 + tMax * dy;
    return true;
}
