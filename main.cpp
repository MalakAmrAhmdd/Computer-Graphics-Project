#define UNICODE
#define _USE_MATH_DEFINES   // makes M_PI available from <cmath> on all compilers
#include <windows.h>
#include <cmath>
#include <stack>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <algorithm>

using namespace std;

// ════════════════════════════════════════════════════════════════════════════
// MENU IDs — every clickable menu item needs a unique integer ID.
// When the user clicks a menu item, WndProc receives WM_COMMAND with that ID
// in LOWORD(wParam). Add your own IDs here following the same pattern.
// ════════════════════════════════════════════════════════════════════════════
#define ID_FILE_CLEAR 1001 // File → Clear Screen
#define ID_FILE_SAVE  1002 // File → Save
#define ID_FILE_LOAD  1003 // File → Load

#define ID_PREF_WHITEBG 2001 // Preferences: White Background
#define ID_PREF_CURSOR  2002 // Preferences: Change Cursor
#define ID_PREF_COLOR   2003 // Preferences: Choose Color

#define ID_LINE_DDA        3001
#define ID_LINE_MIDPOINT   3002
#define ID_LINE_PARAMETRIC 3003

#define ID_CIRCLE_DIRECT       4001
#define ID_CIRCLE_POLAR        4002
#define ID_CIRCLE_ITER_POLAR   4003
#define ID_CIRCLE_MIDPOINT     4004
#define ID_CIRCLE_MOD_MIDPOINT 4005

#define ID_ELLIPSE_DIRECT 5001
#define ID_ELLIPSE_POLAR  5002
#define ID_ELLIPSE_MID    5003

#define ID_CURVE_CARDINAL 6001

#define ID_FILL_CIRCLE_LINES   7001
#define ID_FILL_CIRCLE_CIRCLES 7002
#define ID_FILL_SQUARE_HERMIT  7003
#define ID_FILL_RECT_BEZIER    7004
#define ID_FILL_CONVEX         7005
#define ID_FILL_NONCONVEX      7006
#define ID_FILL_FLOOD_REC      7007
#define ID_FILL_FLOOD_NONREC   7008

#define ID_CLIP_RECT_POINT 8001
#define ID_CLIP_RECT_LINE  8002
#define ID_CLIP_RECT_POLY  8003
#define ID_CLIP_SQ_POINT   8004
#define ID_CLIP_SQ_LINE    8005

// New: Circle clipping window (center + radius, then line or point)
#define ID_CLIP_CIRCLE_LINE  8006
#define ID_CLIP_CIRCLE_POINT 8007

// Bonus: Smiley - Sad faces
#define ID_BONUS_HAPPY 9001
#define ID_BONUS_SAD   9002

// ════════════════════════════════════════════════════════════════════════════
// SHAPE STRUCT — this is how every drawn shape is stored in memory.
//
// - type   : a string identifying what kind of shape it is.
//            Used in RedrawShapes() to know which draw function to call,
//            and written to the save file so it can be reloaded.
//            Examples: "LINE_DDA", "CIRCLE_POLAR", "ELLIPSE_MID"
//
// - params : all the numbers needed to redraw the shape.
//            For a line: { x1, y1, x2, y2 }
//            For a circle: { cx, cy, radius }
//            For an ellipse: { cx, cy, rx, ry }
//            Each decides what params their shapes need.
//
// - color  : the COLORREF color the shape was drawn with.
//            Saved and restored so colors survive save/load.
// ════════════════════════════════════════════════════════════════════════════
struct Shape
{
    string type;
    vector<int> params;
    COLORREF color{};
};

// ── shapes vector: ALL drawn shapes live here ────────────────────────────
// Every must push_back a Shape here after drawing.
// This is what SaveToFile() writes and LoadFromFile() reads.
// RedrawShapes() loops over this to repaint after resize/load/clear.
vector<Shape> shapes;

// ════════════════════════════════════════════════════════════════════════════
// GLOBAL STATE — shared variables accessible by all functions
// ════════════════════════════════════════════════════════════════════════════

COLORREF currentColor = RGB(0, 0, 0); // active drawing color, set by color picker
bool useCustomCursor = false;         // toggles between arrow and crosshair cursor
bool whiteBg = false;         // toggles between gray and white background
HBRUSH bgBrush = NULL;          // handle to the current background brush

// activeAlgorithm — tracks which tool the user selected from the menu.
// When the user clicks the canvas, WM_LBUTTONDOWN checks this to know
// what to draw. Each sets this string when their menu item is clicked.
// Convention: "TYPENAME_VARIANT", e.g. "LINE_DDA", "CIRCLE_POLAR"
// Empty string "" means no tool is selected.
string activeAlgorithm = "";

// Two-click drawing state for lines.
// Other members will need similar variables for their shapes
// (e.g. circles need center click then radius click).
bool waitingForSecondClick = false;
int  x1Line = 0, y1Line = 0;

// circle click-state variables here
bool circleWaitingForRadius = false;
int  circleCX = 0, circleCY = 0;

// ellipse click-state
bool ellipseWaiting = false;
int  ellipseCX = 0, ellipseCY = 0;

// ── Cardinal Spline click-state ────────────────────────
// curveCollecting becomes true after the user enters tension in console.
// Points accumulate with each left-click; right-click finalises the spline.
bool          curveCollecting = false;
double        curveTension = 0.5;
vector<POINT> curvePoints;

// ── Circle-fill click-state  ────────────────────────────
// Two clicks define the circle (center then edge), quarter comes from console.
bool fillWaitingCenter = false;
bool fillWaitingEdge = false;
int  fillCX = 0, fillCY = 0;
int  fillQuarter = 1; // 1=top-right 2=top-left 3=bottom-left 4=bottom-right

// ── Hermite square fill click-state ──────────────────────────────────────
// Two clicks: top-left corner, then bottom-right corner.
bool hermiteWaitingSecond = false;
int  hermiteX1 = 0, hermiteY1 = 0;

// ── Bezier rectangle fill click-state ────────────────────────────────────
// Two clicks: any corner, then the opposite corner.
bool bezierRectWaitingSecond = false;
int  bezierX1 = 0, bezierY1 = 0;

// ── Smiley face click-state ────────────────────────────
// Two clicks: center then edge to set face radius.
bool smileyWaitingCenter = false;
bool smileyWaitingEdge = false;
int  smileyCX = 0, smileyCY = 0;

// ── Flood fill state ─────────────────────────────────────────────────────
struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

// ── 4-click polygon for flood fill ───────────────────────────────────────
// Now 4 clicks define the polygon; the 4th edge auto-closes back to point 1.
POINT pts[4];
int  pointCount = 0;
bool polygonDrawn = false;
bool useRecursive = true;

// ── 5: Add clipping click-state variables here ────────────────────
union outcode {
    struct { unsigned L : 1, R : 1, B : 1, T : 1; };
    unsigned all : 4;
};
typedef bool  (*InF)(Point& p, double edge);
typedef Point(*InterF)(Point& p1, Point& p2, double edge);
typedef vector<Point> polygonn;

// Rectangle clipping window bounds
double xLeft, xRight, yTop, yBottom;
// Line endpoints reused for clipping
double x1line, y1line, x2Line, y2Line;
// Square clipping window bounds
double sqLeft, sqRight, sqTop, sqBottom;
// Circle clipping window (center + radius)
double clipCircleCX = 0, clipCircleCY = 0, clipCircleR = 0;
bool   circleClipWaitingRadius = false;

// FIX 6: single unified clipState variable (the original had both
//        clipStage and clipState, which are different variables — only
//        clipState is actually read in the click handler).
int clipState = 0;

static int p1x, p1y;
static int p2x, p2y;

vector<Point> polyPoints;
bool polyCollect = false;

// ── Convex / Non-Convex polygon fill click-state ──────────────────────────
// Vertices are collected with left-clicks; right-click triggers the fill.
struct FillPoint { int x, y; };
vector<FillPoint> fillPolyPoints;
bool fillPolyCollecting = false;
// ════════════════════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS — tells the compiler these functions exist
// before they are defined later in the file
// ════════════════════════════════════════════════════════════════════════════
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HMENU CreateAppMenu();
void ClearScreen(HWND hwnd);
void SaveToFile(HWND hwnd);
void LoadFromFile(HWND hwnd);
void RedrawShapes(HDC hdc);

void LineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
void LineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
void LineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
void DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);

void CircleDirect(HDC hdc, int xc, int yc, int r, COLORREF c);
void CirclePolar(HDC hdc, int xc, int yc, int r, COLORREF c);
void CircleIterPolar(HDC hdc, int xc, int yc, int r, COLORREF c);
void CircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF c);
void CircleModMid(HDC hdc, int xc, int yc, int r, COLORREF c);
void CircleDraw(HDC hdc, int cx, int cy, int r, COLORREF c);

// ── 3: Declare ellipse + curve functions here ─────────────────────
void EllipseDirect(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c);
void EllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c);
void EllipsePolar(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c);
void EllipseDraw(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c);

// ── Curve functions ─────────────────────────────────────
void GetHermiteCoeff(double p0, double s0, double p1, double s1, double coeff[4]);
void DrawHermiteSeg(HDC hdc, int x0, int y0, int tx0, int ty0,
    int x1, int y1, int tx1, int ty1, COLORREF c, int numpts = 200);
void DrawCardinalSpline(HDC hdc, POINT P[], int n, double tension, COLORREF c);

// ── Circle fill functions ──────────────────────────────
void FillCircleWithLines(HDC hdc, int xc, int yc, int R, int quarter, COLORREF c);
void FillCircleWithCircles(HDC hdc, int xc, int yc, int R, int quarter, COLORREF c);

// ── Hermite & Bezier fill functions ──────────────────────────────
void FillSquareHermite(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);
void FillRectangleBezier(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c);

