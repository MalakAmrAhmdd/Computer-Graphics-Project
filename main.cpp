#define UNICODE
#include <windows.h>
#include <cmath>
#include <stack>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>

using namespace std;

// ════════════════════════════════════════════════════════════════════════════
// MENU IDs — every clickable menu item needs a unique integer ID.
// When the user clicks a menu item, WndProc receives WM_COMMAND with that ID
// in LOWORD(wParam). Add your own IDs here following the same pattern.
// ════════════════════════════════════════════════════════════════════════════
#define ID_FILE_CLEAR 1001 // File → Clear Screen
#define ID_FILE_SAVE 1002  // File → Save
#define ID_FILE_LOAD 1003  // File → Load

#define ID_PREF_WHITEBG 2001 // Preferences → White Background
#define ID_PREF_CURSOR 2002  // Preferences → Change Cursor
#define ID_PREF_COLOR 2003   // Preferences → Choose Color

#define ID_LINE_DDA 3001
#define ID_LINE_MIDPOINT 3002
#define ID_LINE_PARAMETRIC 3003

#define ID_CIRCLE_DIRECT 4001
#define ID_CIRCLE_POLAR 4002
#define ID_CIRCLE_ITER_POLAR 4003
#define ID_CIRCLE_MIDPOINT 4004
#define ID_CIRCLE_MOD_MIDPOINT 4005

#define ID_ELLIPSE_DIRECT 5001
#define ID_ELLIPSE_POLAR 5002
#define ID_ELLIPSE_MID 5003

#define ID_CURVE_CARDINAL 6001

#define ID_FILL_CIRCLE_LINES 7001
#define ID_FILL_CIRCLE_CIRCLES 7002
#define ID_FILL_SQUARE_HERMIT 7003
#define ID_FILL_RECT_BEZIER 7004
#define ID_FILL_CONVEX 7005
#define ID_FILL_NONCONVEX 7006
#define ID_FILL_FLOOD_REC 7007
#define ID_FILL_FLOOD_NONREC 7008

#define ID_CLIP_RECT_POINT 8001
#define ID_CLIP_RECT_LINE 8002
#define ID_CLIP_RECT_POLY 8003
#define ID_CLIP_SQ_POINT 8004
#define ID_CLIP_SQ_LINE 8005

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
    COLORREF color;
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
                                      // all members use this when drawing

bool useCustomCursor = false; // toggles between arrow and crosshair cursor
bool whiteBg = false;         // toggles between gray and white background
HBRUSH bgBrush = NULL;        // handle to the current background brush

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
int x1Line = 0, y1Line = 0;

// ── 2: Add your click-state variables here ────────────────────────
// Example:
// bool circleWaitingForRadius = false;
// int circleCX = 0, circleCY = 0;

// ── 3: Add ellipse/curve click-state variables here ───────────────

// ── 4: Add filling click-state variables here ─────────────────────

// ── 5: Add clipping click-state variables here ────────────────────

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

// ── 2: Declare your circle functions here ─────────────────────────
// Example:
// void CircleDirect(HDC hdc, int cx, int cy, int r, COLORREF c);
// void CirclePolar(HDC hdc, int cx, int cy, int r, COLORREF c);
// void CircleIterPolar(HDC hdc, int cx, int cy, int r, COLORREF c);
// void CircleMidpoint(HDC hdc, int cx, int cy, int r, COLORREF c);
// void CircleModMidpoint(HDC hdc, int cx, int cy, int r, COLORREF c);

// ── 3: Declare ellipse + curve functions here ─────────────────────

// ── 4: Declare filling functions here ─────────────────────────────

