#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <math.h>
#include "resource.h"

using namespace Gdiplus;

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "gdiplus.lib")

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

// Timers
#define TIMER_ANIM 303
#define TIMER_UAC  304

// Tray Commands
#define ID_TRAY_SCHED_WORK   204
#define ID_TRAY_SCHED_ALWAYS 205

template<typename T>
static inline T MathMin(T a, T b) { return (a < b) ? a : b; }

template<typename T>
static inline T MathMax(T a, T b) { return (a > b) ? a : b; }

// Palette matching the reference 3D Liquid Glass aesthetic
#define CANVAS_BG_TOP       Color(255, 244, 246, 249)  // Studio off-white top
#define CANVAS_BG_BOT       Color(255, 235, 238, 243)  // Studio off-white bottom

#define TEXT_PRIMARY_CLR    Color(255, 24, 33, 47)     // Deep slate charcoal
#define TEXT_SECONDARY_CLR  Color(255, 78, 93, 112)    // Slate subtitle
#define TEXT_MUTED_CLR      Color(255, 126, 140, 158)  // Light muted slate
#define TEXT_LIGHT_CLR      Color(255, 255, 255, 255)

#define BLUE_GLASS_TOP      Color(248, 162, 196, 248)  // Translucent sky/cornflower top
#define BLUE_GLASS_BOT      Color(255, 118, 160, 232)  // Rich cornflower bottom
#define BLUE_GLASS_HOVER_T  Color(250, 175, 206, 252)
#define BLUE_GLASS_HOVER_B  Color(255, 130, 172, 240)

#define WHITE_GLASS_TOP     Color(235, 255, 255, 255)  // Translucent frosted white top
#define WHITE_GLASS_BOT     Color(200, 245, 248, 252)  // Frosted white bottom
#define WHITE_GLASS_HOVER_T Color(250, 255, 255, 255)
#define WHITE_GLASS_HOVER_B Color(220, 248, 250, 254)

#define TRACK_BG_TOP        Color(160, 226, 232, 240)  // Pill track container top
#define TRACK_BG_BOT        Color(140, 216, 223, 233)  // Pill track container bottom

#define AMBER_WARN_TOP      Color(255, 251, 191, 36)
#define AMBER_WARN_BOT      Color(255, 217, 119, 6)

enum IconState {
    ICON_STATE_ACTIVE,
    ICON_STATE_PAUSED,
    ICON_STATE_WARNING
};

struct AppStateData {
    int totalActions;
    int accumulatedTimeActive;
    int selectedAction;    // 0 = Mouse Move, 1 = Key Press, 2 = Mouse Click
    int selectedSchedule;  // 0 = Work Hours, 1 = Always On, 2 = Custom
    int sliderInterval;    // 15s to 120s
    int customStartHour;   // Default 9
    int customEndHour;     // Default 17
    bool launchAtLogin;
};

// Global variables
HINSTANCE hInst;
HWND hMainWnd = NULL;
NOTIFYICONDATA nid = { 0 };
HICON hIconActive = NULL;
HICON hIconPaused = NULL;
HICON hIconWarning = NULL;

AppStateData g_state = { 0, 0, 0, 1, 45, 9, 17, false };
bool isActive = false;
bool isDraggingSlider = false;
int secondsRemaining = 45;
time_t sessionStartTime = 0;
float animPhase = 0.0f;
bool isElevatedWarning = false;
int currentDpi = 96;

// Smooth physics animation states
float animActionX = -1.0f;
float animActionW = -1.0f;
float targetActionX = 0.0f;
float targetActionW = 0.0f;

float animScheduleX = -1.0f;
float animScheduleW = -1.0f;
float targetScheduleX = 0.0f;
float targetScheduleW = 0.0f;

float animToggleLogin = 0.0f;
float targetToggleLogin = 0.0f;

float animKnobScale = 1.0f;
float targetKnobScale = 1.0f;

// Layout Rects (Dynamically scaled for 360x550 design)
RECT scaledHeaderRect;
RECT scaledCloseRect;
RECT scaledBtnActivateRect;
RECT scaledBtnHideRect;
RECT scaledStatusCardRect;
RECT scaledActionContainerRect;
RECT scaledActionPillRects[3];
RECT scaledScheduleContainerRect;
RECT scaledSchedulePillRects[3];
RECT scaledSliderRect;
RECT scaledToggleLoginRect;
RECT scaledWarningChipRect;

// Hovered Control ID:
// 0=None, 1=Close, 2=BtnActivate, 3=BtnHide, 4..6=ActionPills, 7..9=SchedPills, 10=Slider, 11=ToggleLogin, 12=WarningBtn
int hoveredControl = 0;

// Function Declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HICON CreateDynamicIcon(COLORREF color, IconState state);
void UpdateTrayIcon();
void PerformSimulationAction();
void CheckSchedules(HWND hWnd);
void FormatDuration(time_t seconds, wchar_t* buffer, size_t bufferSize);
void SetupDwmWindow(HWND hWnd);
void TrimMemory();
void SetSimulationState(bool active);
void RecalculateLayout(HWND hWnd);
int Scale(int value);
bool CheckForegroundElevation();
bool IsCurrentProcessElevated();
bool SetStartupShortcut(bool enable);
bool IsStartupShortcutEnabled();
void SaveStateAsync();
void LoadStateSync();

int Scale(int value) {
    return MulDiv(value, currentDpi, 96);
}

// GDI+ Geometry Utilities
GraphicsPath* CreateRoundedPath(RectF r, float radius) {
    GraphicsPath* path = new GraphicsPath();
    float d = radius * 2.0f;
    if (d > r.Width) d = r.Width;
    if (d > r.Height) d = r.Height;
    path->AddArc(r.X, r.Y, d, d, 180, 90);
    path->AddArc(r.X + r.Width - d, r.Y, d, d, 270, 90);
    path->AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0, 90);
    path->AddArc(r.X, r.Y + r.Height - d, d, d, 90, 90);
    path->CloseFigure();
    return path;
}

// Multi-pass progressive soft ambient shadow (VisionOS depth)
void DrawSoftDropShadow(Graphics& g, RectF rect, float radius, float maxSpread = 8.0f, float offsetY = 4.0f, BYTE baseAlpha = 16) {
    const int passes = 5;
    for (int i = passes; i >= 1; i--) {
        float fraction = (float)i / (float)passes;
        float spread = maxSpread * fraction;
        float curY = offsetY * fraction;
        RectF sRect(rect.X - spread * 0.4f, rect.Y + curY - spread * 0.1f, rect.Width + spread * 0.8f, rect.Height + spread * 0.8f);
        GraphicsPath* p = CreateRoundedPath(sRect, radius + spread * 0.35f);
        BYTE a = (BYTE)(baseAlpha * (1.0f - (fraction * 0.65f)));
        SolidBrush sBrush(Color(a, 18, 28, 48));
        g.FillPath(&sBrush, p);
        delete p;
    }
}

// Liquid Blue Glass Pill (Directly matching "Continue" in reference image)
void DrawLiquidBluePill(Graphics& g, RectF rect, float radius, bool isHovered, bool isPressed) {
    DrawSoftDropShadow(g, rect, radius, 10.0f, 5.0f, isHovered ? 24 : 18);

    GraphicsPath* path = CreateRoundedPath(rect, radius);

    // Liquid Blue Gradient Fill
    Color cTop = isHovered ? BLUE_GLASS_HOVER_T : BLUE_GLASS_TOP;
    Color cBot = isHovered ? BLUE_GLASS_HOVER_B : BLUE_GLASS_BOT;
    if (isPressed) {
        cTop = Color(240, 142, 180, 236);
        cBot = Color(255, 105, 148, 222);
    }
    LinearGradientBrush bodyBrush(rect, cTop, cBot, LinearGradientModeVertical);
    g.FillPath(&bodyBrush, path);

    // Top Glossy Horizon Glare (Smooth liquid sheen with zero seam lines)
    LinearGradientBrush sheenBrush(rect, Color(95, 255, 255, 255), Color(0, 255, 255, 255), LinearGradientModeVertical);
    sheenBrush.SetBlendBellShape(0.0f, 0.48f);
    g.FillPath(&sheenBrush, path);

    // Specular Chamfer Rim (Top Edge Highlight)
    Pen specPen(Color(220, 255, 255, 255), 1.2f);
    g.DrawPath(&specPen, path);

    delete path;
}

