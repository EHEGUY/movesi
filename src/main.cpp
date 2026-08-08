#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef ES_CONTINUOUS
#define ES_CONTINUOUS       0x80000000
#endif
#ifndef ES_DISPLAY_REQUIRED
#define ES_DISPLAY_REQUIRED 0x00000002
#endif
#ifndef ES_SYSTEM_REQUIRED
#define ES_SYSTEM_REQUIRED  0x00000001
#endif

// Colors for Dark Mode
#define DARK_BG             RGB(18, 18, 18)
#define DARK_CARD           RGB(30, 30, 30)
#define DARK_CARD_BORDER    RGB(45, 45, 45)
#define DARK_TEXT           RGB(243, 244, 246)
#define DARK_TEXT_MUTED     RGB(156, 163, 175)

// Colors for Light Mode
#define LIGHT_BG            RGB(243, 244, 246)
#define LIGHT_CARD          RGB(255, 255, 255)
#define LIGHT_CARD_BORDER   RGB(229, 231, 235)
#define LIGHT_TEXT          RGB(17, 24, 39)
#define LIGHT_TEXT_MUTED    RGB(107, 114, 128)

// Shared Colors
#define COLOR_ACCENT        RGB(79, 70, 229)    // Indigo-600
#define COLOR_ACCENT_HOVER  RGB(99, 90, 245)
#define COLOR_ACTIVE        RGB(16, 185, 129)   // Emerald-500
#define COLOR_PAUSED        RGB(156, 163, 175)  // Cool Grey

// Global variables
HINSTANCE hInst;
HWND hMainWnd = NULL;
NOTIFYICONDATA nid = { 0 };
HICON hIconActive = NULL;
HICON hIconPaused = NULL;
HFONT hFontNormal = NULL;
HFONT hFontBold = NULL;
HFONT hFontTitle = NULL;
HFONT hFontStats = NULL;
HFONT hFontSmall = NULL;

// App States
bool isActive = false;
int selectedAction = 0; // 0 = Mouse Move, 1 = Mouse Click, 2 = Key Press
int sliderInterval = 30; // 5s to 120s
int secondsRemaining = 30;
time_t sessionStartTime = 0;
time_t accumulatedTimeActive = 0; // Tracks accumulated duration across active states
int totalActions = 0;
bool isDraggingSlider = false;
bool enableSchedule = false;   // 9am - 5pm Weekdays
bool enableBlackout = false;   // 6pm - 9am & Weekends
bool isDarkMode = true;

// DPI state and Scaled UI Layout Rects
int currentDpi = 96;
RECT scaledToggleRect;
RECT scaledCardMove;
RECT scaledCardClick;
RECT scaledCardKey;
RECT scaledSliderTrack;
RECT scaledCheckboxSchedule;
RECT scaledCheckboxBlackout;
RECT scaledStatsPanel;

int hoveredControl = 0; // 0=None, 1=Toggle, 2=CardMove, 3=CardClick, 4=CardKey, 5=Slider, 6=Schedule, 7=Blackout

// Function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool CheckIsDarkMode();
HICON CreateDynamicIcon(COLORREF color, bool isOpenRing);
void UpdateTrayIcon();
void PerformSimulationAction();
void CheckSchedules(HWND hWnd);
void FormatDuration(time_t seconds, wchar_t* buffer, size_t bufferSize);
void SetImmersiveDarkMode(HWND hWnd, bool dark);
void TrimMemory();
void SetSimulationState(bool active);
void RecalculateLayout(HWND hWnd);
void RecreateFonts(HWND hWnd);
int Scale(int value);

