#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "stb_image.h"

struct Image {
    int w, h;
    std::vector<unsigned char> pixels; // BGRA
};

static Image gImg;
static bool gHasImage = false;
static double gZoom = 1.0;
static int gPanX = 0, gPanY = 0;
static bool gDrag = false;
static POINT gLast;
static RECT gView;

void CenterImage() {
    int vw = gView.right - gView.left;
    int vh = gView.bottom - gView.top;
    gPanX = (vw - int(gImg.w * gZoom)) / 2;
    gPanY = (vh - int(gImg.h * gZoom)) / 2;
}

bool LoadImage(const char* path) {
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 4);
    if (!data) return false;

    gImg.w = w;
    gImg.h = h;
    gImg.pixels.assign(data, data + w * h * 4);
    stbi_image_free(data);

    gZoom = 1.0;
    CenterImage();
    gHasImage = true;
    return true;
}

void DrawImage(HDC hdc) {
    if (!gHasImage) return;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = gImg.w;
    bmi.bmiHeader.biHeight = -gImg.h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;

    int sw = int(gImg.w * gZoom);
    int sh = int(gImg.h * gZoom);

    SetStretchBltMode(hdc, COLORONCOLOR);
    StretchDIBits(
        hdc,
        gPanX, gPanY, sw, sh,
        0, 0, gImg.w, gImg.h,
        &gImg.pixels[0],
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_SIZE:
        GetClientRect(h, &gView);
        if (gHasImage) CenterImage();
        break;

    case WM_MOUSEWHEEL:
        gZoom *= (GET_WHEEL_DELTA_WPARAM(w) > 0) ? 1.25 : 0.8;
        if (gZoom < 0.1) gZoom = 0.1;
        if (gZoom > 8.0) gZoom = 8.0;
        InvalidateRect(h, 0, TRUE);
        break;

    case WM_LBUTTONDOWN:
        gDrag = true;
        gLast.x = GET_X_LPARAM(l);
        gLast.y = GET_Y_LPARAM(l);
        SetCapture(h);
        break;

    case WM_LBUTTONUP:
        gDrag = false;
        ReleaseCapture();
        break;

    case WM_MOUSEMOVE:
        if (gDrag) {
            int x = GET_X_LPARAM(l);
            int y = GET_Y_LPARAM(l);
            gPanX += x - gLast.x;
            gPanY += y - gLast.y;
            gLast.x = x;
            gLast.y = y;
            InvalidateRect(h, 0, TRUE);
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        DrawImage(hdc);
        EndPaint(h, &ps);
    } break;

    case WM_LBUTTONDBLCLK: {
        OPENFILENAMEA of = { sizeof(of) };
        char path[MAX_PATH] = {};
        of.lpstrFilter = "Images\0*.jpg;*.png\0";
        of.lpstrFile = path;
        of.nMaxFile = MAX_PATH;
        if (GetOpenFileNameA(&of)) {
            LoadImage(path);
            InvalidateRect(h, 0, TRUE);
        }
    } break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int s) {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = "XP_FAST_VIEWER";
    RegisterClassA(&wc);

    HWND h = CreateWindowA(
        wc.lpszClassName,
        "XP Fast Photo Viewer",
        WS_OVERLAPPEDWINDOW,
        100, 100, 800, 600,
        0, 0, hi, 0
    );

    ShowWindow(h, s);

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