// Frosted Acrylic Glass Pill & Card (Matching "Cancel", "Search", and "Recommendations")
void DrawFrostedGlassCard(Graphics& g, RectF rect, float radius, bool isHovered = false, bool hasShadow = true) {
    if (hasShadow) {
        DrawSoftDropShadow(g, rect, radius, 12.0f, 5.0f, isHovered ? 20 : 14);
    }

    GraphicsPath* path = CreateRoundedPath(rect, radius);

    // Translucent Frosted White Body
    Color cTop = isHovered ? WHITE_GLASS_HOVER_T : WHITE_GLASS_TOP;
    Color cBot = isHovered ? WHITE_GLASS_HOVER_B : WHITE_GLASS_BOT;
    LinearGradientBrush bodyBrush(rect, cTop, cBot, LinearGradientModeVertical);
    g.FillPath(&bodyBrush, path);

    // Subtle Top Horizon Specular Highlight (Seamless liquid fade)
    LinearGradientBrush sheenBrush(rect, Color(80, 255, 255, 255), Color(0, 255, 255, 255), LinearGradientModeVertical);
    sheenBrush.SetBlendBellShape(0.0f, 0.45f);
    g.FillPath(&sheenBrush, path);

    // Specular White Top Rim
    Pen rimPen(Color(230, 255, 255, 255), 1.2f);
    g.DrawPath(&rimPen, path);

    delete path;
}

// 3D Glass Toggle Switch (Modeled after "Notifications" toggle in reference image)
void DrawGlassToggleSwitch(Graphics& g, RectF rect, float animPos, bool isHovered) {
    float radius = rect.Height * 0.5f;

    // 1. Capsule Track
    GraphicsPath* trackPath = CreateRoundedPath(rect, radius);
    LinearGradientBrush trackBrush(rect, TRACK_BG_TOP, TRACK_BG_BOT, LinearGradientModeVertical);
    g.FillPath(&trackBrush, trackPath);

    // Active Blue Wash on the active side
    if (animPos > 0.01f) {
        float fillW = rect.Height + (rect.Width - rect.Height) * animPos;
        RectF fillR(rect.X, rect.Y, fillW, rect.Height);
        GraphicsPath* fillPath = CreateRoundedPath(fillR, radius);
        Region clip(trackPath);
        g.SetClip(&clip, CombineModeReplace);
        Color washColor((BYTE)(195 * animPos), 135, 178, 238);
        SolidBrush washBrush(washColor);
        g.FillPath(&washBrush, fillPath);
        g.ResetClip();
        delete fillPath;
    }

    Pen trackRim(Color(180, 220, 228, 236), 1.0f);
    g.DrawPath(&trackRim, trackPath);
    delete trackPath;

    // 2. 3D Circular Knob
    float knobDiam = rect.Height - 4.0f;
    float minX = rect.X + 2.0f;
    float maxX = rect.X + rect.Width - knobDiam - 2.0f;
    float curX = minX + (maxX - minX) * animPos;
    float curY = rect.Y + 2.0f;
    RectF knobRect(curX, curY, knobDiam, knobDiam);

    // Knob Contact Shadow
    DrawSoftDropShadow(g, knobRect, knobDiam * 0.5f, 6.0f, 2.5f, 24);

    GraphicsPath* knobPath = new GraphicsPath();
    knobPath->AddEllipse(knobRect);

    // Dynamic 3D Spherical/Cylindrical Knob Shading
    Color knobTop, knobBot;
    if (animPos > 0.5f) { // Active Blue 3D Knob (like left toggle in image)
        knobTop = Color(255, 155, 195, 248);
        knobBot = Color(255, 105, 150, 224);
    } else { // Inactive Pearl White 3D Knob (like right toggle in image)
        knobTop = Color(255, 255, 255, 255);
        knobBot = Color(255, 224, 228, 234);
    }
    LinearGradientBrush knobBrush(knobRect, knobTop, knobBot, LinearGradientModeVertical);
    g.FillPath(&knobBrush, knobPath);

    // Specular Rim on Knob
    Pen knobRim(Color(240, 255, 255, 255), 1.0f);
    g.DrawPath(&knobRim, knobPath);

    delete knobPath;
}

void RecalculateLayout(HWND hWnd) {
    HDC hdc = GetDC(hWnd);
    currentDpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);
    if (currentDpi == 0) currentDpi = 96;

    // Window Layout: 360 x 550 design
    // 1. Header (Y: 12..44)
    scaledHeaderRect.left = 0;
    scaledHeaderRect.top = 0;
    scaledHeaderRect.right = Scale(360);
    scaledHeaderRect.bottom = Scale(48);

    scaledCloseRect.left = Scale(316);
    scaledCloseRect.top = Scale(14);
    scaledCloseRect.right = Scale(344);
    scaledCloseRect.bottom = Scale(42);

    // 2. Dual Top Action Pills (Y: 52..94, Height 42px, radius 21px)
    // Mirrors "Continue" (Blue glass) and "Cancel" (Frosted glass)
    scaledBtnActivateRect.left = Scale(18);
    scaledBtnActivateRect.top = Scale(52);
    scaledBtnActivateRect.right = Scale(174);
    scaledBtnActivateRect.bottom = Scale(94);

    scaledBtnHideRect.left = Scale(186);
    scaledBtnHideRect.top = Scale(52);
    scaledBtnHideRect.right = Scale(342);
    scaledBtnHideRect.bottom = Scale(94);

    // 3. Status Platter / Hero Card (Y: 104..212, Height 108px, radius 20px)
    // Mirrors "Recommendations" Platter
    scaledStatusCardRect.left = Scale(18);
    scaledStatusCardRect.top = Scale(104);
    scaledStatusCardRect.right = Scale(342);
    scaledStatusCardRect.bottom = Scale(212);

    // 4. Action Mode Segmented Pill (Label Y: 224, Track Y: 242..278, Height 36px)
    scaledActionContainerRect.left = Scale(18);
    scaledActionContainerRect.top = Scale(242);
    scaledActionContainerRect.right = Scale(342);
    scaledActionContainerRect.bottom = Scale(278);

    int actW = (scaledActionContainerRect.right - scaledActionContainerRect.left) / 3;
    for (int i = 0; i < 3; i++) {
        scaledActionPillRects[i].left = scaledActionContainerRect.left + (i * actW);
        scaledActionPillRects[i].top = scaledActionContainerRect.top;
        scaledActionPillRects[i].right = (i == 2) ? scaledActionContainerRect.right : (scaledActionContainerRect.left + ((i + 1) * actW));
        scaledActionPillRects[i].bottom = scaledActionContainerRect.bottom;
    }

    targetActionX = (float)scaledActionPillRects[g_state.selectedAction].left;
    targetActionW = (float)(scaledActionPillRects[g_state.selectedAction].right - scaledActionPillRects[g_state.selectedAction].left);
    if (animActionX < 0.0f) {
        animActionX = targetActionX;
        animActionW = targetActionW;
    }

    // 5. Schedule Segmented Pill (Label Y: 290, Track Y: 308..344, Height 36px)
    scaledScheduleContainerRect.left = Scale(18);
    scaledScheduleContainerRect.top = Scale(308);
    scaledScheduleContainerRect.right = Scale(342);
    scaledScheduleContainerRect.bottom = Scale(344);

    int schW = (scaledScheduleContainerRect.right - scaledScheduleContainerRect.left) / 3;
    for (int i = 0; i < 3; i++) {
        scaledSchedulePillRects[i].left = scaledScheduleContainerRect.left + (i * schW);
        scaledSchedulePillRects[i].top = scaledScheduleContainerRect.top;
        scaledSchedulePillRects[i].right = (i == 2) ? scaledScheduleContainerRect.right : (scaledScheduleContainerRect.left + ((i + 1) * schW));
        scaledSchedulePillRects[i].bottom = scaledScheduleContainerRect.bottom;
    }

    targetScheduleX = (float)scaledSchedulePillRects[g_state.selectedSchedule].left;
    targetScheduleW = (float)(scaledSchedulePillRects[g_state.selectedSchedule].right - scaledSchedulePillRects[g_state.selectedSchedule].left);
    if (animScheduleX < 0.0f) {
        animScheduleX = targetScheduleX;
        animScheduleW = targetScheduleW;
    }

    // 6. Slider Section (Label Y: 356, Track Y: 378..402)
    scaledSliderRect.left = Scale(18);
    scaledSliderRect.top = Scale(356);
    scaledSliderRect.right = Scale(342);
    scaledSliderRect.bottom = Scale(406);

    // 7. Footer Toggle Row: "Launch at Windows startup" + 3D Toggle Switch (Y: 418..462)
    scaledToggleLoginRect.left = Scale(18);
    scaledToggleLoginRect.top = Scale(418);
    scaledToggleLoginRect.right = Scale(342);
    scaledToggleLoginRect.bottom = Scale(462);

    targetToggleLogin = g_state.launchAtLogin ? 1.0f : 0.0f;
    if (animToggleLogin < 0.0f) animToggleLogin = targetToggleLogin;

    // 8. Elevated Warning / Safe Status Chip (Y: 472..508)
    scaledWarningChipRect.left = Scale(18);
    scaledWarningChipRect.top = Scale(472);
    scaledWarningChipRect.right = Scale(342);
    scaledWarningChipRect.bottom = Scale(508);
}