// ── Convex / Non-Convex fill ──────────────────────────────────────────────
struct Edge {
    float x;
    int   yMax;
    float mInv;
};
void ConvexFill(HDC hdc, vector<FillPoint>& polygon, COLORREF c);
void NonConvexFill(HDC hdc, vector<FillPoint>& polygon, COLORREF c);

// ── Flood fill functions ──────────────────────────────
void DrawPolygon(HDC hdc);
void RecursiveFloodFill(HDC hdc, int x, int y, COLORREF bc, COLORREF fc);
void NonRecursiveFloodFill(HDC hdc, int x, int y, COLORREF bc, COLORREF fc);
bool IsPointInsidePolygon(int x, int y);

// ── Smiley face helpers ─────────────────────────────────
void DrawArc(HDC hdc, int cx, int cy, int rx, int ry,
    double tStart, double tEnd, COLORREF c);
void DrawSmileyHappy(HDC hdc, int cx, int cy, int R, COLORREF c);
void DrawSmileySad(HDC hdc, int cx, int cy, int R, COLORREF c);

// ── 5: Declare clipping functions here ─────────────────────────────
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

// ════════════════════════════════════════════════════════════════════════════
// WinMain — entry point of a Win32 GUI application (equivalent to main())
//
// hInstance    : handle to this running instance of the program
// hPrevInstance: always NULL in modern Windows, ignore it
// lpCmdLine    : command line arguments as a string, we don't use it
// nCmdShow     : how to show the window (maximized, normal, etc.)
// ════════════════════════════════════════════════════════════════════════════
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    // Open a console window alongside the GUI window.
    // Without this, cout output goes nowhere in a Win32 app.
    // freopen_s redirects stdout and stdin to the console window.
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    cout << "[INFO] 2D Drawing Package started.\n";
    cout << "[INFO] Pick a tool from the menu, then click two points.\n";

    bgBrush = (HBRUSH)GetStockObject(LTGRAY_BRUSH);

    // WNDCLASS — describes our window to Windows OS before creating it.
    // Think of it as a blueprint/class definition for the window.
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW; // repaint when resized
    wc.lpfnWndProc = WndProc;                 // which function handles window events
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush;                 // background color of the window
    wc.lpszClassName = L"MyClass";              // internal name used by CreateWindow
    RegisterClass(&wc);

    // CreateWindow — actually creates the window using the blueprint above.
    // The menu bar is attached here via CreateAppMenu().
    HWND hwnd = CreateWindow(
        L"MyClass", L"2D Drawing Package",
        WS_OVERLAPPEDWINDOW,
        100, 100, 800, 600, // x, y, width, height
        NULL, CreateAppMenu(), hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop — keeps the program running.
    // GetMessage() waits for the next event (mouse click, key press, repaint, etc.)
    // TranslateMessage() converts key events into character events
    // DispatchMessage() sends the event to WndProc for handling
    // The loop exits when PostQuitMessage(0) is called (on window close).
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

// ════════════════════════════════════════════════════════════════════════════
// CreateAppMenu — builds the entire menu bar and returns it.
// Called once inside CreateWindow() above.
// To add items to a menu: AppendMenu(menuHandle, MF_STRING, YOUR_ID, L"Label")
// ════════════════════════════════════════════════════════════════════════════
HMENU CreateAppMenu()
{
    HMENU menuBar = CreateMenu(); // the top-level bar

    // ── File menu ────────────────────────────────────────────────────────
    HMENU fileMenu = CreatePopupMenu();
    AppendMenu(fileMenu, MF_STRING, ID_FILE_CLEAR, L"Clear Screen");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_SAVE, L"Save");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_LOAD, L"Load");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)fileMenu, L"File");

    // ── Preferences menu ─────────────────────────────────────────────────
    HMENU prefMenu = CreatePopupMenu();
    AppendMenu(prefMenu, MF_STRING, ID_PREF_WHITEBG, L"White Background");
    AppendMenu(prefMenu, MF_STRING, ID_PREF_CURSOR, L"Change Cursor");
    AppendMenu(prefMenu, MF_STRING, ID_PREF_COLOR, L"Choose Color");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)prefMenu, L"Preferences");

    // ── Lines menu ───────────────────────────────────────────────────────
    HMENU lineMenu = CreatePopupMenu();
    AppendMenu(lineMenu, MF_STRING, ID_LINE_DDA, L"DDA");
    AppendMenu(lineMenu, MF_STRING, ID_LINE_MIDPOINT, L"Midpoint");
    AppendMenu(lineMenu, MF_STRING, ID_LINE_PARAMETRIC, L"Parametric");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)lineMenu, L"Lines");

    // ── Circle menu ──────────────────────────────────────────────────────
    HMENU circleMenu = CreatePopupMenu();
    AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_DIRECT, L"Direct");
    AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_POLAR, L"Polar");
    AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_ITER_POLAR, L"Iterative Polar");
    AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_MIDPOINT, L"Midpoint");
    AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_MOD_MIDPOINT, L"Modified Midpoint");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)circleMenu, L"Circles");

    // ── Ellipse menu ─────────────────────────────────────────────────────
    HMENU ellipseMenu = CreatePopupMenu();
    AppendMenu(ellipseMenu, MF_STRING, ID_ELLIPSE_DIRECT, L"Direct");
    AppendMenu(ellipseMenu, MF_STRING, ID_ELLIPSE_MID, L"Midpoint");
    AppendMenu(ellipseMenu, MF_STRING, ID_ELLIPSE_POLAR, L"Polar");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)ellipseMenu, L"Ellipse");

    // ── Curves menu ──────────────────────────────────────────────────────
    HMENU curveMenu = CreatePopupMenu();
    AppendMenu(curveMenu, MF_STRING, ID_CURVE_CARDINAL, L"Cardinal Spline");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)curveMenu, L"Curves");

    // ── Filling menu ─────────────────────────────────────────────────────
    HMENU fillMenu = CreatePopupMenu();
    AppendMenu(fillMenu, MF_STRING, ID_FILL_CIRCLE_LINES, L"Circle with Lines");
    AppendMenu(fillMenu, MF_STRING, ID_FILL_CIRCLE_CIRCLES, L"Circle with Circles");
    AppendMenu(fillMenu, MF_STRING, ID_FILL_SQUARE_HERMIT, L"Square with Hermite");
    AppendMenu(fillMenu, MF_STRING, ID_FILL_RECT_BEZIER, L"Rectangle with Bezier");
    AppendMenu(fillMenu, MF_STRING, ID_FILL_CONVEX, L"Convex Polygon Fill");
    AppendMenu(fillMenu, MF_STRING, ID_FILL_NONCONVEX, L"Non-Convex Polygon Fill");
    AppendMenu(fillMenu, MF_STRING, ID_FILL_FLOOD_REC, L"Flood Fill (Recursive)");
    AppendMenu(fillMenu, MF_STRING, ID_FILL_FLOOD_NONREC, L"Flood Fill (Non-Recursive)");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)fillMenu, L"Filling");

    // ── Clipping menu ────────────────────────────────────────────────────
    // ── 5: Replace placeholder with real Clipping menu ────────────
    HMENU clipMenu = CreatePopupMenu();
    AppendMenu(clipMenu, MF_STRING, ID_CLIP_SQ_LINE, L"Square Line");
    AppendMenu(clipMenu, MF_STRING, ID_CLIP_SQ_POINT, L"Square Point");
    AppendMenu(clipMenu, MF_STRING, ID_CLIP_RECT_LINE, L"Rectangle Line");
    AppendMenu(clipMenu, MF_STRING, ID_CLIP_RECT_POLY, L"Rectangle Poly");
    AppendMenu(clipMenu, MF_STRING, ID_CLIP_RECT_POINT, L"Rectangle Point");
    AppendMenu(clipMenu, MF_STRING, ID_CLIP_CIRCLE_LINE, L"Circle Line");
    AppendMenu(clipMenu, MF_STRING, ID_CLIP_CIRCLE_POINT, L"Circle Point");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)clipMenu, L"Clipping");

    // ── Bonus menu ────────────────────────────────────────────────────────
    HMENU bonusMenu = CreatePopupMenu();
    AppendMenu(bonusMenu, MF_STRING, ID_BONUS_HAPPY, L"Happy Smiley");
    AppendMenu(bonusMenu, MF_STRING, ID_BONUS_SAD, L"Sad Smiley");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)bonusMenu, L"Bonus");

    return menuBar;
}

// ════════════════════════════════════════════════════════════════════════════
// ClearScreen — empties the shapes vector and forces a full window repaint.
// WM_PAINT fires after InvalidateRect, which calls RedrawShapes(hdc).
// Since shapes is now empty, nothing gets drawn → screen appears blank.
// ════════════════════════════════════════════════════════════════════════════
void ClearScreen(HWND hwnd)
{
    shapes.clear();
    InvalidateRect(hwnd, NULL, TRUE);

    // Also reset any in-progress drawing state so the next tool starts clean
    activeAlgorithm = "";
    waitingForSecondClick = false;
    circleWaitingForRadius = false;
    ellipseWaiting = false;
    curveCollecting = false;
    curvePoints.clear();
    fillWaitingCenter = false;
    fillWaitingEdge = false;
    smileyWaitingCenter = false;
    smileyWaitingEdge = false;
    hermiteWaitingSecond = false;
    bezierRectWaitingSecond = false;
    pointCount = 0;
    polygonDrawn = false;
    clipState = 0;       
    polyPoints.clear();
    circleClipWaitingRadius = false;
    fillPolyPoints.clear();
    fillPolyCollecting = false;

    // Clear the console window so the log is fresh for the next test
    system("cls");
    cout << "[INFO] 2D Drawing Package started.\n";
    cout << "[INFO] Pick a tool from the menu, then click two points.\n";
    cout << "[INFO] Canvas and console cleared — ready for next test.\n";
}