// ── 5: Declare clipping functions here ─────────────────────────────

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
    freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE **)stdin, "CONIN$", "r", stdin);
    cout << "[INFO] 2D Drawing Package started.\n";
    cout << "[INFO] Pick a tool from the menu, then click two points.\n";

    bgBrush = (HBRUSH)GetStockObject(LTGRAY_BRUSH);

    // WNDCLASS — describes our window to Windows OS before creating it.
    // Think of it as a blueprint/class definition for the window.
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW; // repaint when resized
    wc.lpfnWndProc = WndProc;           // which function handles window events
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush;    // background color of the window
    wc.lpszClassName = L"MyClass"; // internal name used by CreateWindow
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
    return msg.wParam;
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

    // ── 2: Replace this placeholder with your real circle menu ────
    // HMENU circleMenu = CreatePopupMenu();
    // AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_DIRECT,       L"Direct");
    // AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_POLAR,        L"Polar");
    // AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_ITER_POLAR,   L"Iterative Polar");
    // AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_MIDPOINT,     L"Midpoint");
    // AppendMenu(circleMenu, MF_STRING, ID_CIRCLE_MOD_MIDPOINT, L"Modified Midpoint");
    // AppendMenu(menuBar, MF_POPUP, (UINT_PTR)circleMenu, L"Circles");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)CreatePopupMenu(), L"Circles"); // remove this line when done

    // ── 3: Replace placeholders with real Ellipse + Curves menus ──
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)CreatePopupMenu(), L"Ellipse"); // replace when done
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)CreatePopupMenu(), L"Curves");  // replace when done

    // ── 4: Replace placeholder with real Filling menu ─────────────
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)CreatePopupMenu(), L"Filling"); // replace when done

    // ── 5: Replace placeholder with real Clipping menu ────────────
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)CreatePopupMenu(), L"Clipping"); // replace when done

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
    cout << "[FILE] Screen cleared.\n";
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

        for (auto& s : shapes) {
            file << s.type << " " << (int)s.color << " " << s.params.size();
            for (int p : s.params) file << " " << p;
            file << "\n";
        }
        file.close();
        cout << "[FILE] Saved " << shapes.size() << " shapes to " << filename << "\n"; })
        .detach();
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
               if (!file.is_open())
               {
                   cout << "[ERROR] Cannot open file.\n";
                   return;
               }

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
                   for (int i = 0; i < count; i++)
                       file >> s.params[i];
                   shapes.push_back(s);
               }
               file.close();
               cout << "[FILE] Loaded " << shapes.size() << " shapes from " << filename << "\n";
               InvalidateRect(hwnd, NULL, FALSE); // triggers WM_PAINT → RedrawShapes
           })
        .detach();
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
    for (auto &s : shapes)
    {
        // ── Lines (1) ─────────────────────────────────────────────
        if (s.params.size() >= 4)
        {
            if (s.type == "LINE_DDA")
                LineDDA(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
            else if (s.type == "LINE_MIDPOINT")
                LineMidpoint(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
            else if (s.type == "LINE_PARAMETRIC")
                LineParametric(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        }

        // ── 2: Add circle redraw cases here ───────────────────────
        // else if (s.type == "CIRCLE_DIRECT"       && s.params.size() >= 3) CircleDirect    (hdc, s.params[0], s.params[1], s.params[2], s.color);
        // else if (s.type == "CIRCLE_POLAR"        && s.params.size() >= 3) CirclePolar     (hdc, s.params[0], s.params[1], s.params[2], s.color);
        // else if (s.type == "CIRCLE_ITER_POLAR"   && s.params.size() >= 3) CircleIterPolar (hdc, s.params[0], s.params[1], s.params[2], s.color);
        // else if (s.type == "CIRCLE_MIDPOINT"     && s.params.size() >= 3) CircleMidpoint  (hdc, s.params[0], s.params[1], s.params[2], s.color);
        // else if (s.type == "CIRCLE_MOD_MIDPOINT" && s.params.size() >= 3) CircleModMid    (hdc, s.params[0], s.params[1], s.params[2], s.color);

        // ── 3: Add ellipse + curve redraw cases here ──────────────
        // else if (s.type == "ELLIPSE_DIRECT" && s.params.size() >= 4) EllipseDirect(hdc, s.params[0], s.params[1], s.params[2], s.params[3], s.color);
        // else if (s.type == "CURVE_CARDINAL" && s.params.size() >= 2) CardinalSpline(hdc, s.params, s.color);

        // ── 4: Add filling redraw cases here ───────────────────────
        // (filling shapes are usually circles/rects drawn first, then filled)

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
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    float xInc = (float)dx / steps; // how much to move in x each step
    float yInc = (float)dy / steps; // how much to move in y each step

    float x = x1, y = y1;
    for (int i = 0; i <= steps; i++)
    {
        SetPixel(hdc, (int)round(x), (int)round(y), c);
        x += xInc;
        y += yInc;
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
    {                           // more horizontal: step along x
        int d = 2 * dy - dx;    // initial decision variable
        int d1 = 2 * (dy - dx); // increment when d > 0 (diagonal step)
        int d2 = 2 * dy;        // increment when d <= 0 (horizontal step)
        for (int i = 0; i <= dx; i++)
        {
            SetPixel(hdc, x, y, c);
            if (d > 0)
            {
                y += sy;
                d += d1;
            }
            else
                d += d2;
            x += sx;
        }
    }
    else
    { // more vertical: step along y
        int d = 2 * dx - dy;
        int d1 = 2 * (dx - dy);
        int d2 = 2 * dx;
        for (int i = 0; i <= dy; i++)
        {
            SetPixel(hdc, x, y, c);
            if (d > 0)
            {
                x += sx;
                d += d1;
            }
            else
                d += d2;
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
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    for (int i = 0; i <= steps; i++)
    {
        float t = (steps == 0) ? 0 : (float)i / steps; // t from 0 to 1
        int x = (int)round(x1 + t * dx);
        int y = (int)round(y1 + t * dy);
        SetPixel(hdc, x, y, c);
    }
}

// DrawLine — dispatcher that calls the correct algorithm based on activeAlgorithm.
// This is what WM_LBUTTONDOWN calls after getting both click points.
void DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    if (activeAlgorithm == "DDA")
        LineDDA(hdc, x1, y1, x2, y2, c);
    else if (activeAlgorithm == "MIDPOINT")
        LineMidpoint(hdc, x1, y1, x2, y2, c);
    else if (activeAlgorithm == "PARAMETRIC")
        LineParametric(hdc, x1, y1, x2, y2, c);
}

// ── 2: Add your circle functions below DrawLine ───────────────────
// Pattern to follow:
// void CirclePolar(HDC hdc, int cx, int cy, int r, COLORREF c) { ... }

// ── 3: Add ellipse + cardinal spline functions here ───────────────

// ── 4: Add all filling functions here ─────────────────────────────

// ── 5: Add all clipping functions here ─────────────────────────────

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
        case ID_FILE_CLEAR:
            ClearScreen(hwnd);
            break;
        case ID_FILE_SAVE:
            SaveToFile(hwnd);
            break;
        case ID_FILE_LOAD:
            LoadFromFile(hwnd);
            break;

        // ── Preferences ───────────────────────────────────────────
        case ID_PREF_WHITEBG:
        {
            // Toggle background between white and light gray.
            // DeleteObject frees the old brush before creating a new one.
            // SetClassLongPtr updates the window class brush immediately.
            // InvalidateRect forces a repaint so the new color shows.
            whiteBg = !whiteBg;
            if (bgBrush)
                DeleteObject(bgBrush);
            bgBrush = CreateSolidBrush(whiteBg ? RGB(255, 255, 255) : RGB(211, 211, 211));
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);
            InvalidateRect(hwnd, NULL, TRUE);
            cout << "[PREF] Background: " << (whiteBg ? "white" : "gray") << "\n";
            break;
        }
        case ID_PREF_CURSOR:
        {
            // Toggle between default arrow and crosshair cursor.
            // IDC_CROSS is the + shaped cursor, good for precision drawing.
            useCustomCursor = !useCustomCursor;
            HCURSOR cur = LoadCursor(NULL, useCustomCursor ? IDC_CROSS : IDC_ARROW);
            SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR)cur);
            cout << "[PREF] Cursor: " << (useCustomCursor ? "crosshair" : "arrow") << "\n";
            break;
        }
        case ID_PREF_COLOR:
        {
            // Opens the Windows built-in color picker dialog.
            // ChooseColor() returns TRUE if user picked a color and clicked OK.
            // The chosen color is stored in cc.rgbResult → currentColor.
            // All drawing functions use currentColor when plotting pixels.
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
            activeAlgorithm = "DDA";
            waitingForSecondClick = false;
            cout << "[LINE] DDA selected. Click start point.\n";
            break;
        case ID_LINE_MIDPOINT:
            activeAlgorithm = "MIDPOINT";
            waitingForSecondClick = false;
            cout << "[LINE] Midpoint selected. Click start point.\n";
            break;
        case ID_LINE_PARAMETRIC:
            activeAlgorithm = "PARAMETRIC";
            waitingForSecondClick = false;
            cout << "[LINE] Parametric selected. Click start point.\n";
            break;

            // ── 2: Add your circle cases here ─────────────────
            // Pattern:
            // case ID_CIRCLE_POLAR:
            //     activeAlgorithm = "CIRCLE_POLAR";
            //     circleWaitingForRadius = false;
            //     cout << "[CIRCLE] Polar selected. Click center.\n";
            //     break;

            // ── 3: Add ellipse + curve cases here ─────────────

            // ── 4: Add filling cases here ─────────────────────

            // ── 5: Add clipping cases here ────────────────────
        }
        return 0;
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
                x1Line = mx;
                y1Line = my;
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
                s.params = {x1Line, y1Line, mx, my};
                shapes.push_back(s);

                cout << "[LINE] Drew " << activeAlgorithm
                     << " from (" << x1Line << "," << y1Line
                     << ") to (" << mx << "," << my << ")\n";
                waitingForSecondClick = false;
            }
            return 0;
        }

        // ── 2: Add circle click handling here ──────────────────
        // Circles need 2 clicks: center then a point on the rim.
        // Calculate radius = distance between the two clicks.
        // Pattern:
        // if (activeAlgorithm == "CIRCLE_POLAR" || ...) {
        //     if (!circleWaitingForRadius) {
        //         circleCX = mx; circleCY = my;
        //         circleWaitingForRadius = true;
        //         cout << "[CIRCLE] Center set. Click edge point.\n";
        //     } else {
        //         int r = (int)sqrt(pow(mx-circleCX,2) + pow(my-circleCY,2));
        //         HDC hdc = GetDC(hwnd);
        //         CirclePolar(hdc, circleCX, circleCY, r, currentColor);
        //         ReleaseDC(hwnd, hdc);
        //         Shape s;
        //         s.type = "CIRCLE_POLAR";
        //         s.color = currentColor;
        //         s.params = { circleCX, circleCY, r };
        //         shapes.push_back(s);
        //         circleWaitingForRadius = false;
        //     }
        //     return 0;
        // }

        // ── 3: Add ellipse click handling here ─────────────────
        // Ellipses need 2 clicks: center, then corner of bounding box.
        // rx = abs(mx - cx),  ry = abs(my - cy)

        // ── 4: Add filling click handling here ─────────────────
        // Most filling needs: draw shape first, then click inside to fill.
        // Use a separate activeAlgorithm string like "FILL_FLOOD_REC"

        // ── 5: Add clipping click handling here ────────────────
        // Rectangle clipping window: 2 clicks for top-left and bottom-right.

        break;
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