int GetControlUnderMouse(HWND hWnd, int x, int y) {
    POINT pt = { x, y };
    if (PtInRect(&scaledCloseRect, pt)) return 1;
    if (PtInRect(&scaledBtnActivateRect, pt)) return 2;
    if (PtInRect(&scaledBtnHideRect, pt)) return 3;
    for (int i = 0; i < 3; i++) {
        if (PtInRect(&scaledActionPillRects[i], pt)) return 4 + i;
        if (PtInRect(&scaledSchedulePillRects[i], pt)) return 7 + i;
    }
    if (PtInRect(&scaledSliderRect, pt)) return 10;
    if (PtInRect(&scaledToggleLoginRect, pt)) return 11;
    if (isElevatedWarning && PtInRect(&scaledWarningChipRect, pt)) return 12;
    return 0;
}

// State JSON File I/O
void GetStateFilePath(wchar_t* outPath, size_t maxLen) {
    wchar_t appData[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData);
    _snwprintf(outPath, maxLen, L"%s\\Movesi", appData);
    CreateDirectoryW(outPath, NULL);
    _snwprintf(outPath, maxLen, L"%s\\Movesi\\state.json", appData);
}

DWORD WINAPI SaveStateThreadProc(LPVOID lpParam) {
    wchar_t filePath[MAX_PATH];
    GetStateFilePath(filePath, MAX_PATH);
    
    FILE* f = _wfopen(filePath, L"w");
    if (f) {
        fwprintf(f, L"{\n");
        fwprintf(f, L"  \"totalActions\": %d,\n", g_state.totalActions);
        fwprintf(f, L"  \"accumulatedTimeActive\": %d,\n", g_state.accumulatedTimeActive);
        fwprintf(f, L"  \"selectedAction\": %d,\n", g_state.selectedAction);
        fwprintf(f, L"  \"selectedSchedule\": %d,\n", g_state.selectedSchedule);
        fwprintf(f, L"  \"sliderInterval\": %d,\n", g_state.sliderInterval);
        fwprintf(f, L"  \"customStartHour\": %d,\n", g_state.customStartHour);
        fwprintf(f, L"  \"customEndHour\": %d\n", g_state.customEndHour);
        fwprintf(f, L"}\n");
        fclose(f);
    }
    return 0;
}

void SaveStateAsync() {
    HANDLE hThread = CreateThread(NULL, 0, SaveStateThreadProc, NULL, 0, NULL);
    if (hThread) CloseHandle(hThread);
}

void LoadStateSync() {
    wchar_t filePath[MAX_PATH];
    GetStateFilePath(filePath, MAX_PATH);
    FILE* f = _wfopen(filePath, L"r");
    if (f) {
        char buf[512] = { 0 };
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        if (n > 0) {
            buf[n] = '\0';
            int ta = 0, ata = 0, sa = 0, ss = 1, si = 45, csh = 9, ceh = 17;
            if (sscanf(buf, "%*[^t]totalActions\": %d", &ta) == 1) g_state.totalActions = ta;
            if (sscanf(buf, "%*[^a]accumulatedTimeActive\": %d", &ata) == 1) g_state.accumulatedTimeActive = ata;
            if (sscanf(buf, "%*[^s]selectedAction\": %d", &sa) == 1) g_state.selectedAction = sa;
            if (sscanf(buf, "%*[^s]selectedSchedule\": %d", &ss) == 1) g_state.selectedSchedule = ss;
            if (sscanf(buf, "%*[^s]sliderInterval\": %d", &si) == 1) g_state.sliderInterval = si;
            if (sscanf(buf, "%*[^c]customStartHour\": %d", &csh) == 1) g_state.customStartHour = csh;
            if (sscanf(buf, "%*[^c]customEndHour\": %d", &ceh) == 1) g_state.customEndHour = ceh;
        }
    }
}

static const GUID MY_CLSID_ShellLink = { 0x00021401, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID MY_IID_IShellLinkW = { 0x000214F9, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID MY_IID_IPersistFile = { 0x0000010b, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

typedef HRESULT(WINAPI* PFN_CoInitialize)(LPVOID);
typedef HRESULT(WINAPI* PFN_CoCreateInstance)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
typedef void(WINAPI* PFN_CoUninitialize)(void);

bool SetStartupShortcut(bool enable) {
    wchar_t startupPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath))) {
        return false;
    }
    wchar_t shortcutPath[MAX_PATH];
    _snwprintf(shortcutPath, MAX_PATH, L"%s\\Movesi.lnk", startupPath);
    
    if (!enable) {
        DeleteFileW(shortcutPath);
        g_state.launchAtLogin = false;
        return true;
    }
    
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    
    HMODULE hOle32 = LoadLibraryW(L"ole32.dll");
    if (!hOle32) return false;
    
    PFN_CoInitialize pfnCoInitialize = (PFN_CoInitialize)GetProcAddress(hOle32, "CoInitialize");
    PFN_CoCreateInstance pfnCoCreateInstance = (PFN_CoCreateInstance)GetProcAddress(hOle32, "CoCreateInstance");
    PFN_CoUninitialize pfnCoUninitialize = (PFN_CoUninitialize)GetProcAddress(hOle32, "CoUninitialize");
    
    bool success = false;
    if (pfnCoInitialize && pfnCoCreateInstance && pfnCoUninitialize) {
        pfnCoInitialize(NULL);
        IShellLinkW* psl = NULL;
        HRESULT hr = pfnCoCreateInstance(MY_CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, MY_IID_IShellLinkW, (void**)&psl);
        if (SUCCEEDED(hr) && psl) {
            psl->SetPath(exePath);
            psl->SetDescription(L"Movesi Session Protection Utility");
            
            IPersistFile* ppf = NULL;
            hr = psl->QueryInterface(MY_IID_IPersistFile, (void**)&ppf);
            if (SUCCEEDED(hr) && ppf) {
                ppf->Save(shortcutPath, TRUE);
                ppf->Release();
                g_state.launchAtLogin = true;
                success = true;
            }
            psl->Release();
        }
        pfnCoUninitialize();
    }
    FreeLibrary(hOle32);
    return success;
}