// ════════════════════════════════════════════════════════════════════════════
// SaveToFile — writes every shape in the shapes vector to a text file.
//
// File format (one shape per line):
//   TYPE COLOR PARAM_COUNT p1 p2 p3 ...
//   e.g: LINE_DDA 255 4 100 200 300 400
//
// Runs in a background thread so the window stays responsive while
// waiting for the user to type a filename in the console.
// ════════════════════════════════════════════════════════════════════════════
void SaveToFile(HWND hwnd)
{
    thread([]()
        {
            cout << "[FILE] Enter filename to save (ex. drawing.txt): ";
            string filename;
            cin >> filename;

            ofstream file(filename);
            if (!file.is_open()) { cout << "[ERROR] Cannot open file.\n"; return; }

            for (auto& s : shapes)
            {
                file << s.type << " " << (int)s.color << " " << s.params.size();
                for (int p : s.params) file << " " << p;
                file << "\n";
            }
            file.close();
            cout << "[FILE] Saved " << shapes.size() << " shapes to " << filename << "\n";
        }).detach();
}

// ════════════════════════════════════════════════════════════════════════════
// LoadFromFile — reads shapes from a text file back into the shapes vector,
// then triggers WM_PAINT to redraw them all via RedrawShapes().
//
// Also runs in a background thread for the same reason as SaveToFile.
// ════════════════════════════════════════════════════════════════════════════
void LoadFromFile(HWND hwnd)
{
    thread([hwnd]()
        {
            cout << "[FILE] Enter filename to load (ex. drawing.txt): ";
            string filename;
            cin >> filename;

            ifstream file(filename);
            if (!file.is_open()) { cout << "[ERROR] Cannot open file.\n"; return; }

            shapes.clear();
            string type;
            while (file >> type)
            {
                Shape s;
                s.type = type;
                int colorVal, count;
                file >> colorVal >> count;
                s.color = (COLORREF)colorVal;
                s.params.resize(count);
                for (int i = 0; i < count; i++) file >> s.params[i];
                shapes.push_back(s);
            }
            file.close();
            cout << "[FILE] Loaded " << shapes.size() << " shapes from " << filename << "\n";
            InvalidateRect(hwnd, NULL, FALSE); // triggers WM_PAINT → RedrawShapes
        }).detach();
}

// ════════════════════════════════════════════════════════════════════════════
// RedrawShapes — called every time WM_PAINT fires (on resize, clear, load).
// Loops over every saved shape and redraws it using the correct algorithm.
//
// HOW TO ADD YOUR SHAPES (all members):
//   1. Add an else-if block for your shape type string
//   2. Call your draw function with the params from s.params
//   3. Make sure your params order matches what you saved when drawing
//
// Example for circles (2):
//   else if (s.type == "CIRCLE_POLAR")
//       CirclePolar(hdc, s.params[0], s.params[1], s.params[2], s.color);
//   params[0]=cx, params[1]=cy, params[2]=radius
// ════════════════════════════════════════════════════════════════════════════
void RedrawShapes(HDC hdc)
{
    // Each type is checked independently with its own size guard.
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
        else if ((s.type == "FILL_CONVEX" || s.type == "FILL_NONCONVEX")
            && s.params.size() >= 1)
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

        // ── Smiley faces  — Params: [cx, cy, R] ─────────
        else if (s.type == "SMILEY_HAPPY" && s.params.size() >= 3)
            DrawSmileyHappy(hdc, s.params[0], s.params[1], s.params[2], s.color);
        else if (s.type == "SMILEY_SAD" && s.params.size() >= 3)
            DrawSmileySad(hdc, s.params[0], s.params[1], s.params[2], s.color);

        // ── 5: Clipping doesn't need redraw entries —
        //    it operates on existing shapes using the clipping window
    }
}

// ════════════════════════════════════════════════════════════════════════════
// LINE ALGORITHMS
// All three take the same parameters: HDC, start point, end point, color.
// HDC (Handle to Device Context) is the "canvas" you draw on.
// SetPixel(hdc, x, y, color) draws a single pixel — the core drawing call.
// ════════════════════════════════════════════════════════════════════════════

// DDA (Digital Differential Analyzer)
// Calculates how much x and y change per step using floating point.
// Steps = the longer axis (more horizontal → step in x, more vertical → step in y).
// Each iteration moves by xInc and yInc, rounding to nearest pixel.
void LineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = x2 - x1, dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    float xInc = (float)dx / steps; // how much to move in x each step
    float yInc = (float)dy / steps; // how much to move in y each step

    float x = (float)x1, y = (float)y1;
    for (int i = 0; i <= steps; i++)
    {
        SetPixel(hdc, (int)round(x), (int)round(y), c);
        x += xInc; y += yInc;
    }
}

// Midpoint Line Algorithm
// Uses only integer arithmetic (faster than DDA, no floating point).
// Decision variable d tells us whether to stay on the same row/column
// or move diagonally. d > 0 means move diagonally, d <= 0 stay straight.
void LineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x2 > x1) ? 1 : -1; // step direction in x (+1 right, -1 left)
    int sy = (y2 > y1) ? 1 : -1; // step direction in y (+1 down,  -1 up)
    int x = x1, y = y1;

    if (dx >= dy)
    {                            // more horizontal: step along x
        int d = 2 * dy - dx;   // initial decision variable
        int d1 = 2 * (dy - dx); // increment when d > 0 (diagonal step)
        int d2 = 2 * dy;        // increment when d <= 0 (horizontal step)
        for (int i = 0; i <= dx; i++)
        {
            SetPixel(hdc, x, y, c);
            if (d > 0) { y += sy; d += d1; }
            else d += d2;
            x += sx;
        }
    }
    else
    {                            // more vertical: step along y
        int d = 2 * dx - dy;
        int d1 = 2 * (dx - dy);
        int d2 = 2 * dx;
        for (int i = 0; i <= dy; i++)
        {
            SetPixel(hdc, x, y, c);
            if (d > 0) { x += sx; d += d1; }
            else d += d2;
            y += sy;
        }
    }
}

// Parametric Line Algorithm
// Uses parameter t that goes from 0.0 to 1.0.
// At t=0: point is (x1,y1). At t=1: point is (x2,y2).
// Formula: P(t) = P1 + t*(P2-P1) = (x1 + t*dx, y1 + t*dy)
void LineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = x2 - x1, dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    for (int i = 0; i <= steps; i++)
    {
        float t = (steps == 0) ? 0 : (float)i / steps; // t from 0 to 1
        SetPixel(hdc, (int)round(x1 + t * dx), (int)round(y1 + t * dy), c);
    }
}

// DrawLine — dispatcher that calls the correct algorithm based on activeAlgorithm.
// This is what WM_LBUTTONDOWN calls after getting both click points.
void DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    if (activeAlgorithm == "DDA")       LineDDA(hdc, x1, y1, x2, y2, c);
    else if (activeAlgorithm == "MIDPOINT")  LineMidpoint(hdc, x1, y1, x2, y2, c);
    else if (activeAlgorithm == "PARAMETRIC")LineParametric(hdc, x1, y1, x2, y2, c);
}

// ── 2: Add your circle functions below DrawLine ───────────────────
void CircleDraw(HDC hdc, int cx, int cy, int r, COLORREF c)
{
    if (activeAlgorithm == "MID")              CircleMidpoint(hdc, cx, cy, r, c);
    else if (activeAlgorithm == "Modified_Midpoint")CircleModMid(hdc, cx, cy, r, c);
    else if (activeAlgorithm == "DIRECT")           CircleDirect(hdc, cx, cy, r, c);
    else if (activeAlgorithm == "POLAR")            CirclePolar(hdc, cx, cy, r, c);
    else if (activeAlgorithm == "Iterative_POLAR")  CircleIterPolar(hdc, cx, cy, r, c);
}

// Draw with the 8 points of symmetry
void drawPoints(HDC hdc, int xc, int yc, int x, int y, COLORREF c)
{
    SetPixel(hdc, xc + x, yc + y, c); SetPixel(hdc, xc - x, yc + y, c);
    SetPixel(hdc, xc - x, yc - y, c); SetPixel(hdc, xc + x, yc - y, c);
    SetPixel(hdc, xc + y, yc + x, c); SetPixel(hdc, xc + y, yc - x, c);
    SetPixel(hdc, xc - y, yc - x, c); SetPixel(hdc, xc - y, yc + x, c);
}

void CircleDirect(HDC hdc, int xc, int yc, int R, COLORREF c)
{
    int x = 0, y = R;
    drawPoints(hdc, xc, yc, x, y, c);
    while (x < y)
    {
        x++;
        y = (int)round(sqrt((double)(R * R - x * x)));
        drawPoints(hdc, xc, yc, x, y, c);
    }
}

// using first order difference (DDA)
void CircleMidpoint(HDC hdc, int xc, int yc, int R, COLORREF c)
{
    int x = 0, y = R, d = 1 - R;
    drawPoints(hdc, xc, yc, x, y, c);
    while (x < y)
    {
        if (d < 0) d += 2 * x + 3;
        else { d += 2 * x - 2 * y + 5; y--; }
        x++;
        drawPoints(hdc, xc, yc, x, y, c);
    }
}

// using second order (fewer calculations — no multiplication, only addition)
void CircleModMid(HDC hdc, int xc, int yc, int R, COLORREF c)
{
    int x = 0, y = R, d = 1 - R;
    int c1 = 3, c2 = 5 - 2 * R;
    drawPoints(hdc, xc, yc, x, y, c);
    while (x < y)
    {
        if (d < 0) { d += c1; c2 += 2; }
        else { d += c2; c2 += 4; y--; }
        x++; c1 += 2;
        drawPoints(hdc, xc, yc, x, y, c);
    }
}

void CirclePolar(HDC hdc, int xc, int yc, int R, COLORREF c)
{
    double theta = 0, dtheta = 1.0 / R;
    int x = R, y = 0;
    drawPoints(hdc, xc, yc, x, y, c);
    while (x > y)
    {
        theta += dtheta;
        x = (int)round(R * cos(theta));
        y = (int)round(R * sin(theta));
        drawPoints(hdc, xc, yc, x, y, c);
    }
}

