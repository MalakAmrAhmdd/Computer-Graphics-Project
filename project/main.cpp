#define UNICODE
#define _USE_MATH_DEFINES
#include "headers/globals.h"
#include "headers/shapes.h"
#include "headers/lines.h"
#include "headers/circles.h"
#include "headers/ellipse.h"
#include "headers/curves.h"
#include "headers/filling.h"
#include "headers/clipping.h"
#include "headers/smiley_face.h"
#include <cmath>
#include <stack>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>

using namespace std;

#define ID_FILE_CLEAR 1001 // File → Clear Screen
#define ID_FILE_SAVE 1002  // File → Save
#define ID_FILE_LOAD 1003  // File → Load

#define ID_PREF_WHITEBG 2001 // Preferences: White Background
#define ID_PREF_CURSOR 2002  // Preferences: Change Cursor
#define ID_PREF_COLOR 2003   // Preferences: Choose Color

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

#define ID_CLIP_CIRCLE_LINE 8006
#define ID_CLIP_CIRCLE_POINT 8007

// Bonus: Smiley - Sad faces
#define ID_BONUS_HAPPY 9001
#define ID_BONUS_SAD 9002

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HMENU CreateAppMenu();
void ClearScreen(HWND hwnd);
void SaveToFile(HWND hwnd);
void LoadFromFile(HWND hwnd);
void RedrawShapes(HDC hdc);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
    // Open a console window alongside the GUI window.
    // Without this, cout output goes nowhere in a Win32 app.
    // freopen_s redirects stdout and stdin to the console window.
    FreeConsole();
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
    clipState = 0;
    polyPoints.clear();
    circleClipWaitingRadius = false;
    fillPolyPoints.clear();
    fillPolyCollecting = false;

    // Clear the console window so the log is fresh for the next test
    system("cls");
    cout << "[INFO] 2D Drawing Package started.\n";
    cout << "[INFO] Pick a tool from the menu, then click two points.\n";
    cout << "[INFO] Canvas and console cleared ready for next test.\n";
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

            // ── Circles ──────────────────────────────────────────────
        case ID_CIRCLE_DIRECT:
            activeAlgorithm = "DIRECT";
            circleWaitingForRadius = false;
            cout << "[CIRCLE] Direct selected. Click center.\n";
            break;
        case ID_CIRCLE_POLAR:
            activeAlgorithm = "POLAR";
            circleWaitingForRadius = false;
            cout << "[CIRCLE] Polar selected. Click center.\n";
            break;
        case ID_CIRCLE_ITER_POLAR:
            activeAlgorithm = "Iterative_POLAR";
            circleWaitingForRadius = false;
            cout << "[CIRCLE] Iterative Polar selected. Click center.\n";
            break;
        case ID_CIRCLE_MIDPOINT:
            activeAlgorithm = "MID";
            circleWaitingForRadius = false;
            cout << "[CIRCLE] Midpoint selected. Click center.\n";
            break;
        case ID_CIRCLE_MOD_MIDPOINT:
            activeAlgorithm = "Modified_Midpoint";
            circleWaitingForRadius = false;
            cout << "[CIRCLE] Modified Midpoint selected. Click center.\n";
            break;

            // ── Ellipses ─────────────────────────────────────────────
        case ID_ELLIPSE_DIRECT:
            activeAlgorithm = "Ellipse_Direct";
            ellipseWaiting = false;
            cout << "[ELLIPSE] Direct selected. Click center.\n";
            break;
        case ID_ELLIPSE_MID:
            activeAlgorithm = "Ellipse_Midpoint";
            ellipseWaiting = false;
            cout << "[ELLIPSE] Midpoint selected. Click center.\n";
            break;
        case ID_ELLIPSE_POLAR:
            activeAlgorithm = "Ellipse_Polar";
            ellipseWaiting = false;
            cout << "[ELLIPSE] Polar selected. Click center.\n";
            break;

            // ── Cardinal Spline ───────────────────────────────────────
        case ID_CURVE_CARDINAL:
            activeAlgorithm = "CURVE_CARDINAL";
            curveCollecting = false;
            curvePoints.clear();
            // Ask tension in a background thread so the GUI stays live
            thread([]()
                   {
                        cout << "[CURVE] Cardinal Spline selected.\n";
                        cout << "[CURVE] Enter tension (0.0 = smooth, 1.0 = straight): ";
                        cin >> curveTension;
                        if (curveTension < 0) curveTension = 0;
                        if (curveTension > 1) curveTension = 1;
                        curveCollecting = true;
                        cout << "[CURVE] Click at least 4 points on the canvas.\n";
                        cout << "[CURVE] Right-click when done.\n"; })
                .detach();
            break;

            // ── Circle fill ───────────────────────────────────────────
        case ID_FILL_CIRCLE_LINES:
            activeAlgorithm = "FILL_CIRCLE_LINES";
            fillWaitingCenter = false;
            fillWaitingEdge = false;
            thread([]()
                   {
                        cout << "[FILL] Fill Circle with Lines.\n";
                        cout << "[FILL] Enter quarter (1=top-right, 2=top-left, "
                                "3=bottom-left, 4=bottom-right): ";
                        cin >> fillQuarter;
                        if (fillQuarter < 1 || fillQuarter > 4) fillQuarter = 1;
                        fillWaitingCenter = true;
                        cout << "[FILL] Click circle center.\n"; })
                .detach();
            break;
        case ID_FILL_CIRCLE_CIRCLES:
            activeAlgorithm = "FILL_CIRCLE_CIRCLES";
            fillWaitingCenter = false;
            fillWaitingEdge = false;
            thread([]()
                   {
                        cout << "[FILL] Fill Circle with Circles.\n";
                        cout << "[FILL] Enter quarter (1=top-right, 2=top-left, "
                                "3=bottom-left, 4=bottom-right): ";
                        cin >> fillQuarter;
                        if (fillQuarter < 1 || fillQuarter > 4) fillQuarter = 1;
                        fillWaitingCenter = true;
                        cout << "[FILL] Click circle center.\n"; })
                .detach();
            break;

            // ── Hermite / Bezier fill ─────────────────────────────────
        case ID_FILL_SQUARE_HERMIT:
            activeAlgorithm = "FILL_SQUARE_HERMIT";
            hermiteWaitingSecond = false;
            cout << "[FILL] Square with Hermite selected. Click first corner.\n";
            break;
        case ID_FILL_RECT_BEZIER:
            activeAlgorithm = "FILL_RECT_BEZIER";
            bezierRectWaitingSecond = false;
            cout << "[FILL] Rectangle with Bezier selected. Click first corner.\n";
            break;

            // ── Convex / Non-Convex polygon fill ─────────────────────────────
        case ID_FILL_CONVEX:
            activeAlgorithm = "FILL_CONVEX";
            fillPolyPoints.clear();
            fillPolyCollecting = true;
            cout << "[FILL] Convex Polygon Fill selected.\n";
            cout << "[FILL] Left-click to add vertices. Right-click to fill.\n";
            break;
        case ID_FILL_NONCONVEX:
            activeAlgorithm = "FILL_NONCONVEX";
            fillPolyPoints.clear();
            fillPolyCollecting = true;
            cout << "[FILL] Non-Convex Polygon Fill selected.\n";
            cout << "[FILL] Left-click to add vertices. Right-click to fill.\n";
            break;

            // ── Flood fill ────────────────────────────────────────────
        case ID_FILL_FLOOD_REC:
            activeAlgorithm = "recursive";
            cout << "[FLOOD] Recursive Flood Fill — click 4 points for polygon.\n";
            break;
        case ID_FILL_FLOOD_NONREC:
            activeAlgorithm = "non_recursive";
            cout << "[FLOOD] Non-Recursive Flood Fill — click 4 points for polygon.\n";
            break;

            // ── Smiley faces ──────────────────────────────────────────
        case ID_BONUS_HAPPY:
            activeAlgorithm = "SMILEY_HAPPY";
            smileyWaitingCenter = true;
            smileyWaitingEdge = false;
            cout << "[BONUS] Happy Smiley selected. Click face center.\n";
            break;
        case ID_BONUS_SAD:
            activeAlgorithm = "SMILEY_SAD";
            smileyWaitingCenter = true;
            smileyWaitingEdge = false;
            cout << "[BONUS] Sad Smiley selected. Click face center.\n";
            break;

            // ── Clipping ─────────────────────────────────────────────

        case ID_CLIP_RECT_LINE:
            activeAlgorithm = "CLIP_RECT_LINE";
            clipState = 0;
            polyPoints.clear();
            cout << "[CLIP] Rect Line : click P1, P2, third point for height, "
                    "then line start + end.\n";
            break;
        case ID_CLIP_RECT_POINT:
            activeAlgorithm = "CLIP_RECT_POINT";
            clipState = 0;
            polyPoints.clear();
            cout << "[CLIP] Rect Point : click P1, P2, third point for height, "
                    "then click points to test.\n";
            break;
        case ID_CLIP_RECT_POLY:
            activeAlgorithm = "CLIP_RECT_POLY";
            clipState = 0;
            polyPoints.clear();
            cout << "[CLIP] Rect Poly : click P1, P2, third point for height, "
                    "then click polygon vertices. Right-click to clip.\n";
            break;
        case ID_CLIP_SQ_LINE:
            activeAlgorithm = "CLIP_SQ_LINE";
            clipState = 0;
            polyPoints.clear();
            cout << "[CLIP] Square Line : click center, then side point, "
                    "then line start + end.\n";
            break;
        case ID_CLIP_SQ_POINT:
            activeAlgorithm = "CLIP_SQ_POINT";
            clipState = 0;
            polyPoints.clear();
            cout << "[CLIP] Square Point : click center, then side point, "
                    "then click points to test.\n";
            break;
        case ID_CLIP_CIRCLE_LINE:
            activeAlgorithm = "CLIP_CIRCLE_LINE";
            clipState = 0;
            circleClipWaitingRadius = false;
            cout << "[CLIP] Circle Line : click circle center, then edge point "
                    "to define window, then line start + end.\n";
            break;
        case ID_CLIP_CIRCLE_POINT:
            activeAlgorithm = "CLIP_CIRCLE_POINT";
            clipState = 0;
            circleClipWaitingRadius = false;
            cout << "[CLIP] Circle Point : click circle center, then edge point "
                    "to define window, then click points to test.\n";
            break;
        }
        break;
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

        // ── Circles ───────────────────────────────────────────────────
        if (activeAlgorithm == "POLAR" ||
            activeAlgorithm == "Iterative_POLAR" ||
            activeAlgorithm == "DIRECT" ||
            activeAlgorithm == "MID" ||
            activeAlgorithm == "Modified_Midpoint")
        {
            if (!circleWaitingForRadius)
            {
                circleCX = mx;
                circleCY = my;
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

                if (activeAlgorithm == "DIRECT")
                    s.type = "CIRCLE_DIRECT";
                else if (activeAlgorithm == "POLAR")
                    s.type = "CIRCLE_POLAR";
                else if (activeAlgorithm == "Iterative_POLAR")
                    s.type = "CIRCLE_ITER_POLAR";
                else if (activeAlgorithm == "MID")
                    s.type = "CIRCLE_MIDPOINT";
                else
                    s.type = "CIRCLE_MOD_MIDPOINT";
                s.color = currentColor;
                s.params = {circleCX, circleCY, r};
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
                ellipseCX = mx;
                ellipseCY = my;
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

                if (activeAlgorithm == "Ellipse_Direct")
                    s.type = "ELLIPSE_DIRECT";
                else if (activeAlgorithm == "Ellipse_Midpoint")
                    s.type = "ELLIPSE_MIDPOINT";
                else
                    s.type = "ELLIPSE_POLAR";
                s.color = currentColor;
                s.params = {ellipseCX, ellipseCY, rx, ry};
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
            POINT pt = {mx, my};
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
             activeAlgorithm == "FILL_CIRCLE_CIRCLES") &&
            fillWaitingCenter)
        {
            if (!fillWaitingEdge)
            {
                fillCX = mx;
                fillCY = my;
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
                s.params = {fillCX, fillCY, R, fillQuarter};
                shapes.push_back(s);
                cout << "[FILL] Drew " << activeAlgorithm
                     << " center=(" << fillCX << "," << fillCY
                     << ") R=" << R << " Q=" << fillQuarter << "\n";
                fillWaitingCenter = false;
                fillWaitingEdge = false;
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
                hermiteX1 = mx;
                hermiteY1 = my;
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
                s.params = {hermiteX1, hermiteY1, mx, my};
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
                bezierX1 = mx;
                bezierY1 = my;
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
                s.params = {bezierX1, bezierY1, mx, my};
                shapes.push_back(s);
                cout << "[FILL] Bezier rectangle drawn.\n";
                bezierRectWaitingSecond = false;
            }
            return 0;
        }
        // ── Convex / Non-Convex polygon fill click collection ──────────
        if ((activeAlgorithm == "FILL_CONVEX" || activeAlgorithm == "FILL_NONCONVEX") && fillPolyCollecting)
        {
            fillPolyPoints.push_back({mx, my});
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
                pts[pointCount++] = {mx, my};
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
                    ReleaseDC(hwnd, hdc);

                    Shape s;
                    s.type = (activeAlgorithm == "recursive") ? "FLOOD_RECURSIVE" : "FLOOD_NONRECURSIVE";
                    s.color = currentColor;
                    // params: [4 polygon vertices as x0,y0,x1,y1,..., then seed x, seed y]
                    for (int i = 0; i < 4; i++)
                    {
                        s.params.push_back(pts[i].x);
                        s.params.push_back(pts[i].y);
                    }
                    s.params.push_back(mx); // seed x
                    s.params.push_back(my); // seed y
                    shapes.push_back(s);

                    polygonDrawn = false;
                    pointCount = 0;
                    activeAlgorithm = "";
                }
                else
                    MessageBox(hwnd, L"Click INSIDE the polygon!", L"Outside Polygon",
                               MB_OK | MB_ICONWARNING);
            }
            ReleaseDC(hwnd, hdc);
            return 0;
        }

        // ── Smiley face click handling ─────────────────────────────────
        if ((activeAlgorithm == "SMILEY_HAPPY" || activeAlgorithm == "SMILEY_SAD") && smileyWaitingCenter)
        {
            if (!smileyWaitingEdge)
            {
                smileyCX = mx;
                smileyCY = my;
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
                s.params = {smileyCX, smileyCY, R};
                shapes.push_back(s);
                cout << "[BONUS] Drew " << activeAlgorithm
                     << " at (" << smileyCX << "," << smileyCY
                     << ") R=" << R << "\n";
                smileyWaitingCenter = false;
                smileyWaitingEdge = false;
            }
            return 0;
        }

        // ════════════════════════════════════════════════════════════════
        // CLIPPING — two completely separate, flat state machines
        // Rectangle window: 3 clicks → P1, P2, then a third y-extent point
        // Square window   : 2 clicks → center, then a side point
        // Circle window   : 2 clicks → center, then an edge point
        // ════════════════════════════════════════════════════════════════

        // ── Rectangle clipping ────────────────────────────────────────
        if (activeAlgorithm == "CLIP_RECT_LINE" ||
            activeAlgorithm == "CLIP_RECT_POINT" ||
            activeAlgorithm == "CLIP_RECT_POLY")
        {
            // State 0,1,2 — build the clipping window
            if (clipState == 0)
            {
                p1x = mx;
                p1y = my;
                clipState = 1;
                cout << "[CLIP] P1 set.\n";
                return 0;
            }
            if (clipState == 1)
            {
                p2x = mx;
                p2y = my;
                clipState = 2;
                cout << "[CLIP] P2 set.\n";
                return 0;
            }
            if (clipState == 2)
            {
                // always sort so yTop < yBottom numerically
                xLeft = min(p1x, p2x);
                xRight = max(p1x, p2x);
                yTop = min(p1y, my); // smaller y = top in screen coords
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
                    x1Line = mx;
                    y1Line = my;
                    clipState = 4;
                    cout << "[CLIP] Line start set.\n";
                }
                else // clipState == 4
                {
                    double lx1 = x1Line, ly1 = y1Line, lx2 = mx, ly2 = my;
                    HDC hdc = GetDC(hwnd);
                    CoheSuth(hdc, lx1, ly1, lx2, ly2, xLeft, xRight, yBottom, yTop);
                    DrawRectangleWindow(hdc);
                    ReleaseDC(hwnd, hdc);
                    Shape s;
                    s.type = "CLIP_RECT_LINE";
                    s.color = currentColor;
                    s.params = {(int)xLeft, (int)yTop, (int)xRight, (int)yBottom,
                                (int)x1Line, (int)y1Line, mx, my};
                    shapes.push_back(s);
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

                    HBRUSH br = CreateSolidBrush(currentColor);
                    HBRUSH old = (HBRUSH)SelectObject(hdc, br);
                    Ellipse(hdc, mx - 3, my - 3, mx + 3, my + 3);
                    SelectObject(hdc, old);
                    DeleteObject(br);

                    Shape s;
                    s.type = "CLIP_RECT_POINT";
                    s.color = currentColor;
                    s.params = {(int)xLeft, (int)yTop, (int)xRight, (int)yBottom, mx, my};
                    shapes.push_back(s);
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
                polyPoints.push_back({mx, my});
                HDC hdc = GetDC(hwnd);
                Ellipse(hdc, mx - 1, my - 1, mx + 1, my + 1);
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

        if (activeAlgorithm == "CLIP_SQ_LINE" ||
            activeAlgorithm == "CLIP_SQ_POINT")
        {
            if (clipState == 0)
            {
                p1x = mx;
                p1y = my;
                clipState = 1;
                cout << "[CLIP] Square center set.\n";
                return 0;
            }
            if (clipState == 1)
            {
                int side = max(abs(mx - p1x), abs(my - p1y));
                sqLeft = p1x - side;
                sqRight = p1x + side;
                sqTop = p1y - side;
                sqBottom = p1y + side;
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
                    x1Line = mx;
                    y1Line = my;
                    clipState = 4;
                    cout << "[CLIP] Line start set.\n";
                }
                else // clipState == 4
                {
                    double lx1 = x1Line, ly1 = y1Line, lx2 = mx, ly2 = my;
                    HDC hdc = GetDC(hwnd);
                    CoheSuth(hdc, lx1, ly1, lx2, ly2, sqLeft, sqRight, sqBottom, sqTop);
                    DrawSquareWindow(hdc);
                    ReleaseDC(hwnd, hdc);
                    Shape s;
                    s.type = "CLIP_SQ_LINE";
                    s.color = currentColor;
                    s.params = {(int)sqLeft, (int)sqTop, (int)sqRight, (int)sqBottom,
                                (int)x1Line, (int)y1Line, mx, my};
                    shapes.push_back(s);
                    clipState = 3;
                    cout << "[CLIP] Line clipped.\n";
                }
                return 0;
            }

            // ── Square Point clipping ─────────────────────────────────

            if (activeAlgorithm == "CLIP_SQ_POINT")
            {
                bool inside = pointclip(mx, my, sqLeft, sqRight, sqBottom, sqTop);
                HDC hdc = GetDC(hwnd);
                if (inside)
                {
                    HBRUSH br = CreateSolidBrush(currentColor);
                    HBRUSH old = (HBRUSH)SelectObject(hdc, br);
                    Ellipse(hdc, mx - 3, my - 3, mx + 3, my + 3);
                    SelectObject(hdc, old);
                    DeleteObject(br);

                    Shape s;
                    s.type = "CLIP_SQ_POINT";
                    s.color = currentColor;
                    s.params = {(int)sqLeft, (int)sqTop, (int)sqRight, (int)sqBottom,
                                mx, my};
                    shapes.push_back(s);
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
                clipCircleCX = mx;
                clipCircleCY = my;
                clipState = 1;
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
                    x1Line = mx;
                    y1Line = my;
                    clipState = 4;
                    cout << "[CLIP] Line start set.\n";
                }
                else // clipState == 4
                {
                    double ox1, oy1, ox2, oy2;
                    HDC hdc = GetDC(hwnd);
                    if (ClipLineToCircle(x1Line, y1Line, mx, my, ox1, oy1, ox2, oy2))
                    {
                        LineMidpoint(hdc, (int)round(ox1), (int)round(oy1),
                                     (int)round(ox2), (int)round(oy2), currentColor);

                        Shape s;
                        s.type = "CLIP_CIRCLE_LINE";
                        s.color = currentColor;
                        s.params = {(int)clipCircleCX, (int)clipCircleCY, (int)clipCircleR,
                                    (int)round(ox1), (int)round(oy1),
                                    (int)round(ox2), (int)round(oy2)};
                        shapes.push_back(s);
                    }
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
                    // Draw accepted point in currentColor (not hardcoded black)
                    HBRUSH br = CreateSolidBrush(currentColor);
                    HBRUSH old = (HBRUSH)SelectObject(hdc, br);
                    Ellipse(hdc, mx - 3, my - 3, mx + 3, my + 3);
                    SelectObject(hdc, old);
                    DeleteObject(br);
                    Shape s;
                    s.type = "CLIP_CIRCLE_POINT";
                    s.color = currentColor;
                    s.params = {(int)clipCircleCX, (int)clipCircleCY, (int)clipCircleR,
                                mx, my};
                    shapes.push_back(s);
                    cout << "[CLIP] Point INSIDE circle drawn.\n";
                }
                else
                    cout << "[CLIP] Point OUTSIDE circle  not drawn.\n";
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
                curveCollecting = false;
                curvePoints.clear();
            }
        }
        // ── Right-click finalises Convex / Non-Convex polygon fill ─────
        if ((activeAlgorithm == "FILL_CONVEX" || activeAlgorithm == "FILL_NONCONVEX") && fillPolyCollecting && (int)fillPolyPoints.size() >= 3)
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
            for (auto &fp : fillPolyPoints)
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
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);

            HDC hdc = GetDC(hwnd);

            // Redraw the clipping window border so it remains visible
            DrawRectangleWindow(hdc);

            polygonclip(hdc, polyPoints.data(), (int)polyPoints.size(),
                        xLeft, xRight, yBottom, yTop);
            Shape s;
            s.type = "CLIP_RECT_POLY";
            s.color = currentColor;
            s.params = {(int)xLeft, (int)yTop, (int)xRight, (int)yBottom,
                        (int)polyPoints.size()};
            for (auto &pp : polyPoints)
            {
                s.params.push_back(pp.x);
                s.params.push_back(pp.y);
            }
            shapes.push_back(s);

            ReleaseDC(hwnd, hdc);
            cout << "[CLIP] Polygon clipped : inside portion drawn, outside removed.\n";
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