bool IsStartupShortcutEnabled() {
    wchar_t startupPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath))) {
        return false;
    }
    wchar_t shortcutPath[MAX_PATH];
    _snwprintf(shortcutPath, MAX_PATH, L"%s\\Movesi.lnk", startupPath);
    DWORD dwAttrib = GetFileAttributesW(shortcutPath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

#ifndef TokenElevation
#define TokenElevation ((TOKEN_INFORMATION_CLASS)20)
#endif

typedef struct _MY_TOKEN_ELEVATION {
    DWORD TokenIsElevated;
} MY_TOKEN_ELEVATION;

bool IsCurrentProcessElevated() {
    HANDLE hToken = NULL;
    bool isElevated = false;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        MY_TOKEN_ELEVATION elevation;
        DWORD dwSize = sizeof(elevation);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
            isElevated = (elevation.TokenIsElevated != 0);
        }
        CloseHandle(hToken);
    }
    return isElevated;
}

bool CheckForegroundElevation() {
    return false; // Safe fallback
}

typedef HRESULT(WINAPI* PFN_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

void SetupDwmWindow(HWND hWnd) {
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        PFN_DwmSetWindowAttribute pfnSetAttr = (PFN_DwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (pfnSetAttr) {
            DWORD corner = 2; // DWMWCP_ROUND (Smooth rounded window corners on Win11)
            pfnSetAttr(hWnd, 33, &corner, sizeof(corner));
        }
        FreeLibrary(hDwm);
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"MovesiSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(L"MovesiWindowClass", L"Movesi");
        if (hExisting) {
            ShowWindow(hExisting, SW_SHOW);
            SetForegroundWindow(hExisting);
        }
        CloseHandle(hMutex);
        return 0;
    }

    hInst = hInstance;
    srand((unsigned int)time(NULL));

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    LoadStateSync();
    g_state.launchAtLogin = IsStartupShortcutEnabled();
    targetToggleLogin = g_state.launchAtLogin ? 1.0f : 0.0f;
    animToggleLogin = targetToggleLogin;
    secondsRemaining = g_state.sliderInterval;

    hIconActive = CreateDynamicIcon(RGB(115, 160, 230), ICON_STATE_ACTIVE);
    hIconPaused = CreateDynamicIcon(RGB(148, 163, 184), ICON_STATE_PAUSED);
    hIconWarning = CreateDynamicIcon(RGB(245, 158, 11), ICON_STATE_WARNING);

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL; // Prevent background flickering
    wcex.lpszClassName = L"MovesiWindowClass";

    if (!RegisterClassExW(&wcex)) {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    // 360 x 550 Frameless Window
    hMainWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"MovesiWindowClass", L"Movesi",
        WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 360, 550, NULL, NULL, hInstance, NULL);

    if (!hMainWnd) {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    RecalculateLayout(hMainWnd);
    SetupDwmWindow(hMainWnd);

    // Apply smooth window rounded clipping
    int winW = Scale(360);
    int winH = Scale(550);
    HRGN hRgn = CreateRoundRectRgn(0, 0, winW + 1, winH + 1, Scale(26), Scale(26));
    SetWindowRgn(hMainWnd, hRgn, TRUE);

    // Position window nicely at bottom-right above taskbar
    RECT workArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int posX = workArea.right - winW - Scale(20);
    int posY = workArea.bottom - winH - Scale(20);
    SetWindowPos(hMainWnd, HWND_TOPMOST, posX, posY, winW, winH, SWP_SHOWWINDOW);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hMainWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = hIconPaused;
    wcsncpy(nid.szTip, L"Movesi - Paused", ARRAYSIZE(nid.szTip) - 1);
    nid.szTip[ARRAYSIZE(nid.szTip) - 1] = L'\0';
    Shell_NotifyIcon(NIM_ADD, &nid);

    TrimMemory();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SaveStateAsync();

    Shell_NotifyIcon(NIM_DELETE, &nid);
    if (hIconActive) DestroyIcon(hIconActive);
    if (hIconPaused) DestroyIcon(hIconPaused);
    if (hIconWarning) DestroyIcon(hIconWarning);

    GdiplusShutdown(gdiplusToken);
    CloseHandle(hMutex);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        SetTimer(hWnd, TIMER_SEC, 1000, NULL);
        SetTimer(hWnd, TIMER_ANIM, 16, NULL); // 60 FPS smooth physics transitions
        SetTimer(hWnd, TIMER_UAC, 2000, NULL);
        break;

    case WM_NCHITTEST: {
        // Native Windows header dragging with zero jitter
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);
        if (pt.y < Scale(48) && !PtInRect(&scaledCloseRect, pt)) {
            return HTCAPTION;
        }
        return HTCLIENT;
    }

    case WM_USER + 1:
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONDOWN) {
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
            TrimMemory();
        }
        else if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();

            AppendMenuW(hMenu, isActive ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_TRAY_TOGGLE, L"Toggle Protection");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, (g_state.selectedSchedule == 0) ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_TRAY_SCHED_WORK, L"Schedule: Work Hours");
            AppendMenuW(hMenu, (g_state.selectedSchedule == 1) ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_TRAY_SCHED_ALWAYS, L"Schedule: Always On");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Quit Movesi");

            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_TOGGLE:
            SetSimulationState(!isActive);
            InvalidateRect(hWnd, NULL, FALSE);
            TrimMemory();
            break;
        case ID_TRAY_SCHED_WORK:
            g_state.selectedSchedule = 0;
            targetScheduleX = (float)scaledSchedulePillRects[0].left;
            targetScheduleW = (float)(scaledSchedulePillRects[0].right - scaledSchedulePillRects[0].left);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case ID_TRAY_SCHED_ALWAYS:
            g_state.selectedSchedule = 1;
            targetScheduleX = (float)scaledSchedulePillRects[1].left;
            targetScheduleW = (float)(scaledSchedulePillRects[1].right - scaledSchedulePillRects[1].left);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case ID_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        break;

    case WM_DPICHANGED: {
        currentDpi = LOWORD(wParam);
        RecalculateLayout(hWnd);

        int winW = Scale(360);
        int winH = Scale(550);
        HRGN hRgn = CreateRoundRectRgn(0, 0, winW + 1, winH + 1, Scale(26), Scale(26));
        SetWindowRgn(hWnd, hRgn, TRUE);

        LPRECT lprcProposed = (LPRECT)lParam;
        SetWindowPos(hWnd, NULL, lprcProposed->left, lprcProposed->top, winW, winH, SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }

    case WM_TIMER:
        if (wParam == TIMER_SEC) {
            CheckSchedules(hWnd);

            if (isActive) {
                secondsRemaining--;
                if (secondsRemaining <= 0) {
                    PerformSimulationAction();
                    g_state.totalActions++;

                    float jitter = 0.75f + ((rand() % 51) / 100.0f);
                    secondsRemaining = (int)(g_state.sliderInterval * jitter);
                    if (secondsRemaining < 10) secondsRemaining = 10;
                }
            }
            UpdateTrayIcon();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (wParam == TIMER_ANIM) {
            bool needRedraw = false;
            if (isActive) {
                animPhase += 0.075f;
                if (animPhase > 6.28318f) animPhase -= 6.28318f;
                needRedraw = true;
            }

            // Smooth spring transition for Action Mode sliding pill
            if (fabsf(animActionX - targetActionX) > 0.3f || fabsf(animActionW - targetActionW) > 0.3f) {
                animActionX += (targetActionX - animActionX) * 0.28f;
                animActionW += (targetActionW - animActionW) * 0.28f;
                needRedraw = true;
            } else {
                animActionX = targetActionX;
                animActionW = targetActionW;
            }

            // Smooth spring transition for Schedule sliding pill
            if (fabsf(animScheduleX - targetScheduleX) > 0.3f || fabsf(animScheduleW - targetScheduleW) > 0.3f) {
                animScheduleX += (targetScheduleX - animScheduleX) * 0.28f;
                animScheduleW += (targetScheduleW - animScheduleW) * 0.28f;
                needRedraw = true;
            } else {
                animScheduleX = targetScheduleX;
                animScheduleW = targetScheduleW;
            }

            // Smooth transition for Startup toggle knob
            if (fabsf(animToggleLogin - targetToggleLogin) > 0.02f) {
                animToggleLogin += (targetToggleLogin - animToggleLogin) * 0.25f;
                needRedraw = true;
            } else {
                animToggleLogin = targetToggleLogin;
            }

            // Smooth slider knob hover transition
            targetKnobScale = (hoveredControl == 10 || isDraggingSlider) ? 1.15f : 1.0f;
            if (fabsf(animKnobScale - targetKnobScale) > 0.01f) {
                animKnobScale += (targetKnobScale - animKnobScale) * 0.25f;
                needRedraw = true;
            } else {
                animKnobScale = targetKnobScale;
            }

            if (needRedraw) {
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (wParam == TIMER_UAC) {
            bool elevated = CheckForegroundElevation();
            if (elevated != isElevatedWarning) {
                isElevatedWarning = elevated;
                UpdateTrayIcon();
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        break;

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        int control = GetControlUnderMouse(hWnd, x, y);

        if (control == 1) { // Close button -> hide to tray
            ShowWindow(hWnd, SW_HIDE);
            TrimMemory();
        }
        else if (control == 2) { // Primary Action Button (Activate / Pause)
            SetSimulationState(!isActive);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (control == 3) { // Hide Button ("Cancel" style pill)
            ShowWindow(hWnd, SW_HIDE);
            TrimMemory();
        }
        else if (control >= 4 && control <= 6) { // Action mode pills
            int newAct = control - 4;
            if (newAct != g_state.selectedAction) {
                g_state.selectedAction = newAct;
                targetActionX = (float)scaledActionPillRects[newAct].left;
                targetActionW = (float)(scaledActionPillRects[newAct].right - scaledActionPillRects[newAct].left);
                InvalidateRect(hWnd, &scaledActionContainerRect, FALSE);
                SaveStateAsync();
            }
        }
        else if (control >= 7 && control <= 9) { // Schedule pills
            int newSched = control - 7;
            if (newSched != g_state.selectedSchedule) {
                g_state.selectedSchedule = newSched;
                targetScheduleX = (float)scaledSchedulePillRects[newSched].left;
                targetScheduleW = (float)(scaledSchedulePillRects[newSched].right - scaledSchedulePillRects[newSched].left);
                CheckSchedules(hWnd);
                InvalidateRect(hWnd, &scaledScheduleContainerRect, FALSE);
                SaveStateAsync();
            }
        }
        else if (control == 10) { // Slider clicked / start drag
            isDraggingSlider = true;
            SetCapture(hWnd);

            int trackLeft = scaledSliderRect.left + Scale(8);
            int trackRight = scaledSliderRect.right - Scale(8);
            int trackW = trackRight - trackLeft;
            if (trackW > 0) {
                int clampedX = MathMin(MathMax(x, trackLeft), trackRight);
                float ratio = (float)(clampedX - trackLeft) / (float)trackW;
                int val = 15 + (int)(ratio * (120 - 15) + 0.5f);
                g_state.sliderInterval = val;
                if (isActive) secondsRemaining = val;
                InvalidateRect(hWnd, &scaledSliderRect, FALSE);
            }
        }
        else if (control == 11) { // Startup Toggle Row
            bool newState = !g_state.launchAtLogin;
            SetStartupShortcut(newState);
            targetToggleLogin = newState ? 1.0f : 0.0f;
            InvalidateRect(hWnd, &scaledToggleLoginRect, FALSE);
        }
        else if (control == 12) { // Warning Chip: Relaunch Admin
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            ShellExecuteW(NULL, L"runas", exePath, NULL, NULL, SW_SHOWNORMAL);
            DestroyWindow(hWnd);
        }
        break;
    }

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        if (isDraggingSlider) {
            int trackLeft = scaledSliderRect.left + Scale(8);
            int trackRight = scaledSliderRect.right - Scale(8);
            int trackW = trackRight - trackLeft;
            if (trackW > 0) {
                int clampedX = MathMin(MathMax(x, trackLeft), trackRight);
                float ratio = (float)(clampedX - trackLeft) / (float)trackW;
                int val = 15 + (int)(ratio * (120 - 15) + 0.5f);
                if (val != g_state.sliderInterval) {
                    g_state.sliderInterval = val;
                    if (isActive) secondsRemaining = val;
                    InvalidateRect(hWnd, &scaledSliderRect, FALSE);
                }
            }
        }
        else {
            int ctrl = GetControlUnderMouse(hWnd, x, y);
            if (ctrl != hoveredControl) {
                hoveredControl = ctrl;
                InvalidateRect(hWnd, NULL, FALSE);

                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
            }
        }
        break;
    }

    case WM_LBUTTONUP:
        if (isDraggingSlider) {
            isDraggingSlider = false;
            ReleaseCapture();
            SaveStateAsync();
            InvalidateRect(hWnd, &scaledSliderRect, FALSE);
        }
        break;

    case WM_MOUSELEAVE:
        if (hoveredControl != 0) {
            hoveredControl = 0;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        // Double Buffering
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        {
            Graphics g(memDC);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

            // 0. Clean Studio Canvas Background
            RectF canvasRect(0, 0, (float)width, (float)height);
            LinearGradientBrush canvasBgBrush(canvasRect, CANVAS_BG_TOP, CANVAS_BG_BOT, LinearGradientModeVertical);
            g.FillRectangle(&canvasBgBrush, 0, 0, width, height);

            // Typography setup
            FontFamily fontFamily(L"Segoe UI");
            Gdiplus::Font fontTitle(&fontFamily, 14.5f, FontStyleBold, UnitPixel);
            Gdiplus::Font fontBadge(&fontFamily, 8.5f, FontStyleBold, UnitPixel);
            Gdiplus::Font fontHeroHeading(&fontFamily, 14.0f, FontStyleBold, UnitPixel);
            Gdiplus::Font fontSub(&fontFamily, 11.0f, FontStyleRegular, UnitPixel);
            Gdiplus::Font fontBtn(&fontFamily, 13.0f, FontStyleBold, UnitPixel);
            Gdiplus::Font fontSection(&fontFamily, 9.5f, FontStyleBold, UnitPixel);
            Gdiplus::Font fontPill(&fontFamily, 11.5f, FontStyleBold, UnitPixel);
            Gdiplus::Font fontStatVal(&fontFamily, 13.0f, FontStyleBold, UnitPixel);
            Gdiplus::Font fontStatLbl(&fontFamily, 9.5f, FontStyleRegular, UnitPixel);

            StringFormat centerFormat;
            centerFormat.SetAlignment(StringAlignmentCenter);
            centerFormat.SetLineAlignment(StringAlignmentCenter);

            StringFormat leftFormat;
            leftFormat.SetAlignment(StringAlignmentNear);
            leftFormat.SetLineAlignment(StringAlignmentCenter);

            SolidBrush textPrimaryBrush(TEXT_PRIMARY_CLR);
            SolidBrush textSecondaryBrush(TEXT_SECONDARY_CLR);
            SolidBrush textMutedBrush(TEXT_MUTED_CLR);
            SolidBrush textLightBrush(TEXT_LIGHT_CLR);

            // 1. Header: Movesi + Indicator bead + Frosted Close circle
            PointF ptTitle((float)Scale(20), (float)Scale(16));
            g.DrawString(L"Movesi", -1, &fontTitle, ptTitle, &textPrimaryBrush);

            // Small live indicator bead next to title
            float beadX = (float)Scale(84);
            float beadY = (float)Scale(24);
            float beadR = (float)Scale(4.5f);
            if (isActive) {
                float pulse = (sinf(animPhase * 3.0f) + 1.0f) * 0.5f;
                float pR = beadR + pulse * (float)Scale(3.0f);
                SolidBrush pulseB(Color((BYTE)(80 * (1.0f - pulse)), 115, 160, 232));
                g.FillEllipse(&pulseB, beadX - pR, beadY - pR, pR * 2.0f, pR * 2.0f);
            }
            Color beadC = isActive ? Color(255, 115, 160, 232) : Color(255, 160, 172, 188);
            SolidBrush beadB(beadC);
            g.FillEllipse(&beadB, beadX - beadR, beadY - beadR, beadR * 2.0f, beadR * 2.0f);
            Pen beadRimP(Color(220, 255, 255, 255), 1.0f);
            g.DrawEllipse(&beadRimP, beadX - beadR, beadY - beadR, beadR * 2.0f, beadR * 2.0f);

            // Close Button (Frosted Glass Circle with ✕)
            RectF closeR((float)scaledCloseRect.left, (float)scaledCloseRect.top, (float)(scaledCloseRect.right - scaledCloseRect.left), (float)(scaledCloseRect.bottom - scaledCloseRect.top));
            DrawFrostedGlassCard(g, closeR, closeR.Height * 0.5f, (hoveredControl == 1), true);
            g.DrawString(L"✕", -1, &fontSub, closeR, &centerFormat, &textSecondaryBrush);

            // 2. Dual Top Action Pills (Matching "Continue" & "Cancel" from Reference Image)
            // Left: Activate/Pause (Liquid Blue Glass)
            RectF btnActR((float)scaledBtnActivateRect.left, (float)scaledBtnActivateRect.top, (float)(scaledBtnActivateRect.right - scaledBtnActivateRect.left), (float)(scaledBtnActivateRect.bottom - scaledBtnActivateRect.top));
            if (isActive) {
                DrawLiquidBluePill(g, btnActR, btnActR.Height * 0.5f, (hoveredControl == 2), false);
                g.DrawString(L"Pause Session", -1, &fontBtn, btnActR, &centerFormat, &textPrimaryBrush);
            } else {
                DrawLiquidBluePill(g, btnActR, btnActR.Height * 0.5f, (hoveredControl == 2), false);
                g.DrawString(L"Start Session", -1, &fontBtn, btnActR, &centerFormat, &textPrimaryBrush);
            }

            // Right: Hide to Tray (Frosted Glass "Cancel" Pill)
            RectF btnHideR((float)scaledBtnHideRect.left, (float)scaledBtnHideRect.top, (float)(scaledBtnHideRect.right - scaledBtnHideRect.left), (float)(scaledBtnHideRect.bottom - scaledBtnHideRect.top));
            DrawFrostedGlassCard(g, btnHideR, btnHideR.Height * 0.5f, (hoveredControl == 3), true);
            g.DrawString(L"Hide to Tray", -1, &fontBtn, btnHideR, &centerFormat, &textPrimaryBrush);

            // 3. Status Platter / Hero Card (Matching "Recommendations" in Reference Image)
            RectF statusR((float)scaledStatusCardRect.left, (float)scaledStatusCardRect.top, (float)(scaledStatusCardRect.right - scaledStatusCardRect.left), (float)(scaledStatusCardRect.bottom - scaledStatusCardRect.top));
            DrawFrostedGlassCard(g, statusR, 20.0f, false, true);

            // Status Card Heading & Subtitle
            PointF ptCardHead(statusR.X + (float)Scale(16), statusR.Y + (float)Scale(14));
            g.DrawString(isActive ? L"Session Active" : L"Session Paused", -1, &fontHeroHeading, ptCardHead, &textPrimaryBrush);

            wchar_t subText[96];
            if (isActive) {
                _snwprintf(subText, ARRAYSIZE(subText), L"Next action in %ds • Mode: %s", secondsRemaining,
                    (g_state.selectedAction == 0) ? L"Mouse Move" : (g_state.selectedAction == 1 ? L"Key Press" : L"Mouse Click"));
            } else {
                if (g_state.selectedSchedule == 0) wcsncpy(subText, L"Resumes at 9:00 AM • Work Hours mode", ARRAYSIZE(subText) - 1);
                else if (g_state.selectedSchedule == 2) _snwprintf(subText, ARRAYSIZE(subText), L"Resumes at %02d:00 • Custom schedule", g_state.customStartHour);
                else wcsncpy(subText, L"Simulation idle • Click Start to protect session", ARRAYSIZE(subText) - 1);
            }
            PointF ptCardSub(statusR.X + (float)Scale(16), statusR.Y + (float)Scale(34));
            g.DrawString(subText, -1, &fontSub, ptCardSub, &textSecondaryBrush);

            // Status Divider
            Pen divPen(Color(40, 140, 160, 185), 1.0f);
            float divY = statusR.Y + (float)Scale(58);
            g.DrawLine(&divPen, statusR.X + (float)Scale(16), divY, statusR.GetRight() - (float)Scale(16), divY);

            // 3 Stats Columns inside card
            float statY = statusR.Y + (float)Scale(66);
            float statColW = (statusR.Width - (float)Scale(32)) / 3.0f;

            // Col 1: Active Time
            time_t totalSecs = g_state.accumulatedTimeActive;
            if (isActive && sessionStartTime != 0) {
                totalSecs += time(NULL) - sessionStartTime;
            }
            wchar_t szTime[32];
            FormatDuration(totalSecs, szTime, ARRAYSIZE(szTime));
            PointF ptS1V(statusR.X + (float)Scale(16), statY);
            g.DrawString(szTime, -1, &fontStatVal, ptS1V, &textPrimaryBrush);
            PointF ptS1L(statusR.X + (float)Scale(16), statY + (float)Scale(16));
            g.DrawString(L"Active Time", -1, &fontStatLbl, ptS1L, &textMutedBrush);

            // Col 2: Actions Sent
            wchar_t szActs[32];
            _snwprintf(szActs, ARRAYSIZE(szActs), L"%d", g_state.totalActions);
            PointF ptS2V(statusR.X + (float)Scale(16) + statColW, statY);
            g.DrawString(szActs, -1, &fontStatVal, ptS2V, &textPrimaryBrush);
            PointF ptS2L(statusR.X + (float)Scale(16) + statColW, statY + (float)Scale(16));
            g.DrawString(L"Actions Sent", -1, &fontStatLbl, ptS2L, &textMutedBrush);

            // Col 3: Interval Setting
            wchar_t szInt[32];
            _snwprintf(szInt, ARRAYSIZE(szInt), L"~%ds", g_state.sliderInterval);
            PointF ptS3V(statusR.X + (float)Scale(16) + statColW * 2.0f, statY);
            g.DrawString(szInt, -1, &fontStatVal, ptS3V, &textPrimaryBrush);
            PointF ptS3L(statusR.X + (float)Scale(16) + statColW * 2.0f, statY + (float)Scale(16));
            g.DrawString(L"Interval", -1, &fontStatLbl, ptS3L, &textMutedBrush);

            // 4. Action Simulation Mode - Sliding Segmented Glass Capsule ("Today | Week" style)
            PointF ptActTitle((float)Scale(20), (float)Scale(224));
            g.DrawString(L"ACTION SIMULATION", -1, &fontSection, ptActTitle, &textMutedBrush);

            RectF actTrackR((float)scaledActionContainerRect.left, (float)scaledActionContainerRect.top, (float)(scaledActionContainerRect.right - scaledActionContainerRect.left), (float)(scaledActionContainerRect.bottom - scaledActionContainerRect.top));
            GraphicsPath* actTrackP = CreateRoundedPath(actTrackR, actTrackR.Height * 0.5f);
            LinearGradientBrush actTrackB(actTrackR, TRACK_BG_TOP, TRACK_BG_BOT, LinearGradientModeVertical);
            g.FillPath(&actTrackB, actTrackP);
            Pen actTrackRim(Color(170, 220, 228, 236), 1.0f);
            g.DrawPath(&actTrackRim, actTrackP);
            delete actTrackP;

            // Animated Gliding Pill
            float curActX = (animActionX > 0.0f) ? animActionX : (float)scaledActionPillRects[g_state.selectedAction].left;
            float curActW = (animActionW > 0.0f) ? animActionW : (float)(scaledActionPillRects[g_state.selectedAction].right - scaledActionPillRects[g_state.selectedAction].left);
            RectF selActR(curActX + 2.0f, actTrackR.Y + 2.0f, curActW - 4.0f, actTrackR.Height - 4.0f);
            DrawLiquidBluePill(g, selActR, selActR.Height * 0.5f, false, false);

            const wchar_t* actionLabels[3] = { L"Mouse Move", L"Key Press", L"Mouse Click" };
            for (int i = 0; i < 3; i++) {
                RectF pillR((float)scaledActionPillRects[i].left, (float)scaledActionPillRects[i].top, (float)(scaledActionPillRects[i].right - scaledActionPillRects[i].left), (float)(scaledActionPillRects[i].bottom - scaledActionPillRects[i].top));
                bool isSel = (g_state.selectedAction == i);
                g.DrawString(actionLabels[i], -1, &fontPill, pillR, &centerFormat, isSel ? &textPrimaryBrush : &textSecondaryBrush);
            }

            // 5. Schedule Automation - Sliding Segmented Glass Capsule
            PointF ptSchTitle((float)Scale(20), (float)Scale(290));
            g.DrawString(L"SCHEDULE AUTOMATION", -1, &fontSection, ptSchTitle, &textMutedBrush);

            RectF schTrackR((float)scaledScheduleContainerRect.left, (float)scaledScheduleContainerRect.top, (float)(scaledScheduleContainerRect.right - scaledScheduleContainerRect.left), (float)(scaledScheduleContainerRect.bottom - scaledScheduleContainerRect.top));
            GraphicsPath* schTrackP = CreateRoundedPath(schTrackR, schTrackR.Height * 0.5f);
            LinearGradientBrush schTrackB(schTrackR, TRACK_BG_TOP, TRACK_BG_BOT, LinearGradientModeVertical);
            g.FillPath(&schTrackB, schTrackP);
            Pen schTrackRim(Color(170, 220, 228, 236), 1.0f);
            g.DrawPath(&schTrackRim, schTrackP);
            delete schTrackP;

            float curSchX = (animScheduleX > 0.0f) ? animScheduleX : (float)scaledSchedulePillRects[g_state.selectedSchedule].left;
            float curSchW = (animScheduleW > 0.0f) ? animScheduleW : (float)(scaledSchedulePillRects[g_state.selectedSchedule].right - scaledSchedulePillRects[g_state.selectedSchedule].left);
            RectF selSchR(curSchX + 2.0f, schTrackR.Y + 2.0f, curSchW - 4.0f, schTrackR.Height - 4.0f);
            DrawLiquidBluePill(g, selSchR, selSchR.Height * 0.5f, false, false);

            const wchar_t* schedLabels[3] = { L"Work Hours", L"Always On", L"Custom" };
            for (int i = 0; i < 3; i++) {
                RectF pillR((float)scaledSchedulePillRects[i].left, (float)scaledSchedulePillRects[i].top, (float)(scaledSchedulePillRects[i].right - scaledSchedulePillRects[i].left), (float)(scaledSchedulePillRects[i].bottom - scaledSchedulePillRects[i].top));
                bool isSel = (g_state.selectedSchedule == i);
                g.DrawString(schedLabels[i], -1, &fontPill, pillR, &centerFormat, isSel ? &textPrimaryBrush : &textSecondaryBrush);
            }

            // 6. Activity Interval Slider
            PointF ptIntTitle((float)Scale(20), (float)Scale(356));
            g.DrawString(L"ACTIVITY INTERVAL", -1, &fontSection, ptIntTitle, &textMutedBrush);

            wchar_t szSliderVal[32];
            _snwprintf(szSliderVal, ARRAYSIZE(szSliderVal), L"Every %ds", g_state.sliderInterval);
            RectF intValR((float)Scale(240), (float)Scale(354), (float)Scale(102), (float)Scale(18));
            StringFormat rightFmt;
            rightFmt.SetAlignment(StringAlignmentFar);
            rightFmt.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(szSliderVal, -1, &fontPill, intValR, &rightFmt, &textPrimaryBrush);

            // Slider Track
            float trackX = (float)scaledSliderRect.left + (float)Scale(8);
            float trackY = (float)Scale(380);
            float trackW = (float)(scaledSliderRect.right - scaledSliderRect.left) - (float)Scale(16);
            float trackH = (float)Scale(12);
            RectF sliderTrackR(trackX, trackY, trackW, trackH);
            GraphicsPath* sliderTrackP = CreateRoundedPath(sliderTrackR, trackH * 0.5f);
            LinearGradientBrush sliderTrackB(sliderTrackR, TRACK_BG_TOP, TRACK_BG_BOT, LinearGradientModeVertical);
            g.FillPath(&sliderTrackB, sliderTrackP);
            Pen sliderTrackRim(Color(170, 220, 228, 236), 1.0f);
            g.DrawPath(&sliderTrackRim, sliderTrackP);

            // Active Progress Fill
            float sRatio = (float)(g_state.sliderInterval - 15) / (float)(120 - 15);
            float knobX = trackX + (sRatio * trackW);
            if (knobX > trackX + 3.0f) {
                RectF fillR(trackX, trackY, knobX - trackX, trackH);
                GraphicsPath* fillP = CreateRoundedPath(fillR, trackH * 0.5f);
                Region clipReg(sliderTrackP);
                g.SetClip(&clipReg, CombineModeReplace);
                SolidBrush progBrush(Color(240, 120, 165, 235));
                g.FillPath(&progBrush, fillP);
                g.ResetClip();
                delete fillP;
            }
            delete sliderTrackP;

            // 3D Circular Pebble Knob with Spring Scaling
            float baseKnobDiam = (float)Scale(22);
            float knobDiam = baseKnobDiam * animKnobScale;
            RectF knobR(knobX - (knobDiam * 0.5f), trackY + (trackH * 0.5f) - (knobDiam * 0.5f), knobDiam, knobDiam);

            DrawSoftDropShadow(g, knobR, knobDiam * 0.5f, 6.0f, 2.5f, 24);

            GraphicsPath* knobP = new GraphicsPath();
            knobP->AddEllipse(knobR);
            LinearGradientBrush knobBrush(knobR, Color(255, 255, 255, 255), Color(255, 225, 230, 238), LinearGradientModeVertical);
            g.FillPath(&knobBrush, knobP);
            Pen knobRim(Color(235, 255, 255, 255), 1.2f);
            g.DrawPath(&knobRim, knobP);
            delete knobP;

            // Slider Range Sub-captions
            PointF ptMin((float)Scale(20), (float)Scale(396));
            g.DrawString(L"15s (Faster)", -1, &fontStatLbl, ptMin, &textMutedBrush);
            PointF ptMax((float)Scale(280), (float)Scale(396));
            g.DrawString(L"120s (Slower)", -1, &fontStatLbl, ptMax, &textMutedBrush);

            // 7. Footer Toggle Row: "Launch at Windows startup" + 3D Toggle Switch
            RectF loginRowR((float)scaledToggleLoginRect.left, (float)scaledToggleLoginRect.top, (float)(scaledToggleLoginRect.right - scaledToggleLoginRect.left), (float)(scaledToggleLoginRect.bottom - scaledToggleLoginRect.top));
            DrawFrostedGlassCard(g, loginRowR, 14.0f, (hoveredControl == 11), true);

            PointF ptLoginTitle(loginRowR.X + (float)Scale(14), loginRowR.Y + (float)Scale(8));
            g.DrawString(L"Launch at Windows startup", -1, &fontBtn, ptLoginTitle, &textPrimaryBrush);

            PointF ptLoginSub(loginRowR.X + (float)Scale(14), loginRowR.Y + (float)Scale(25));
            g.DrawString(L"Runs silently in tray when PC turns on", -1, &fontStatLbl, ptLoginSub, &textMutedBrush);

            // 3D Glass Toggle Switch on right
            float swW = (float)Scale(46);
            float swH = (float)Scale(24);
            RectF switchR(loginRowR.GetRight() - swW - (float)Scale(14), loginRowR.Y + (loginRowR.Height - swH) * 0.5f, swW, swH);
            DrawGlassToggleSwitch(g, switchR, animToggleLogin, (hoveredControl == 11));

            // 8. Elevated Warning Chip OR Native Protected Watermark
            if (isElevatedWarning) {
                RectF warnR((float)scaledWarningChipRect.left, (float)scaledWarningChipRect.top, (float)(scaledWarningChipRect.right - scaledWarningChipRect.left), (float)(scaledWarningChipRect.bottom - scaledWarningChipRect.top));
                GraphicsPath* warnP = CreateRoundedPath(warnR, 12.0f);
                LinearGradientBrush warnB(warnR, AMBER_WARN_TOP, AMBER_WARN_BOT, LinearGradientModeVertical);
                g.FillPath(&warnB, warnP);
                Pen warnRim(Color(255, 253, 230, 138), 1.2f);
                g.DrawPath(&warnRim, warnP);
                delete warnP;

                PointF ptWarn(warnR.X + (float)Scale(12), warnR.Y + (float)Scale(8));
                g.DrawString(L"⚠ Admin window detected", -1, &fontSub, ptWarn, &textLightBrush);

                RectF btnAdminR(warnR.GetRight() - (float)Scale(130), warnR.Y, (float)Scale(120), warnR.Height);
                g.DrawString(L"[ Relaunch Admin ]", -1, &fontPill, btnAdminR, &centerFormat, &textLightBrush);
            } else {
                RectF botTagR((float)Scale(18), (float)Scale(476), (float)Scale(324), (float)Scale(20));
                g.DrawString(L"Movesi • Native Hardware-Level Input Simulation", -1, &fontStatLbl, botTagR, &centerFormat, &textMutedBrush);
            }
        }

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
        break;
    }

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

HICON CreateDynamicIcon(COLORREF color, IconState state) {
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

    if (state == ICON_STATE_PAUSED) {
        HPEN hRingPen = CreatePen(PS_SOLID, 2, color);
        HBRUSH hRingBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HPEN hOldPen = (HPEN)SelectObject(hMemDC, hRingPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, hRingBrush);

        Ellipse(hMemDC, 1, 1, size - 1, size - 1);

        SelectObject(hMemDC, hOldPen);
        SelectObject(hMemDC, hOldBrush);
        DeleteObject(hRingPen);
    }
    else if (state == ICON_STATE_WARNING) {
        HBRUSH hBrush = CreateSolidBrush(color);
        HPEN hPen = CreatePen(PS_SOLID, 1, color);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);
        HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);

        Ellipse(hMemDC, 1, 1, size - 1, size - 1);

        SelectObject(hMemDC, hOldBrush);
        SelectObject(hMemDC, hOldPen);
        DeleteObject(hBrush);
        DeleteObject(hPen);

        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(255, 255, 255));
        HFONT hFont = CreateFontW(-MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);
        RECT txtRect = { 0, 0, size, size };
        DrawTextW(hMemDC, L"!", 1, &txtRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hMemDC, hOldFont);
        DeleteObject(hFont);
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

    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hdc);

    HBITMAP hMask = CreateBitmap(size, size, 1, 1, NULL);
    HDC hMaskDC = CreateCompatibleDC(NULL);
    HBITMAP hOldMask = (HBITMAP)SelectObject(hMaskDC, hMask);

    HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hMaskDC, &rect, hWhiteBrush);
    DeleteObject(hWhiteBrush);

    HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
    HPEN hBlackPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HBRUSH hOldMB = (HBRUSH)SelectObject(hMaskDC, hBlackBrush);
    HPEN hOldMP = (HPEN)SelectObject(hMaskDC, hBlackPen);

    Ellipse(hMaskDC, 1, 1, size - 1, size - 1);

    SelectObject(hMaskDC, hOldMB);
    SelectObject(hMaskDC, hOldMP);
    DeleteObject(hBlackBrush);
    DeleteObject(hBlackPen);
    SelectObject(hMaskDC, hOldMask);
    DeleteDC(hMaskDC);

    ICONINFO ii = { 0 };
    ii.fIcon = TRUE;
    ii.hbmMask = hMask;
    ii.hbmColor = hBitmap;

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hBitmap);
    DeleteObject(hMask);

    return hIcon;
}