// Draw helpers
void DrawRoundedRect(HDC hdc, RECT rect, int radius, COLORREF fillColor, COLORREF borderColor, int borderWidth = 1) {
    HPEN hPen = CreatePen(PS_SOLID, borderWidth, borderColor);
    HBRUSH hBrush = CreateSolidBrush(fillColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void DrawCheckbox(HDC hdc, RECT rect, bool checked, bool darkTheme) {
    COLORREF bg = checked ? COLOR_ACCENT : (darkTheme ? DARK_CARD : LIGHT_CARD);
    COLORREF border = checked ? COLOR_ACCENT : (darkTheme ? DARK_CARD_BORDER : LIGHT_CARD_BORDER);
    DrawRoundedRect(hdc, rect, Scale(4), bg, border, 1);

    if (checked) {
        HPEN hPen = CreatePen(PS_SOLID, Scale(2), RGB(255, 255, 255));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        int cx = rect.left + (rect.right - rect.left) / 2;
        int cy = rect.top + (rect.bottom - rect.top) / 2;
        
        MoveToEx(hdc, cx - Scale(3), cy, NULL);
        LineTo(hdc, cx - Scale(1), cy + Scale(3));
        LineTo(hdc, cx + Scale(4), cy - Scale(2));

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }
}

// DPI scale helper
int Scale(int value) {
    return MulDiv(value, currentDpi, 96);
}

void RecalculateLayout(HWND hWnd) {
    // Determine current DPI
    HDC hdc = GetDC(hWnd);
    currentDpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);
    if (currentDpi == 0) currentDpi = 96;

    // Base unscaled layout dimensions scaled dynamically
    scaledToggleRect.left = Scale(280);
    scaledToggleRect.top = Scale(25);
    scaledToggleRect.right = Scale(330);
    scaledToggleRect.bottom = Scale(53);

    scaledCardMove.left = Scale(20);
    scaledCardMove.top = Scale(110);
    scaledCardMove.right = Scale(116);
    scaledCardMove.bottom = Scale(155);

    scaledCardClick.left = Scale(126);
    scaledCardClick.top = Scale(110);
    scaledCardClick.right = Scale(222);
    scaledCardClick.bottom = Scale(155);

    scaledCardKey.left = Scale(232);
    scaledCardKey.top = Scale(110);
    scaledCardKey.right = Scale(328);
    scaledCardKey.bottom = Scale(155);

    scaledSliderTrack.left = Scale(20);
    scaledSliderTrack.top = Scale(210);
    scaledSliderTrack.right = Scale(328);
    scaledSliderTrack.bottom = Scale(214);

    scaledStatsPanel.left = Scale(20);
    scaledStatsPanel.top = Scale(250);
    scaledStatsPanel.right = Scale(328);
    scaledStatsPanel.bottom = Scale(335);

    scaledCheckboxSchedule.left = Scale(20);
    scaledCheckboxSchedule.top = Scale(390);
    scaledCheckboxSchedule.right = Scale(36);
    scaledCheckboxSchedule.bottom = Scale(406);

    scaledCheckboxBlackout.left = Scale(20);
    scaledCheckboxBlackout.top = Scale(425);
    scaledCheckboxBlackout.right = Scale(36);
    scaledCheckboxBlackout.bottom = Scale(441);
}

void RecreateFonts(HWND hWnd) {
    if (hFontNormal) DeleteObject(hFontNormal);
    if (hFontBold) DeleteObject(hFontBold);
    if (hFontTitle) DeleteObject(hFontTitle);
    if (hFontStats) DeleteObject(hFontStats);
    if (hFontSmall) DeleteObject(hFontSmall);

    // Calculate height based on DPI (negative value for character height sizing)
    int nNormalHeight = -MulDiv(11, currentDpi, 72);
    int nTitleHeight = -MulDiv(20, currentDpi, 72);
    int nStatsHeight = -MulDiv(17, currentDpi, 72);
    int nSmallHeight = -MulDiv(9, currentDpi, 72);

    hFontNormal = CreateFontW(nNormalHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontBold = CreateFontW(nNormalHeight, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontTitle = CreateFontW(nTitleHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontStats = CreateFontW(nStatsHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontSmall = CreateFontW(nSmallHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

int GetControlUnderMouse(HWND hWnd, int x, int y) {
    POINT pt = { x, y };
    
    if (PtInRect(&scaledToggleRect, pt)) return 1;
    if (PtInRect(&scaledCardMove, pt)) return 2;
    if (PtInRect(&scaledCardClick, pt)) return 3;
    if (PtInRect(&scaledCardKey, pt)) return 4;
    
    RECT sliderGrabRect = { scaledSliderTrack.left - Scale(10), scaledSliderTrack.top - Scale(10), scaledSliderTrack.right + Scale(10), scaledSliderTrack.bottom + Scale(10) };
    if (PtInRect(&sliderGrabRect, pt)) return 5;
    
    if (PtInRect(&scaledCheckboxSchedule, pt)) return 6;
    if (PtInRect(&scaledCheckboxBlackout, pt)) return 7;
    
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // Named mutex to enforce single instance
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"MovesiSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Bring existing instance to foreground if it exists
        HWND hExisting = FindWindowW(L"MovesiWindowClass", L"Movesi");
        if (hExisting) {
            ShowWindow(hExisting, SW_RESTORE);
            SetForegroundWindow(hExisting);
        }
        CloseHandle(hMutex);
        return 0;
    }

    hInst = hInstance;
    srand((unsigned int)time(NULL));

    // Determine current theme
    isDarkMode = CheckIsDarkMode();

    // Create dynamic tray icons
    hIconActive = CreateDynamicIcon(COLOR_ACTIVE, false);
    hIconPaused = CreateDynamicIcon(RGB(150, 150, 150), true);

    // Setup window class
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"MovesiWindowClass";
    
    if (!RegisterClassExW(&wcex)) {
        return 0;
    }

    // Set default unscaled size (360x480) and adjust according to starting DPI
    hMainWnd = CreateWindowW(L"MovesiWindowClass", L"Movesi", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 360, 480, 
        NULL, NULL, hInstance, NULL);

    if (!hMainWnd) {
        return 0;
    }

    // Recalculate layout metrics for initial DPI and size the window appropriately
    RecalculateLayout(hMainWnd);
    RecreateFonts(hMainWnd);

    RECT rc = { 0, 0, Scale(360), Scale(480) };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    SetWindowPos(hMainWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Initialize System Tray Icon
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hMainWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = hIconPaused;
    wcsncpy(nid.szTip, L"Movesi - Paused", ARRAYSIZE(nid.szTip) - 1);
    nid.szTip[ARRAYSIZE(nid.szTip) - 1] = L'\0';
    Shell_NotifyIcon(NIM_ADD, &nid);

    // Apply dark mode styling to window title bar dynamically
    SetImmersiveDarkMode(hMainWnd, isDarkMode);

    // Trim working set initially (once after window setup & tray registration)
    TrimMemory();

    // Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup resources
    Shell_NotifyIcon(NIM_DELETE, &nid);
    if (hIconActive) DestroyIcon(hIconActive);
    if (hIconPaused) DestroyIcon(hIconPaused);
    if (hFontNormal) DeleteObject(hFontNormal);
    if (hFontBold) DeleteObject(hFontBold);
    if (hFontTitle) DeleteObject(hFontTitle);
    if (hFontStats) DeleteObject(hFontStats);
    if (hFontSmall) DeleteObject(hFontSmall);

    CloseHandle(hMutex);

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // Start 1-second ticks
        SetTimer(hWnd, TIMER_SEC, 1000, NULL);
        break;

    case WM_USER + 1: // Tray Icon Notification Callback
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONDOWN) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            TrimMemory(); // Trim pages upon restore
        }
        else if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            
            wchar_t headerText[64];
            _snwprintf(headerText, ARRAYSIZE(headerText), L"Movesi (%s)", isActive ? L"Active" : L"Paused");
            AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 0, headerText);
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN, L"Open Interface");
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, isActive ? L"Pause Session" : L"Resume Session");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit Application");

            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_OPEN:
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            TrimMemory();
            break;
        case ID_TRAY_TOGGLE:
            SetSimulationState(!isActive);
            InvalidateRect(hWnd, NULL, FALSE);
            TrimMemory();
            break;
        case ID_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        break;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(hWnd, SW_HIDE);
            TrimMemory(); // Trim memory when hidden to tray
            return 0;
        }
        break;

    case WM_SETTINGCHANGE:
        if (lParam && wcscmp((wchar_t*)lParam, L"ImmersiveColorSet") == 0) {
            isDarkMode = CheckIsDarkMode();
            SetImmersiveDarkMode(hWnd, isDarkMode);

            if (hIconActive) DestroyIcon(hIconActive);
            if (hIconPaused) DestroyIcon(hIconPaused);
            hIconActive = CreateDynamicIcon(COLOR_ACTIVE, false);
            hIconPaused = CreateDynamicIcon(RGB(150, 150, 150), true);
            UpdateTrayIcon();
            
            InvalidateRect(hWnd, NULL, TRUE);
            TrimMemory();
        }
        break;

    case WM_DPICHANGED:
        {
            // Update DPI and recalculate all layouts and fonts
            currentDpi = LOWORD(wParam);
            RecalculateLayout(hWnd);
            RecreateFonts(hWnd);

            // Re-apply suggested window frame coordinates
            LPRECT lprcProposed = (LPRECT)lParam;
            SetWindowPos(hWnd, NULL, 
                lprcProposed->left, lprcProposed->top, 
                lprcProposed->right - lprcProposed->left, 
                lprcProposed->bottom - lprcProposed->top, 
                SWP_NOZORDER | SWP_NOACTIVATE);

            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_TIMER:
        if (wParam == TIMER_SEC) {
            CheckSchedules(hWnd);

            if (isActive) {
                secondsRemaining--;
                if (secondsRemaining <= 0) {
                    PerformSimulationAction();
                    totalActions++;
                    secondsRemaining = sliderInterval;
                }
            }
            InvalidateRect(hWnd, NULL, FALSE);
            // Dynamic working set trimming removed from recurring timer to avoid IO/PageFault thrashing.
        }
        break;

    case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int control = GetControlUnderMouse(hWnd, x, y);

            if (control == 1) { // Toggle Switch
                SetSimulationState(!isActive);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else if (control >= 2 && control <= 4) { // Action cards
                selectedAction = control - 2;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else if (control == 5) { // Slider
                isDraggingSlider = true;
                SetCapture(hWnd);
                
                int trackWidth = scaledSliderTrack.right - scaledSliderTrack.left;
                if (trackWidth > 0) {
                    int newInterval = 5 + (int)(((double)(x - scaledSliderTrack.left) / trackWidth) * (120 - 5));
                    if (newInterval < 5) newInterval = 5;
                    if (newInterval > 120) newInterval = 120;
                    sliderInterval = newInterval;
                    if (isActive) {
                        secondsRemaining = sliderInterval;
                    }
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else if (control == 6) { // Schedule checkbox
                enableSchedule = !enableSchedule;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else if (control == 7) { // Blackout checkbox
                enableBlackout = !enableBlackout;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        break;

    case WM_LBUTTONUP:
        if (isDraggingSlider) {
            isDraggingSlider = false;
            ReleaseCapture();
            TrimMemory();
        }
        break;

    case WM_MOUSEMOVE:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if (isDraggingSlider) {
                int trackWidth = scaledSliderTrack.right - scaledSliderTrack.left;
                if (trackWidth > 0) {
                    int newInterval = 5 + (int)(((double)(x - scaledSliderTrack.left) / trackWidth) * (120 - 5));
                    if (newInterval < 5) newInterval = 5;
                    if (newInterval > 120) newInterval = 120;
                    sliderInterval = newInterval;
                    if (isActive) {
                        secondsRemaining = sliderInterval;
                    }
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else {
                int newHover = GetControlUnderMouse(hWnd, x, y);
                if (newHover != hoveredControl) {
                    hoveredControl = newHover;
                    
                    TRACKMOUSEEVENT tme = { 0 };
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hWnd;
                    TrackMouseEvent(&tme);
                    
                    InvalidateRect(hWnd, NULL, FALSE);
                }

                if (hoveredControl > 0) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                }
                else {
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                }
            }
        }
        break;

    case WM_MOUSELEAVE:
        if (hoveredControl != 0) {
            hoveredControl = 0;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            // Double Buffering setup
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

            COLORREF bgCol = isDarkMode ? DARK_BG : LIGHT_BG;
            COLORREF cardCol = isDarkMode ? DARK_CARD : LIGHT_CARD;
            COLORREF borderCol = isDarkMode ? DARK_CARD_BORDER : LIGHT_CARD_BORDER;
            COLORREF textCol = isDarkMode ? DARK_TEXT : LIGHT_TEXT;
            COLORREF textMutedCol = isDarkMode ? DARK_TEXT_MUTED : LIGHT_TEXT_MUTED;

            HBRUSH hBackBrush = CreateSolidBrush(bgCol);
            FillRect(memDC, &clientRect, hBackBrush);
            DeleteObject(hBackBrush);

            SetBkMode(memDC, TRANSPARENT);

            // Title
            SetTextColor(memDC, textCol);
            SelectObject(memDC, hFontTitle);
            TextOutW(memDC, Scale(20), Scale(20), L"Movesi", 6);

            // Subtitle
            SelectObject(memDC, hFontNormal);
            if (isActive) {
                SetTextColor(memDC, COLOR_ACTIVE);
                TextOutW(memDC, Scale(20), Scale(52), L"Active  •  Session Protected", 28);
            }
            else {
                SetTextColor(memDC, textMutedCol);
                TextOutW(memDC, Scale(20), Scale(52), L"Paused  •  Session May Expire", 29);
            }

            // Toggle Switch
            COLORREF toggleBg = isActive ? COLOR_ACTIVE : (isDarkMode ? DARK_CARD_BORDER : LIGHT_CARD_BORDER);
            COLORREF toggleBorder = isActive ? COLOR_ACTIVE : (isDarkMode ? DARK_CARD_BORDER : LIGHT_CARD_BORDER);
            if (hoveredControl == 1) {
                toggleBg = isActive ? RGB(20, 205, 149) : (isDarkMode ? RGB(65, 65, 65) : RGB(200, 200, 200));
            }
            DrawRoundedRect(memDC, scaledToggleRect, Scale(28), toggleBg, toggleBorder, 1);

            RECT knobRect;
            int knobRadius = Scale(22);
            int knobYOffset = scaledToggleRect.top + (scaledToggleRect.bottom - scaledToggleRect.top - knobRadius) / 2;
            if (isActive) {
                knobRect.left = scaledToggleRect.right - knobRadius - Scale(3);
                knobRect.right = scaledToggleRect.right - Scale(3);
            }
            else {
                knobRect.left = scaledToggleRect.left + Scale(3);
                knobRect.right = scaledToggleRect.left + Scale(3) + knobRadius;
            }
            knobRect.top = knobYOffset;
            knobRect.bottom = knobYOffset + knobRadius;
            DrawRoundedRect(memDC, knobRect, knobRadius, RGB(255, 255, 255), RGB(255, 255, 255), 1);

            // Section Action Mode
            SetTextColor(memDC, textMutedCol);
            SelectObject(memDC, hFontSmall);
            TextOutW(memDC, Scale(20), Scale(92), L"SIMULATION MODE", 15);

            RECT cards[3] = { scaledCardMove, scaledCardClick, scaledCardKey };
            const wchar_t* cardLabels[3] = { L"Mouse Move", L"Mouse Click", L"Key Press" };
            
            for (int i = 0; i < 3; i++) {
                bool isSelected = (selectedAction == i);
                COLORREF cellFill = isSelected ? COLOR_ACCENT : cardCol;
                COLORREF cellBorder = isSelected ? COLOR_ACCENT : borderCol;
                
                if (hoveredControl == (2 + i) && !isSelected) {
                    cellFill = isDarkMode ? RGB(40, 40, 40) : RGB(248, 250, 252);
                }

                DrawRoundedRect(memDC, cards[i], Scale(12), cellFill, cellBorder, 1);
                
                SetTextColor(memDC, isSelected ? RGB(255, 255, 255) : textCol);
                SelectObject(memDC, hFontNormal);
                
                SIZE textSize;
                GetTextExtentPoint32W(memDC, cardLabels[i], (int)wcslen(cardLabels[i]), &textSize);
                int tx = cards[i].left + (cards[i].right - cards[i].left - textSize.cx) / 2;
                int ty = cards[i].top + (cards[i].bottom - cards[i].top - textSize.cy) / 2;
                
                TextOutW(memDC, tx, ty, cardLabels[i], (int)wcslen(cardLabels[i]));
            }

            // Slider Section
            SetTextColor(memDC, textMutedCol);
            SelectObject(memDC, hFontSmall);
            TextOutW(memDC, Scale(20), Scale(180), L"INTERVAL", 8);

            wchar_t valText[32];
            _snwprintf(valText, ARRAYSIZE(valText), L"Every %ds", sliderInterval);
            SetTextColor(memDC, textCol);
            SelectObject(memDC, hFontBold);
            TextOutW(memDC, Scale(85), Scale(178), valText, (int)wcslen(valText));

            if (isActive) {
                wchar_t countText[32];
                _snwprintf(countText, ARRAYSIZE(countText), L"Next action: %ds", secondsRemaining);
                SetTextColor(memDC, COLOR_ACTIVE);
                SelectObject(memDC, hFontBold);
                
                SIZE textSize;
                GetTextExtentPoint32W(memDC, countText, (int)wcslen(countText), &textSize);
                TextOutW(memDC, Scale(328) - textSize.cx, Scale(178), countText, (int)wcslen(countText));
            }

            // Track
            COLORREF trackUnfilled = isDarkMode ? DARK_CARD_BORDER : LIGHT_CARD_BORDER;
            DrawRoundedRect(memDC, scaledSliderTrack, Scale(4), trackUnfilled, trackUnfilled, 1);

            int knobX = scaledSliderTrack.left + (int)(((sliderInterval - 5.0) / (120.0 - 5.0)) * (scaledSliderTrack.right - scaledSliderTrack.left));
            RECT sliderFilled = { scaledSliderTrack.left, scaledSliderTrack.top, knobX, scaledSliderTrack.bottom };
            DrawRoundedRect(memDC, sliderFilled, Scale(4), COLOR_ACCENT, COLOR_ACCENT, 1);

            RECT knobSlider;
            int sKnobDiam = Scale(16);
            knobSlider.left = knobX - (sKnobDiam / 2);
            knobSlider.right = knobX + (sKnobDiam / 2);
            knobSlider.top = scaledSliderTrack.top + (scaledSliderTrack.bottom - scaledSliderTrack.top) / 2 - (sKnobDiam / 2);
            knobSlider.bottom = knobSlider.top + sKnobDiam;
            
            COLORREF knobCol = (hoveredControl == 5 || isDraggingSlider) ? COLOR_ACCENT_HOVER : COLOR_ACCENT;
            DrawRoundedRect(memDC, knobSlider, sKnobDiam, RGB(255, 255, 255), knobCol, Scale(2));

            // Stats Panel
            DrawRoundedRect(memDC, scaledStatsPanel, Scale(16), cardCol, borderCol, 1);

            SetTextColor(memDC, textMutedCol);
            SelectObject(memDC, hFontSmall);
            TextOutW(memDC, Scale(40), Scale(265), L"TIME ACTIVE", 11);

            wchar_t durStr[32] = L"00:00:00";
            time_t totalSeconds = accumulatedTimeActive;
            if (isActive && sessionStartTime != 0) {
                totalSeconds += time(NULL) - sessionStartTime;
            }
            FormatDuration(totalSeconds, durStr, ARRAYSIZE(durStr));
            
            SetTextColor(memDC, textCol);
            SelectObject(memDC, hFontStats);
            TextOutW(memDC, Scale(40), Scale(285), durStr, (int)wcslen(durStr));

            SetTextColor(memDC, textMutedCol);
            SelectObject(memDC, hFontSmall);
            TextOutW(memDC, Scale(195), Scale(265), L"ACTIONS SIM'D", 13);

            wchar_t actStr[32];
            _snwprintf(actStr, ARRAYSIZE(actStr), L"%d", totalActions);
            SetTextColor(memDC, textCol);
            SelectObject(memDC, hFontStats);
            TextOutW(memDC, Scale(195), Scale(285), actStr, (int)wcslen(actStr));

            // Automation Section
            SetTextColor(memDC, textMutedCol);
            SelectObject(memDC, hFontSmall);
            TextOutW(memDC, Scale(20), Scale(360), L"AUTOMATION SETTINGS", 19);

            DrawCheckbox(memDC, scaledCheckboxSchedule, enableSchedule, isDarkMode);
            SetTextColor(memDC, textCol);
            SelectObject(memDC, hFontNormal);
            TextOutW(memDC, Scale(48), Scale(389), L"Work Hours (9:00 AM - 5:00 PM, Weekdays)", 41);

            DrawCheckbox(memDC, scaledCheckboxBlackout, enableBlackout, isDarkMode);
            TextOutW(memDC, Scale(48), Scale(424), L"Blackout (6:00 PM - 9:00 AM & Weekends)", 39);

            BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
        }
        break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        TrimMemory();
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool CheckIsDarkMode() {
    HKEY hKey;
    DWORD value = 1; 
    DWORD size = sizeof(value);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    return value == 0;
}

HICON CreateDynamicIcon(COLORREF color, bool isOpenRing) {
    int size = GetSystemMetrics(SM_CXSMICON); 
    if (size == 0) size = 16;

    HDC hdc = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, size, size);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

    HBRUSH hBackBrush = CreateSolidBrush(RGB(0, 0, 0));
    RECT rect = { 0, 0, size, size };
    FillRect(hMemDC, &rect, hBackBrush);
    DeleteObject(hBackBrush);

    if (isOpenRing) {
        HPEN hRingPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        HBRUSH hRingBrush = CreateSolidBrush(RGB(0, 0, 0)); 
        HPEN hOldPen = (HPEN)SelectObject(hMemDC, hRingPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, hRingBrush);

        Ellipse(hMemDC, 1, 1, size - 1, size - 1);

        HBRUSH hDotBrush = CreateSolidBrush(color);
        SelectObject(hMemDC, hDotBrush);
        Ellipse(hMemDC, size / 2 - 2, size / 2 - 2, size / 2 + 2, size / 2 + 2);

        SelectObject(hMemDC, hOldPen);
        SelectObject(hMemDC, hOldBrush);
        DeleteObject(hRingPen);
        DeleteObject(hRingBrush);
        DeleteObject(hDotBrush);
    }
    else {
        HBRUSH hBrush = CreateSolidBrush(color);
        HPEN hPen = CreatePen(PS_SOLID, 1, color);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);
        HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);

        Ellipse(hMemDC, 1, 1, size - 1, size - 1);

        SelectObject(hMemDC, hOldBrush);
        SelectObject(hMemDC, hOldPen);
        DeleteObject(hBrush);
        DeleteObject(hPen);
    }

    HBITMAP hMask = CreateCompatibleBitmap(hdc, size, size);
    HDC hMaskDC = CreateCompatibleDC(hdc);
    HBITMAP hOldMask = (HBITMAP)SelectObject(hMaskDC, hMask);

    HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hMaskDC, &rect, hWhiteBrush);
    DeleteObject(hWhiteBrush);

    HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
    HPEN hBlackPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HBRUSH hOldMaskBrush = (HBRUSH)SelectObject(hMaskDC, hBlackBrush);
    HPEN hOldMaskPen = (HPEN)SelectObject(hMaskDC, hBlackPen);

    Ellipse(hMaskDC, 1, 1, size - 1, size - 1);

    SelectObject(hMaskDC, hOldMaskBrush);
    SelectObject(hMaskDC, hOldMaskPen);
    DeleteObject(hBlackBrush);
    DeleteObject(hBlackPen);

    ICONINFO iconInfo = { 0 };
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = hMask;
    iconInfo.hbmColor = hBitmap;

    HICON hIcon = CreateIconIndirect(&iconInfo);

    SelectObject(hMemDC, hOldBitmap);
    SelectObject(hMaskDC, hOldMask);
    DeleteDC(hMemDC);
    DeleteDC(hMaskDC);
    DeleteObject(hBitmap);
    DeleteObject(hMask);
    ReleaseDC(NULL, hdc);

    return hIcon;
}

void UpdateTrayIcon() {
    nid.hIcon = isActive ? hIconActive : hIconPaused;
    wcsncpy(nid.szTip, isActive ? L"Movesi - Active" : L"Movesi - Paused", ARRAYSIZE(nid.szTip) - 1);
    nid.szTip[ARRAYSIZE(nid.szTip) - 1] = L'\0';
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void PerformSimulationAction() {
    if (selectedAction == 0) {
        // Relative mouse movement (simulates hardware-level activity)
        // with a larger displacement (10 to 20 pixels) to reset the OS idle timer.
        int dx = (rand() % 11) + 10; // 10 to 20 pixels
        int dy = (rand() % 11) + 10; // 10 to 20 pixels
        if (rand() % 2) dx = -dx;
        if (rand() % 2) dy = -dy;

        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = dx;
        input.mi.dy = dy;

        SendInput(1, &input, sizeof(INPUT));
    }
    else if (selectedAction == 1) {
        INPUT inputs[2] = { 0 };
        
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

        SendInput(2, inputs, sizeof(INPUT));
    }
    else if (selectedAction == 2) {
        INPUT inputs[2] = { 0 };

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_F15;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_F15;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(2, inputs, sizeof(INPUT));
    }
}

void CheckSchedules(HWND hWnd) {
    SYSTEMTIME lt;
    GetLocalTime(&lt);

    bool shouldPause = false;
    bool shouldResume = false;

    if (enableBlackout) {
        bool isWeekend = (lt.wDayOfWeek == 0 || lt.wDayOfWeek == 6); 
        bool isNight = (lt.wHour >= 18 || lt.wHour < 9); 
        if (isWeekend || isNight) {
            shouldPause = true;
        }
    }

    if (enableSchedule && !shouldPause) {
        bool isWeekday = (lt.wDayOfWeek >= 1 && lt.wDayOfWeek <= 5);
        bool isWorkHours = (lt.wHour >= 9 && lt.wHour < 17);
        if (!isWeekday || !isWorkHours) {
            shouldPause = true;
        } else {
            shouldResume = true;
        }
    }

    if (shouldPause && isActive) {
        SetSimulationState(false);
        InvalidateRect(hWnd, NULL, FALSE);
    }
    else if (shouldResume && !isActive && !shouldPause) {
        SetSimulationState(true);
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void FormatDuration(time_t seconds, wchar_t* buffer, size_t bufferSize) {
    int h = (int)(seconds / 3600);
    int m = (int)((seconds % 3600) / 60);
    int s = (int)(seconds % 60);
    _snwprintf(buffer, bufferSize, L"%02d:%02d:%02d", h, m, s);
}

typedef HRESULT(WINAPI* PFN_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

void SetImmersiveDarkMode(HWND hWnd, bool dark) {
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        PFN_DwmSetWindowAttribute pfnDwmSetWindowAttribute = (PFN_DwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (pfnDwmSetWindowAttribute) {
            BOOL value = dark ? TRUE : FALSE;
            pfnDwmSetWindowAttribute(hWnd, 20, &value, sizeof(value));
        }
        FreeLibrary(hDwm);
    }
}

void TrimMemory() {
    HMODULE hPsapi = LoadLibraryW(L"psapi.dll");
    if (hPsapi) {
        typedef BOOL(WINAPI* PFN_EmptyWorkingSet)(HANDLE);
        PFN_EmptyWorkingSet pfnEmptyWorkingSet = (PFN_EmptyWorkingSet)GetProcAddress(hPsapi, "EmptyWorkingSet");
        if (pfnEmptyWorkingSet) {
            pfnEmptyWorkingSet(GetCurrentProcess());
        }
        FreeLibrary(hPsapi);
    }
}

void SetSimulationState(bool active) {
    isActive = active;
    if (active) {
        sessionStartTime = time(NULL);
        secondsRemaining = sliderInterval;
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
    } else {
        if (sessionStartTime != 0) {
            accumulatedTimeActive += time(NULL) - sessionStartTime;
            sessionStartTime = 0;
        }
        SetThreadExecutionState(ES_CONTINUOUS);
    }
    UpdateTrayIcon();
}