void CircleIterPolar(HDC hdc, int xc, int yc, int R, COLORREF c)
{
    double dtheta = 1.0 / R;
    double cosA = cos(dtheta), sinA = sin(dtheta);
    double x = R, y = 0;
    drawPoints(hdc, xc, yc, (int)round(x), (int)round(y), c);
    while (x > y)
    {
        double nx = x * cosA - y * sinA;
        y = x * sinA + y * cosA;
        x = nx;
        drawPoints(hdc, xc, yc, (int)round(x), (int)round(y), c);
    }
}

// ── 3: Add ellipse + cardinal spline functions here ───────────────
void EllipseDraw(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c)
{
    if (activeAlgorithm == "Ellipse_Direct")   EllipseDirect(hdc, xc, yc, rx, ry, c);
    else if (activeAlgorithm == "Ellipse_Midpoint") EllipseMidpoint(hdc, xc, yc, rx, ry, c);
    else if (activeAlgorithm == "Ellipse_Polar")    EllipsePolar(hdc, xc, yc, rx, ry, c);
}

void ellipsePoints(HDC hdc, int xc, int yc, int x, int y, COLORREF c)
{
    SetPixel(hdc, xc + x, yc + y, c); SetPixel(hdc, xc - x, yc + y, c);
    SetPixel(hdc, xc - x, yc - y, c); SetPixel(hdc, xc + x, yc - y, c);
}

// x^2/a^2 + y^2/b^2 = 1  (a: half width, b: half height)
void EllipseDirect(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c)
{
    for (int x = 0; x <= rx; x++)
    {
        double y = ry * sqrt(1.0 - (double)(x * x) / ((double)rx * rx));
        ellipsePoints(hdc, xc, yc, x, (int)round(y), c);
    }
    for (int y = 0; y <= ry; y++)
    {
        double x = rx * sqrt(1.0 - (double)(y * y) / ((double)ry * ry));
        ellipsePoints(hdc, xc, yc, (int)round(x), y, c);
    }
}

void EllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c)
{
    int x = 0, y = ry;
    double d1 = (double)(ry * ry) - (double)(rx * rx * ry) + 0.25 * (double)(rx * rx);

    // slope < 1 — curve more horizontal
    while (2.0 * ry * ry * x < 2.0 * rx * rx * y)
    {
        ellipsePoints(hdc, xc, yc, x, y, c);
        if (d1 < 0) { x++; d1 += 2.0 * ry * ry * x + ry * ry; }
        else { x++; y--; d1 += 2.0 * ry * ry * x - 2.0 * rx * rx * y + ry * ry; }
    }

    double d2 = (double)(ry * ry) * (x + 0.5) * (x + 0.5)
        + (double)(rx * rx) * (y - 1) * (y - 1)
        - (double)(rx * rx * ry * ry);
    // slope > 1 — more vertical
    while (y >= 0)
    {
        ellipsePoints(hdc, xc, yc, x, y, c);
        if (d2 > 0) { y--; d2 += rx * rx - 2.0 * rx * rx * y; }
        else { y--; x++; d2 += 2.0 * ry * ry * x - 2.0 * rx * rx * y + rx * rx; }
    }
}

void EllipsePolar(HDC hdc, int xc, int yc, int rx, int ry, COLORREF c)
{
    double theta = 0, dtheta = 1.0 / max(rx, ry);
    // from 0 to 90 degrees
    while (theta <= M_PI / 2.0)
    {
        int x = (int)round(rx * cos(theta));
        int y = (int)round(ry * sin(theta));
        ellipsePoints(hdc, xc, yc, x, y, c);
        theta += dtheta;
    }
}

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
    coeff[2] = s0;                         // t¹
    coeff[3] = p0;                         // t⁰
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
        double t = i * dt, t2 = t * t, t3 = t2 * t;
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
        DrawHermiteSeg(hdc, P[i - 1].x, P[i - 1].y, tx0, ty0,
            P[i].x, P[i].y, tx1, ty1, c);
        tx0 = tx1; ty0 = ty1;
    }
}

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
}