void UpdateTrayIcon() {
    if (isElevatedWarning) {
        nid.hIcon = hIconWarning;
        wcsncpy(nid.szTip, L"Movesi - Admin Mode Required", ARRAYSIZE(nid.szTip) - 1);
    }
    else if (isActive) {
        nid.hIcon = hIconActive;
        _snwprintf(nid.szTip, ARRAYSIZE(nid.szTip), L"Next action in %ds", secondsRemaining);
    }
    else {
        nid.hIcon = hIconPaused;
        wcsncpy(nid.szTip, L"Movesi - Paused", ARRAYSIZE(nid.szTip) - 1);
    }
    nid.szTip[ARRAYSIZE(nid.szTip) - 1] = L'\0';
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void PerformSimulationAction() {
    if (g_state.selectedAction == 0) { // Mouse Move
        int dx = (rand() % 11) + 10;
        int dy = (rand() % 11) + 10;
        if (rand() % 2) dx = -dx;
        if (rand() % 2) dy = -dy;

        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = dx;
        input.mi.dy = dy;

        SendInput(1, &input, sizeof(INPUT));
    }
    else if (g_state.selectedAction == 1) { // Key Press
        INPUT inputs[2] = { 0 };
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_F15;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_F15;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(2, inputs, sizeof(INPUT));
    }
    else if (g_state.selectedAction == 2) { // Mouse Click
        INPUT inputs[2] = { 0 };
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

        SendInput(2, inputs, sizeof(INPUT));
    }
}

void CheckSchedules(HWND hWnd) {
    SYSTEMTIME lt;
    GetLocalTime(&lt);

    bool shouldPause = false;
    bool shouldResume = false;

    if (g_state.selectedSchedule == 0) { // Work Hours
        bool isWeekend = (lt.wDayOfWeek == 0 || lt.wDayOfWeek == 6);
        bool isWorkHours = (lt.wHour >= 9 && lt.wHour < 17);
        if (isWeekend || !isWorkHours) {
            shouldPause = true;
        } else {
            shouldResume = true;
        }
    }
    else if (g_state.selectedSchedule == 2) { // Custom Time Range
        bool inRange = (lt.wHour >= g_state.customStartHour && lt.wHour < g_state.customEndHour);
        if (!inRange) {
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
    _snwprintf(buffer, bufferSize, L"%dh %dm", h, m);
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
        secondsRemaining = g_state.sliderInterval;
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
    } else {
        if (sessionStartTime != 0) {
            g_state.accumulatedTimeActive += (int)(time(NULL) - sessionStartTime);
            sessionStartTime = 0;
        }
        SetThreadExecutionState(ES_CONTINUOUS);
    }
    UpdateTrayIcon();
}