// Fill the selected quarter of a circle with concentric circle arcs.
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
    for (int r = 5; r <= R; r += 5)
    {
        double dtheta = 1.0 / r;
        for (double theta = tStart; theta <= tEnd + dtheta / 2.0; theta += dtheta)
        {
            int x = xc + (int)round(r * cos(theta));
            int y = yc + (int)round(r * sin(theta));
            SetPixel(hdc, x, y, c);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// FILL SQUARE WITH HERMITE CURVES
//
// Fills the bounding rectangle [x1,y1]-[x2,y2] with vertical cubic Hermite
// curves. Each curve runs from (xCol, top) to (xCol, bottom) with a
// horizontal tension tangent, producing solid vertical fill lines.
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
    // ── SQUARE FIX: clamp both dimensions to the shorter side ────────────
    int side = min(abs(x2 - x1), abs(y2 - y1));
    int right = left + side;
    int bottom = top + side;
    // ─────────────────────────────────────────────────────────────────────

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

// ════════════════════════════════════════════════════════════════════════════
// FILL RECTANGLE WITH BEZIER CURVES
//
// Fills [x1,y1]-[x2,y2] by stacking one horizontal cubic Bezier per row.
// All four control points share the same y → the curve is a straight line,
// giving solid pixel-perfect horizontal fill.
//
// Params saved/loaded: [x1, y1, x2, y2]
// ════════════════════════════════════════════════════════════════════════════

// Evaluate a single cubic Bezier point at parameter t.
static void BezierPoint(double p0x, double p0y, double p1x, double p1y,
    double p2x, double p2y, double p3x, double p3y,
    double t, double& outX, double& outY)
{
    double u = 1.0 - t, u2 = u * u, u3 = u2 * u, t2 = t * t, t3 = t2 * t;
    outX = u3 * p0x + 3 * t * u2 * p1x + 3 * t2 * u * p2x + t3 * p3x;
    outY = u3 * p0y + 3 * t * u2 * p1y + 3 * t2 * u * p2y + t3 * p3y;
}

void FillRectangleBezier(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int left = min(x1, x2), right = max(x1, x2);
    int top = min(y1, y2), bottom = max(y1, y2);
    int width = right - left, height = bottom - top;
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

// DrawPolygon — draws the 4-sided closed polygon from pts[].
// The 4th vertex connects back to the 1st automatically (% 4 wraps it).
void DrawPolygon(HDC hdc)
{
    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4; // wraps: edge 3→0 closes the polygon
        LineMidpoint(hdc, pts[i].x, pts[i].y, pts[next].x, pts[next].y, RGB(0, 0, 0));
    }
}

void RecursiveFloodFill(HDC hdc, int x, int y, COLORREF bc, COLORREF fc)
{
    COLORREF cur = GetPixel(hdc, x, y);
    if (cur == bc || cur == fc) return;
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
        COLORREF cur = GetPixel(hdc, p.x, p.y);
        if (cur == bc || cur == fc) continue;
        SetPixel(hdc, p.x, p.y, fc);
        s.push(Point(p.x + 1, p.y)); s.push(Point(p.x - 1, p.y));
        s.push(Point(p.x, p.y + 1)); s.push(Point(p.x, p.y - 1));
    }
}

// IsPointInsidePolygon — ray-casting algorithm, works for any simple polygon.
// Now tests against 4 edges (pts[0..3]).
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

// ════════════════════════════════════════════════════════════════════════════
// BONUS: SMILEY FACES
//
// Uses CircleMidpoint and LineMidpoint.
// Mouth is drawn as a parametric arc (portion of an ellipse).
// ════════════════════════════════════════════════════════════════════════════
void DrawArc(HDC hdc, int cx, int cy, int rx, int ry,
    double tStart, double tEnd, COLORREF c)
{
    double dtheta = 1.0 / max(rx, ry); // from polar circle lecture
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
// Face + eyes: CircleMidpoint, Nose: LineMidpoint,
// Smile: lower half of an ellipse arc (θ = 0 → π, bottom arc = U shape).
void DrawSmileyHappy(HDC hdc, int cx, int cy, int R, COLORREF c)
{
    CircleMidpoint(hdc, cx, cy, R, c);
    CircleMidpoint(hdc, cx - R / 3, cy - R / 4, R / 8, c);
    CircleMidpoint(hdc, cx + R / 3, cy - R / 4, R / 8, c);
    LineMidpoint(hdc, cx, cy, cx - R / 10, cy + R / 6, c);
    LineMidpoint(hdc, cx, cy, cx + R / 10, cy + R / 6, c);
    DrawArc(hdc, cx, cy + R / 4, R / 2, R / 4, 0, M_PI, c);
}

// Sad smiley face.
// Same structure but frown = top half of ellipse arc (θ = π → 2π,
// which sweeps: left-corner → top-middle → right-corner, an upside-down U).
void DrawSmileySad(HDC hdc, int cx, int cy, int R, COLORREF c)
{
    CircleMidpoint(hdc, cx, cy, R, c);
    CircleMidpoint(hdc, cx - R / 3, cy - R / 4, R / 8, c);
    CircleMidpoint(hdc, cx + R / 3, cy - R / 4, R / 8, c);
    LineMidpoint(hdc, cx, cy, cx - R / 10, cy + R / 6, c);
    LineMidpoint(hdc, cx, cy, cx + R / 10, cy + R / 6, c);
    DrawArc(hdc, cx, cy + R / 2, R / 2, R / 6, M_PI, 2 * M_PI, c);
}

// ── 5: Add all clipping functions here ─────────────────────────────

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
    // Draw only if both endpoints are now inside
    if (!out1.all && !out2.all)
        LineMidpoint(hdc, (int)round(x1), (int)round(y1),
            (int)round(x2), (int)round(y2), RGB(255, 0, 0));
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

// Sutherland-Hodgman polygon clipping.
// Draws only the inside portion — no red coloring, just the clipped result.

void polygonclip(HDC hdc, Point* p, int n,
    double xleft, double xright, double ybottom, double ytop)
{
    polygonn vlist;
    for (int i = 0; i < n; i++) vlist.push_back(Point(p[i].x, p[i].y));

    // Clip against each of the four window edges in turn
    vlist = clipEdge(vlist, xleft, Inleft, VIntersect);
    vlist = clipEdge(vlist, xright, Inright, VIntersect);
    vlist = clipEdge(vlist, ytop, Intop, HIntersect);
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
//
// The circle window is defined by center (clipCircleCX, clipCircleCY)
// and radius clipCircleR.
//
// PointInsideCircleWindow: true when the Euclidean distance from the
//   point to the circle center is <= clipCircleR.
//
// ClipLineToCircle: parametric line-circle intersection.
//   Solves |P(t)|² = R² for t ∈ [0,1] where P(t) = (x1,y1) + t*(dx,dy).
//   Returns false (trivially reject) when no intersection exists and
//   both endpoints are outside.  Returns true when the visible sub-segment
//   [ox1,oy1]-[ox2,oy2] has been computed.
// ════════════════════════════════════════════════════════════════════════════
bool PointInsideCircleWindow(double x, double y)
{
    double dx = x - clipCircleCX, dy = y - clipCircleCY;
    return (dx * dx + dy * dy) <= clipCircleR * clipCircleR;
}

// Clip a line segment to the circle window.
// Returns true  → the clipped segment is in [ox1,oy1]-[ox2,oy2].
// Returns false → segment is entirely outside the circle.
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

// ════════════════════════════════════════════════════════════════════════════
// WndProc — the heart of the Win32 app. Every event (click, repaint, menu
// selection, window close) comes here as a "message".
//
// msg      : what happened (WM_LBUTTONDOWN, WM_COMMAND, WM_PAINT, etc.)
// wParam   : extra info (for WM_COMMAND: which menu item was clicked)
// lParam   : extra info (for WM_LBUTTONDOWN: mouse x/y packed together)
// ════════════════════════════════════════════════════════════════════════════
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // ── WM_COMMAND: fires when any menu item is clicked ───────────────
        // LOWORD(wParam) gives the ID of the clicked item.
        // Match it against your #define IDs and call the right function.
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
            // ── File ──────────────────────────────────────────────────
        case ID_FILE_CLEAR: ClearScreen(hwnd); break;
        case ID_FILE_SAVE:  SaveToFile(hwnd);  break;
        case ID_FILE_LOAD:  LoadFromFile(hwnd); break;

            // ── Preferences ───────────────────────────────────────────
        case ID_PREF_WHITEBG:
        {
            // Toggle background between white and light gray.
            whiteBg = !whiteBg;
            if (bgBrush) DeleteObject(bgBrush);
            bgBrush = CreateSolidBrush(whiteBg ? RGB(255, 255, 255) : RGB(211, 211, 211));
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);
            InvalidateRect(hwnd, NULL, TRUE);
            cout << "[PREF] Background: " << (whiteBg ? "white" : "gray") << "\n";
            break;
        }
        case ID_PREF_CURSOR:
        {
            // Toggle between default arrow and crosshair cursor.
            useCustomCursor = !useCustomCursor;
            HCURSOR cur = LoadCursor(NULL, useCustomCursor ? IDC_CROSS : IDC_ARROW);
            SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR)cur);
            cout << "[PREF] Cursor: " << (useCustomCursor ? "crosshair" : "arrow") << "\n";
            break;
        }
        case ID_PREF_COLOR:
        {
            // Opens the Windows built-in color picker dialog.
            CHOOSECOLOR cc = {};
            static COLORREF custom[16] = {};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = custom;
            cc.rgbResult = currentColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColor(&cc))
            {
                currentColor = cc.rgbResult;
                cout << "[PREF] Color: R=" << (int)GetRValue(currentColor)
                    << " G=" << (int)GetGValue(currentColor)
                    << " B=" << (int)GetBValue(currentColor) << "\n";
            }
            break;
        }

        // ── Lines ────────────────────────────────────────────────
        // Each case sets activeAlgorithm so WM_LBUTTONDOWN knows
        // what to draw. Also resets the click state so a fresh
        // two-click sequence starts cleanly.
        case ID_LINE_DDA:
            activeAlgorithm = "DDA"; waitingForSecondClick = false;
            cout << "[LINE] DDA selected. Click start point.\n"; break;
        case ID_LINE_MIDPOINT:
            activeAlgorithm = "MIDPOINT"; waitingForSecondClick = false;
            cout << "[LINE] Midpoint selected. Click start point.\n"; break;
        case ID_LINE_PARAMETRIC:
            activeAlgorithm = "PARAMETRIC"; waitingForSecondClick = false;
            cout << "[LINE] Parametric selected. Click start point.\n"; break;

            // ── Circles ──────────────────────────────────────────────
        case ID_CIRCLE_DIRECT:
            activeAlgorithm = "DIRECT"; circleWaitingForRadius = false;
            cout << "[CIRCLE] Direct selected. Click center.\n"; break;
        case ID_CIRCLE_POLAR:
            activeAlgorithm = "POLAR"; circleWaitingForRadius = false;
            cout << "[CIRCLE] Polar selected. Click center.\n"; break;
        case ID_CIRCLE_ITER_POLAR:
            activeAlgorithm = "Iterative_POLAR"; circleWaitingForRadius = false;
            cout << "[CIRCLE] Iterative Polar selected. Click center.\n"; break;
        case ID_CIRCLE_MIDPOINT:
            activeAlgorithm = "MID"; circleWaitingForRadius = false;
            cout << "[CIRCLE] Midpoint selected. Click center.\n"; break;
        case ID_CIRCLE_MOD_MIDPOINT:
            activeAlgorithm = "Modified_Midpoint"; circleWaitingForRadius = false;
            cout << "[CIRCLE] Modified Midpoint selected. Click center.\n"; break;

            // ── Ellipses ─────────────────────────────────────────────
        case ID_ELLIPSE_DIRECT:
            activeAlgorithm = "Ellipse_Direct"; ellipseWaiting = false;
            cout << "[ELLIPSE] Direct selected. Click center.\n"; break;
        case ID_ELLIPSE_MID:
            activeAlgorithm = "Ellipse_Midpoint"; ellipseWaiting = false;
            cout << "[ELLIPSE] Midpoint selected. Click center.\n"; break;
        case ID_ELLIPSE_POLAR:
            activeAlgorithm = "Ellipse_Polar"; ellipseWaiting = false;
            cout << "[ELLIPSE] Polar selected. Click center.\n"; break;

            // ── Cardinal Spline ───────────────────────────────────────
        case ID_CURVE_CARDINAL:
            activeAlgorithm = "CURVE_CARDINAL";
            curveCollecting = false; curvePoints.clear();
            // Ask tension in a background thread so the GUI stays live
            thread([]() {
                cout << "[CURVE] Cardinal Spline selected.\n";
                cout << "[CURVE] Enter tension (0.0 = smooth, 1.0 = straight): ";
                cin >> curveTension;
                if (curveTension < 0) curveTension = 0;
                if (curveTension > 1) curveTension = 1;
                curveCollecting = true;
                cout << "[CURVE] Click at least 4 points on the canvas.\n";
                cout << "[CURVE] Right-click when done.\n";
                }).detach();
            break;

            // ── Circle fill ───────────────────────────────────────────
        case ID_FILL_CIRCLE_LINES:
            activeAlgorithm = "FILL_CIRCLE_LINES";
            fillWaitingCenter = false; fillWaitingEdge = false;
            thread([]() {
                cout << "[FILL] Fill Circle with Lines.\n";
                cout << "[FILL] Enter quarter (1=top-right, 2=top-left, "
                    "3=bottom-left, 4=bottom-right): ";
                cin >> fillQuarter;
                if (fillQuarter < 1 || fillQuarter > 4) fillQuarter = 1;
                fillWaitingCenter = true;
                cout << "[FILL] Click circle center.\n";
                }).detach();
            break;
        case ID_FILL_CIRCLE_CIRCLES:
            activeAlgorithm = "FILL_CIRCLE_CIRCLES";
            fillWaitingCenter = false; fillWaitingEdge = false;
            thread([]() {
                cout << "[FILL] Fill Circle with Circles.\n";
                cout << "[FILL] Enter quarter (1=top-right, 2=top-left, "
                    "3=bottom-left, 4=bottom-right): ";
                cin >> fillQuarter;
                if (fillQuarter < 1 || fillQuarter > 4) fillQuarter = 1;
                fillWaitingCenter = true;
                cout << "[FILL] Click circle center.\n";
                }).detach();
            break;

            // ── Hermite / Bezier fill ─────────────────────────────────
        case ID_FILL_SQUARE_HERMIT:
            activeAlgorithm = "FILL_SQUARE_HERMIT"; hermiteWaitingSecond = false;
            cout << "[FILL] Square with Hermite selected. Click first corner.\n"; break;
        case ID_FILL_RECT_BEZIER:
            activeAlgorithm = "FILL_RECT_BEZIER"; bezierRectWaitingSecond = false;
            cout << "[FILL] Rectangle with Bezier selected. Click first corner.\n"; break;

            // ── Convex / Non-Convex polygon fill ─────────────────────────────
        case ID_FILL_CONVEX:
            activeAlgorithm = "FILL_CONVEX";
            fillPolyPoints.clear(); fillPolyCollecting = true;
            cout << "[FILL] Convex Polygon Fill selected.\n";
            cout << "[FILL] Left-click to add vertices. Right-click to fill.\n"; break;
        case ID_FILL_NONCONVEX:
            activeAlgorithm = "FILL_NONCONVEX";
            fillPolyPoints.clear(); fillPolyCollecting = true;
            cout << "[FILL] Non-Convex Polygon Fill selected.\n";
            cout << "[FILL] Left-click to add vertices. Right-click to fill.\n"; break;
          
            // ── Flood fill ────────────────────────────────────────────
        case ID_FILL_FLOOD_REC:
            activeAlgorithm = "recursive"; useRecursive = true;
            pointCount = 0; polygonDrawn = false;
            cout << "[FLOOD] Recursive Flood Fill — click 4 points for polygon.\n"; break;
        case ID_FILL_FLOOD_NONREC:
            activeAlgorithm = "non_recursive"; useRecursive = false;
            pointCount = 0; polygonDrawn = false;
            cout << "[FLOOD] Non-Recursive Flood Fill — click 4 points for polygon.\n"; break;

            // ── Smiley faces ──────────────────────────────────────────
        case ID_BONUS_HAPPY:
            activeAlgorithm = "SMILEY_HAPPY";
            smileyWaitingCenter = true; smileyWaitingEdge = false;
            cout << "[BONUS] Happy Smiley selected. Click face center.\n"; break;
        case ID_BONUS_SAD:
            activeAlgorithm = "SMILEY_SAD";
            smileyWaitingCenter = true; smileyWaitingEdge = false;
            cout << "[BONUS] Sad Smiley selected. Click face center.\n"; break;

            // ── Clipping ─────────────────────────────────────────────


        case ID_CLIP_RECT_LINE:
            activeAlgorithm = "CLIP_RECT_LINE";
            clipState = 0; polyPoints.clear();
            cout << "[CLIP] Rect Line : click P1, P2, third point for height, "
                "then line start + end.\n"; break;
        case ID_CLIP_RECT_POINT:
            activeAlgorithm = "CLIP_RECT_POINT";
            clipState = 0; polyPoints.clear();
            cout << "[CLIP] Rect Point : click P1, P2, third point for height, "
                "then click points to test.\n"; break;
        case ID_CLIP_RECT_POLY:
            activeAlgorithm = "CLIP_RECT_POLY";
            clipState = 0; polyPoints.clear();
            cout << "[CLIP] Rect Poly : click P1, P2, third point for height, "
                "then click polygon vertices. Right-click to clip.\n"; break;
        case ID_CLIP_SQ_LINE:
            activeAlgorithm = "CLIP_SQ_LINE";
            clipState = 0; polyPoints.clear();
            cout << "[CLIP] Square Line : click center, then side point, "
                "then line start + end.\n"; break;
        case ID_CLIP_SQ_POINT:
            activeAlgorithm = "CLIP_SQ_POINT";
            clipState = 0; polyPoints.clear();
            cout << "[CLIP] Square Point : click center, then side point, "
                "then click points to test.\n"; break;
        case ID_CLIP_CIRCLE_LINE:
            activeAlgorithm = "CLIP_CIRCLE_LINE";
            clipState = 0; circleClipWaitingRadius = false;
            cout << "[CLIP] Circle Line : click circle center, then edge point "
                "to define window, then line start + end.\n"; break;
        case ID_CLIP_CIRCLE_POINT:
            activeAlgorithm = "CLIP_CIRCLE_POINT";
            clipState = 0; circleClipWaitingRadius = false;
            cout << "[CLIP] Circle Point : click circle center, then edge point "
                "to define window, then click points to test.\n"; break;
        }
        break; // FIX 1: this break was missing in the original — it caused
        // every menu click to fall through into WM_LBUTTONDOWN with
        // a garbage lParam, triggering spurious drawing actions.
    }

    // ── WM_LBUTTONDOWN: fires on every left mouse click ───────────────
    // LOWORD(lParam) = mouse X,  HIWORD(lParam) = mouse Y
    // The logic here is a two-click system:
    //   Click 1 → store start point, set waitingForSecondClick = true
    //   Click 2 → draw the shape, save to shapes vector, reset state
    case WM_LBUTTONDOWN:
    {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        cout << "[MOUSE] Click at (" << mx << ", " << my << ")\n";

        // ── Lines ─────────────────────────────────────────────────────
        if (activeAlgorithm == "DDA" ||
            activeAlgorithm == "MIDPOINT" ||
            activeAlgorithm == "PARAMETRIC")
        {
            if (!waitingForSecondClick)
            {
                x1Line = mx; y1Line = my;
                waitingForSecondClick = true;
                cout << "[LINE] Start point set. Click end point.\n";
            }
            else
            {
                HDC hdc = GetDC(hwnd);
                DrawLine(hdc, x1Line, y1Line, mx, my, currentColor);
                ReleaseDC(hwnd, hdc);

                // Save shape so it survives repaint, clear, save, and load
                Shape s;
                s.type = "LINE_" + activeAlgorithm;
                s.color = currentColor;
                s.params = { x1Line, y1Line, mx, my };
                shapes.push_back(s);

                cout << "[LINE] Drew " << activeAlgorithm
                    << " from (" << x1Line << "," << y1Line
                    << ") to (" << mx << "," << my << ")\n";
                waitingForSecondClick = false;
            }
            return 0;
        }

        // ── Circles ───────────────────────────────────────────────────
        if (activeAlgorithm == "POLAR" ||
            activeAlgorithm == "Iterative_POLAR" ||
            activeAlgorithm == "DIRECT" ||
            activeAlgorithm == "MID" ||
            activeAlgorithm == "Modified_Midpoint")
        {
            if (!circleWaitingForRadius)
            {
                circleCX = mx; circleCY = my;
                circleWaitingForRadius = true;
                cout << "[CIRCLE] Center set. Click edge point.\n";
            }
            else
            {
                int r = (int)sqrt(pow((double)(mx - circleCX), 2.0) +
                    pow((double)(my - circleCY), 2.0));
                HDC hdc = GetDC(hwnd);
                CircleDraw(hdc, circleCX, circleCY, r, currentColor);
                ReleaseDC(hwnd, hdc);

                Shape s;
                
                
                if (activeAlgorithm == "DIRECT")           s.type = "CIRCLE_DIRECT";
                else if (activeAlgorithm == "POLAR")            s.type = "CIRCLE_POLAR";
                else if (activeAlgorithm == "Iterative_POLAR")  s.type = "CIRCLE_ITER_POLAR";
                else if (activeAlgorithm == "MID")              s.type = "CIRCLE_MIDPOINT";
                else                                            s.type = "CIRCLE_MOD_MIDPOINT";
                s.color = currentColor;
                s.params = { circleCX, circleCY, r };
                shapes.push_back(s);

                cout << "[CIRCLE] Drew " << activeAlgorithm
                    << " center=(" << circleCX << "," << circleCY
                    << ") r=" << r << "\n";
                circleWaitingForRadius = false;
            }
            return 0;
        }

        // ── Ellipses ──────────────────────────────────────────────────
        // Two clicks: center, then corner of bounding box.
        // rx = abs(mx - cx),  ry = abs(my - cy)
        if (activeAlgorithm == "Ellipse_Direct" ||
            activeAlgorithm == "Ellipse_Midpoint" ||
            activeAlgorithm == "Ellipse_Polar")
        {
            if (!ellipseWaiting)
            {
                ellipseCX = mx; ellipseCY = my;
                ellipseWaiting = true;
                cout << "[ELLIPSE] Center set. Click edge point.\n";
            }
            else
            {
                int rx = abs(mx - ellipseCX);
                int ry = abs(my - ellipseCY);
                HDC hdc = GetDC(hwnd);
                EllipseDraw(hdc, ellipseCX, ellipseCY, rx, ry, currentColor);
                ReleaseDC(hwnd, hdc);

                Shape s;
                
                if (activeAlgorithm == "Ellipse_Direct")   s.type = "ELLIPSE_DIRECT";
                else if (activeAlgorithm == "Ellipse_Midpoint") s.type = "ELLIPSE_MIDPOINT";
                else                                            s.type = "ELLIPSE_POLAR";
                s.color = currentColor;
                s.params = { ellipseCX, ellipseCY, rx, ry };
                shapes.push_back(s);

                cout << "[ELLIPSE] Drew " << activeAlgorithm
                    << " center=(" << ellipseCX << "," << ellipseCY
                    << ") rx=" << rx << " ry=" << ry << "\n";
                ellipseWaiting = false;
            }
            return 0;
        }

        // ── Cardinal Spline click collection ──────────────────────────
        if (activeAlgorithm == "CURVE_CARDINAL" && curveCollecting)
        {
            POINT pt = { mx, my };
            curvePoints.push_back(pt);
            cout << "[CURVE] Point " << curvePoints.size()
                << " added at (" << mx << "," << my << ").\n";
            HDC hdc = GetDC(hwnd);
            Ellipse(hdc, mx - 3, my - 3, mx + 3, my + 3);
            ReleaseDC(hwnd, hdc);
            return 0;
        }

        // ── Circle fill click handling ─────────────────────────────────
        if ((activeAlgorithm == "FILL_CIRCLE_LINES" ||
            activeAlgorithm == "FILL_CIRCLE_CIRCLES") && fillWaitingCenter)
        {
            if (!fillWaitingEdge)
            {
                fillCX = mx; fillCY = my;
                fillWaitingEdge = true;
                cout << "[FILL] Center set. Click edge point.\n";
            }
            else
            {
                int dx = mx - fillCX, dy = my - fillCY;
                int R = (int)round(sqrt((double)(dx * dx + dy * dy)));
                HDC hdc = GetDC(hwnd);
                if (activeAlgorithm == "FILL_CIRCLE_LINES")
                    FillCircleWithLines(hdc, fillCX, fillCY, R, fillQuarter, currentColor);
                else
                    FillCircleWithCircles(hdc, fillCX, fillCY, R, fillQuarter, currentColor);
                ReleaseDC(hwnd, hdc);
                Shape s;
                s.type = activeAlgorithm;
                s.color = currentColor;
                s.params = { fillCX, fillCY, R, fillQuarter };
                shapes.push_back(s);
                cout << "[FILL] Drew " << activeAlgorithm
                    << " center=(" << fillCX << "," << fillCY
                    << ") R=" << R << " Q=" << fillQuarter << "\n";
                fillWaitingCenter = false; fillWaitingEdge = false;
            }
            return 0;
        }

        // ── Fill Square with Hermite ───────────────────────────────────
        // Click 1: store first corner.
        // Click 2: draw filled rectangle, save shape.
        if (activeAlgorithm == "FILL_SQUARE_HERMIT")
        {
            if (!hermiteWaitingSecond)
            {
                hermiteX1 = mx; hermiteY1 = my;
                hermiteWaitingSecond = true;
                cout << "[FILL] First corner set. Click opposite corner.\n";
            }
            else
            {
                HDC hdc = GetDC(hwnd);
                FillSquareHermite(hdc, hermiteX1, hermiteY1, mx, my, currentColor);
                ReleaseDC(hwnd, hdc);
                Shape s;
                s.type = "FILL_SQUARE_HERMIT";
                s.color = currentColor;
                s.params = { hermiteX1, hermiteY1, mx, my };
                shapes.push_back(s);
                cout << "[FILL] Hermite square drawn.\n";
                hermiteWaitingSecond = false;
            }
            return 0;
        }

        // ── Fill Rectangle with Bezier ─────────────────────────────────
        // Click 1: store first corner.
        // Click 2: draw filled rectangle with border, save shape.
        if (activeAlgorithm == "FILL_RECT_BEZIER")
        {
            if (!bezierRectWaitingSecond)
            {
                bezierX1 = mx; bezierY1 = my;
                bezierRectWaitingSecond = true;
                cout << "[FILL] First corner set. Click opposite corner.\n";
            }
            else
            {
                HDC hdc = GetDC(hwnd);
                FillRectangleBezier(hdc, bezierX1, bezierY1, mx, my, currentColor);
                ReleaseDC(hwnd, hdc);
                Shape s;
                s.type = "FILL_RECT_BEZIER";
                s.color = currentColor;
                s.params = { bezierX1, bezierY1, mx, my };
                shapes.push_back(s);
                cout << "[FILL] Bezier rectangle drawn.\n";
                bezierRectWaitingSecond = false;
            }
            return 0;
        }
        // ── Convex / Non-Convex polygon fill click collection ──────────
        if ((activeAlgorithm == "FILL_CONVEX" || activeAlgorithm == "FILL_NONCONVEX")
            && fillPolyCollecting)
        {
            fillPolyPoints.push_back({ mx, my });
            HDC hdc = GetDC(hwnd);
            // Mark vertex with a small dot
            for (int dy2 = -2; dy2 <= 2; dy2++)
                for (int dx2 = -2; dx2 <= 2; dx2++)
                    SetPixel(hdc, mx + dx2, my + dy2, RGB(255, 0, 0));
            // Draw edge to previous vertex
            int n = (int)fillPolyPoints.size();
            if (n > 1)
                LineMidpoint(hdc,
                    fillPolyPoints[n - 2].x, fillPolyPoints[n - 2].y,
                    mx, my, RGB(0, 0, 0));
            ReleaseDC(hwnd, hdc);
            cout << "[FILL] Vertex " << n << " added at (" << mx << "," << my << ").\n";
            return 0;
        }

        // ── Flood Fill ────────────────────────────────────────────────
        if (activeAlgorithm == "recursive" ||
            activeAlgorithm == "non_recursive")
        {
            HDC hdc = GetDC(hwnd);

            if (!polygonDrawn)
            {
                // Collect 4 vertices — the 4th automatically closes back to vertex 1
                pts[pointCount++] = { mx, my };
                // Mark clicked point in red for visibility
                SetPixel(hdc, mx, my, RGB(255, 0, 0));
                SetPixel(hdc, mx + 1, my, RGB(255, 0, 0));
                SetPixel(hdc, mx, my + 1, RGB(255, 0, 0));
                SetPixel(hdc, mx + 1, my + 1, RGB(255, 0, 0));
                cout << "[FLOOD] Point " << pointCount << " added.\n";

                if (pointCount == 4)
                {
                    // DrawPolygon draws edges 0→1, 1→2, 2→3, and 3→0 (auto-closed)
                    DrawPolygon(hdc);
                    polygonDrawn = true;
                    cout << "[FLOOD] Polygon complete (auto-closed). Click INSIDE to fill.\n";
                }
            }
            else
            {
                if (IsPointInsidePolygon(mx, my))
                {
                    COLORREF bc = RGB(0, 0, 0), fc = currentColor;
                    if (activeAlgorithm == "recursive")
                    {
                        RecursiveFloodFill(hdc, mx, my, bc, fc);
                        cout << "[FLOOD] Recursive fill done.\n";
                    }
                    else
                    {
                        NonRecursiveFloodFill(hdc, mx, my, bc, fc);
                        cout << "[FLOOD] Non-recursive fill done.\n";
                    }
                }
                else
                    MessageBox(hwnd, L"Click INSIDE the polygon!", L"Outside Polygon",
                        MB_OK | MB_ICONWARNING);
            }

            ReleaseDC(hwnd, hdc);
            return 0;
        }

        // ── Smiley face click handling ─────────────────────────────────
        if ((activeAlgorithm == "SMILEY_HAPPY" || activeAlgorithm == "SMILEY_SAD")
            && smileyWaitingCenter)
        {
            if (!smileyWaitingEdge)
            {
                smileyCX = mx; smileyCY = my;
                smileyWaitingEdge = true;
                cout << "[BONUS] Center set. Click to set face radius.\n";
            }
            else
            {
                int dx = mx - smileyCX, dy = my - smileyCY;
                int R = max(10, (int)round(sqrt((double)(dx * dx + dy * dy))));
                HDC hdc = GetDC(hwnd);
                if (activeAlgorithm == "SMILEY_HAPPY")
                    DrawSmileyHappy(hdc, smileyCX, smileyCY, R, currentColor);
                else
                    DrawSmileySad(hdc, smileyCX, smileyCY, R, currentColor);
                ReleaseDC(hwnd, hdc);
                Shape s;
                s.type = activeAlgorithm;
                s.color = currentColor;
                s.params = { smileyCX, smileyCY, R };
                shapes.push_back(s);
                cout << "[BONUS] Drew " << activeAlgorithm
                    << " at (" << smileyCX << "," << smileyCY
                    << ") R=" << R << "\n";
                smileyWaitingCenter = false; smileyWaitingEdge = false;
            }
            return 0;
        }

        // ════════════════════════════════════════════════════════════════
        // CLIPPING — two completely separate, flat state machines.
        //
        // FIX 2: the original had the square block nested inside the rect
        //        block, making it unreachable.  Each window type is now its
        //        own top-level if block.
        //
        // Rectangle window: 3 clicks → P1, P2, then a third y-extent point.
        // Square window   : 2 clicks → center, then a side point.
        // Circle window   : 2 clicks → center, then an edge point.
        // ════════════════════════════════════════════════════════════════

        // ── Rectangle clipping ────────────────────────────────────────
        if (activeAlgorithm == "CLIP_RECT_LINE" ||
            activeAlgorithm == "CLIP_RECT_POINT" ||
            activeAlgorithm == "CLIP_RECT_POLY")
        {
            // State 0,1,2 — build the clipping window
            if (clipState == 0)
            {
                p1x = mx; p1y = my; clipState = 1;
                cout << "[CLIP] P1 set.\n";
                return 0;
            }
            if (clipState == 1)
            {
                p2x = mx; p2y = my; clipState = 2;
                cout << "[CLIP] P2 set.\n";
                return 0;
            }
            if (clipState == 2)
            {
                // always sort so yTop < yBottom numerically
                xLeft = min(p1x, p2x);
                xRight = max(p1x, p2x);
                yTop = min(p1y, my);  // smaller y = top in screen coords
                yBottom = max(p1y, my);
                HDC hdc = GetDC(hwnd);
                DrawRectangleWindow(hdc);
                ReleaseDC(hwnd, hdc);
                clipState = 3;
                cout << "[CLIP] Rectangle window ready.\n";
                return 0;
            }

            // State 3+ — window is built, now do clipping

            // ── Rect Line clipping ────────────────────────────────────
            if (activeAlgorithm == "CLIP_RECT_LINE")
            {
                if (clipState == 3)
                {
                    x1Line = mx; y1Line = my; clipState = 4;
                    cout << "[CLIP] Line start set.\n";
                }
                else // clipState == 4
                {
                    double lx1 = x1Line, ly1 = y1Line, lx2 = mx, ly2 = my;
                    HDC hdc = GetDC(hwnd);
                    CoheSuth(hdc, lx1, ly1, lx2, ly2, xLeft, xRight, yBottom, yTop);
                    DrawRectangleWindow(hdc);
                    ReleaseDC(hwnd, hdc);
                    clipState = 3; // ready for another line
                    cout << "[CLIP] Line clipped.\n";
                }
                return 0;
            }

            // ── Rect Point clipping ───────────────────────────────────
            // Only draws the point if it is inside the clipping window.
            if (activeAlgorithm == "CLIP_RECT_POINT")
            {
                bool inside = pointclip(mx, my, xLeft, xRight, yBottom, yTop);
                HDC hdc = GetDC(hwnd);
                if (inside)
                {
                    // Draw a small visible dot at the accepted point
                    for (int dy = -1; dy <= 1; dy++)
                        for (int dx = -1; dx <= 1; dx++)
                            SetPixel(hdc, mx + dx, my + dy, RGB(0, 200, 0));
                    cout << "[CLIP] Point INSIDE — drawn.\n";
                }
                else
                    cout << "[CLIP] Point OUTSIDE — not drawn.\n";
                DrawRectangleWindow(hdc);
                ReleaseDC(hwnd, hdc);
                return 0;
            }

            // ── Rect Polygon — accumulate vertices until right-click ──
            if (activeAlgorithm == "CLIP_RECT_POLY")
            {
                polyPoints.push_back({ mx, my });
                HDC hdc = GetDC(hwnd);
                Ellipse(hdc, mx - 2, my - 2, mx + 2, my + 2);
                if (polyPoints.size() > 1)
                    LineMidpoint(hdc,
                        polyPoints[polyPoints.size() - 2].x,
                        polyPoints[polyPoints.size() - 2].y,
                        mx, my, RGB(0, 0, 255));
                ReleaseDC(hwnd, hdc);
                return 0;
            }
        }

        // ── Square clipping ───────────────────────────────────────────
        // FIX 2: completely separate block — not nested inside rect block
        if (activeAlgorithm == "CLIP_SQ_LINE" ||
            activeAlgorithm == "CLIP_SQ_POINT")
        {
            if (clipState == 0)
            {
                p1x = mx; p1y = my; clipState = 1;
                cout << "[CLIP] Square center set.\n";
                return 0;
            }
            if (clipState == 1)
            {
                int side = max(abs(mx - p1x), abs(my - p1y));
                sqLeft = p1x - side; sqRight = p1x + side;
                sqTop = p1y - side; sqBottom = p1y + side;
                HDC hdc = GetDC(hwnd);
                DrawSquareWindow(hdc);
                ReleaseDC(hwnd, hdc);
                clipState = 3;
                cout << "[CLIP] Square window ready.\n";
                return 0;
            }

            // ── Square Line clipping ──────────────────────────────────
            if (activeAlgorithm == "CLIP_SQ_LINE")
            {
                if (clipState == 3)
                {
                    x1Line = mx; y1Line = my; clipState = 4;
                    cout << "[CLIP] Line start set.\n";
                }
                else // clipState == 4
                {
                    double lx1 = x1Line, ly1 = y1Line, lx2 = mx, ly2 = my;
                    HDC hdc = GetDC(hwnd);
                    CoheSuth(hdc, lx1, ly1, lx2, ly2, sqLeft, sqRight, sqBottom, sqTop);
                    DrawSquareWindow(hdc);
                    ReleaseDC(hwnd, hdc);
                    clipState = 3;
                    cout << "[CLIP] Line clipped.\n";
                }
                return 0;
            }

            // ── Square Point clipping ─────────────────────────────────
            // Only draws the point if it is inside the square window.
            if (activeAlgorithm == "CLIP_SQ_POINT")
            {
                bool inside = pointclip(mx, my, sqLeft, sqRight, sqBottom, sqTop);
                HDC hdc = GetDC(hwnd);
                if (inside)
                {
                    for (int dy = -2; dy <= 2; dy++)
                        for (int dx = -2; dx <= 2; dx++)
                            SetPixel(hdc, mx + dx, my + dy, RGB(0, 200, 0));
                    cout << "[CLIP] Point INSIDE — drawn.\n";
                }
                else
                    cout << "[CLIP] Point OUTSIDE — not drawn.\n";
                DrawSquareWindow(hdc);
                ReleaseDC(hwnd, hdc);
                return 0;
            }
        }

        // ── Circle clipping window ─────────────────────────────────────
        // 2 clicks: center → edge to define the circular window.
        // Then for line: 2 more clicks (start, end).
        // For point   : each click tests inside/outside.
        if (activeAlgorithm == "CLIP_CIRCLE_LINE" ||
            activeAlgorithm == "CLIP_CIRCLE_POINT")
        {
            if (clipState == 0)
            {
                // First click: circle center
                clipCircleCX = mx; clipCircleCY = my; clipState = 1;
                cout << "[CLIP] Circle center set. Click edge to set radius.\n";
                return 0;
            }
            if (clipState == 1)
            {
                // Second click: edge point → compute radius
                double dx = mx - clipCircleCX, dy = my - clipCircleCY;
                clipCircleR = sqrt(dx * dx + dy * dy);
                HDC hdc = GetDC(hwnd);
                DrawCircleClipWindow(hdc);
                ReleaseDC(hwnd, hdc);
                clipState = 3;
                cout << "[CLIP] Circle window ready (R=" << (int)clipCircleR << ").\n";
                return 0;
            }

            // ── Circle Line clipping ──────────────────────────────────
            if (activeAlgorithm == "CLIP_CIRCLE_LINE")
            {
                if (clipState == 3)
                {
                    x1Line = mx; y1Line = my; clipState = 4;
                    cout << "[CLIP] Line start set.\n";
                }
                else // clipState == 4
                {
                    double ox1, oy1, ox2, oy2;
                    HDC hdc = GetDC(hwnd);
                    if (ClipLineToCircle(x1Line, y1Line, mx, my, ox1, oy1, ox2, oy2))
                        LineMidpoint(hdc, (int)round(ox1), (int)round(oy1),
                            (int)round(ox2), (int)round(oy2), RGB(255, 0, 0));
                    DrawCircleClipWindow(hdc);
                    ReleaseDC(hwnd, hdc);
                    clipState = 3; // ready for next line
                    cout << "[CLIP] Line clipped to circle.\n";
                }
                return 0;
            }

            // ── Circle Point clipping ─────────────────────────────────
            // Only draws the point if it is inside the circular window.
            if (activeAlgorithm == "CLIP_CIRCLE_POINT")
            {
                bool inside = PointInsideCircleWindow(mx, my);
                HDC hdc = GetDC(hwnd);
                if (inside)
                {
                    for (int dy = -2; dy <= 2; dy++)
                        for (int dx = -2; dx <= 2; dx++)
                            SetPixel(hdc, mx + dx, my + dy, RGB(0, 200, 0));
                    cout << "[CLIP] Point INSIDE circle — drawn.\n";
                }
                else
                    cout << "[CLIP] Point OUTSIDE circle — not drawn.\n";
                DrawCircleClipWindow(hdc);
                ReleaseDC(hwnd, hdc);
                return 0;
            }
        }

        return 0;
    }

    // ── WM_RBUTTONDOWN: fires on every right mouse click ─────────────
    case WM_RBUTTONDOWN:
    {
        // ── Right-click finalises Cardinal Spline ──────────────────────
        if (activeAlgorithm == "CURVE_CARDINAL" && curveCollecting)
        {
            int n = (int)curvePoints.size();
            if (n < 4)
                cout << "[CURVE] Need at least 4 points (have " << n << "). Keep clicking.\n";
            else
            {
                HDC hdc = GetDC(hwnd);
                DrawCardinalSpline(hdc, curvePoints.data(), n, curveTension, currentColor);
                ReleaseDC(hwnd, hdc);

                // Save: params = [tension*1000, n, x0,y0, x1,y1, ...]
                Shape s;
                s.type = "CURVE_CARDINAL";
                s.color = currentColor;
                s.params.push_back((int)(curveTension * 1000));
                s.params.push_back(n);
                for (int i = 0; i < n; i++)
                {
                    s.params.push_back(curvePoints[i].x);
                    s.params.push_back(curvePoints[i].y);
                }
                shapes.push_back(s);
                cout << "[CURVE] Cardinal Spline drawn through "
                    << (n - 2) << " points (tension=" << curveTension << ").\n";
                curveCollecting = false; curvePoints.clear();
            }
        }
        // ── Right-click finalises Convex / Non-Convex polygon fill ─────
        if ((activeAlgorithm == "FILL_CONVEX" || activeAlgorithm == "FILL_NONCONVEX")
            && fillPolyCollecting && (int)fillPolyPoints.size() >= 3)
        {
            HDC hdc = GetDC(hwnd);
            // Close the polygon outline
            int n = (int)fillPolyPoints.size();
            LineMidpoint(hdc,
                fillPolyPoints[n - 1].x, fillPolyPoints[n - 1].y,
                fillPolyPoints[0].x, fillPolyPoints[0].y,
                RGB(0, 0, 255));
            // Fill
            if (activeAlgorithm == "FILL_CONVEX")
                ConvexFill(hdc, fillPolyPoints, currentColor);
            else
                NonConvexFill(hdc, fillPolyPoints, currentColor);
            ReleaseDC(hwnd, hdc);

            // Save shape — params: [n, x0,y0, x1,y1, ...]
            Shape s;
            s.type = activeAlgorithm;
            s.color = currentColor;
            s.params.push_back(n);
            for (auto& fp : fillPolyPoints)
            {
                s.params.push_back(fp.x);
                s.params.push_back(fp.y);
            }
            shapes.push_back(s);
            cout << "[FILL] " << activeAlgorithm << " polygon filled ("
                << n << " vertices).\n";
            fillPolyPoints.clear();
            fillPolyCollecting = false;
        }
        // ── Right-click finalises Polygon Clipping ─────────────────────
        if (activeAlgorithm == "CLIP_RECT_POLY" && (int)polyPoints.size() >= 3)
        {
            HDC hdc = GetDC(hwnd);
            // Close the polygon visually before clipping
            LineMidpoint(hdc,
                polyPoints.back().x, polyPoints.back().y,
                polyPoints[0].x, polyPoints[0].y,
                RGB(0, 0, 255));
            // polygonclip draws only the inside portion in currentColor;
            // the outside is simply not drawn (deleted, not colored red).
            polygonclip(hdc, polyPoints.data(), (int)polyPoints.size(),
                xLeft, xRight, yBottom, yTop);
            DrawRectangleWindow(hdc);
            ReleaseDC(hwnd, hdc);
            cout << "[CLIP] Polygon clipped — inside portion drawn, outside removed.\n";
            polyPoints.clear();
        }
        return 0;
    }

    // ── WM_PAINT: fires when window needs repainting ──────────────────
    // Triggered by: window resize, minimize+restore, ClearScreen(),
    // any InvalidateRect() call.
    // BeginPaint/EndPaint are required to properly handle the paint cycle.
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RedrawShapes(hdc); // redraws everything from the shapes vector
        EndPaint(hwnd, &ps);
        return 0;
    }

    // ── WM_DESTROY: fires when the window X button is clicked ─────────
    // PostQuitMessage(0) puts a WM_QUIT into the message queue,
    // which causes GetMessage() in WinMain to return 0, ending the loop.
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
