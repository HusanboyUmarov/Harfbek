// Ekrandagi qisqa bildirishnoma (OSD): fokus olmaydigan popup.
// Oyna faqat ko'rsatilganda yaratiladi, yashirilganda DestroyWindow + shrift o'chiriladi (RAM).
#include "app.h"

static const wchar_t* kOsdClass = L"HarfbekOsd";
static const UINT_PTR kOsdTimer = 1;
static const UINT     kOsdMs    = 1500;
static const BYTE     kOsdAlpha = 230;

static HWND    s_hOsd = NULL;
static HFONT   s_font = NULL;
static wchar_t s_text[128];

static LRESULT CALLBACK OsdProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc;
        GetClientRect(h, &rc);
        SetDCBrushColor(dc, RGB(32, 32, 36));
        FillRect(dc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
        HGDIOBJ old = SelectObject(dc, s_font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255, 255, 255));
        DrawTextW(dc, s_text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, old);
        EndPaint(h, &ps);
        return 0;
    }

    case WM_TIMER:
        if (wp == kOsdTimer) DestroyWindow(h);
        return 0;

    case WM_DESTROY:
        KillTimer(h, kOsdTimer);
        if (s_font) { DeleteObject(s_font); s_font = NULL; }
        s_hOsd = NULL;
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

void Osd_Show(const wchar_t* text)
{
    static bool s_registered = false;
    if (!s_registered)
    {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc   = OsdProc;
        wc.hInstance     = g_hInst;
        wc.lpszClassName = kOsdClass;
        if (!RegisterClassExW(&wc)) return;
        s_registered = true;
    }

    lstrcpynW(s_text, text ? text : L"", (int)(sizeof(s_text) / sizeof(s_text[0])));

    // Shrift: Segoe UI ~15pt, system DPI bo'yicha masshtab
    HDC sdc = GetDC(NULL);
    int dpi = GetDeviceCaps(sdc, LOGPIXELSY);
    if (s_font) DeleteObject(s_font);
    s_font = CreateFontW(-MulDiv(15, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HGDIOBJ old = SelectObject(sdc, s_font);
    RECT tr = { 0, 0, 0, 0 };
    DrawTextW(sdc, s_text, -1, &tr, DT_CALCRECT | DT_SINGLELINE);
    SelectObject(sdc, old);
    ReleaseDC(NULL, sdc);

    // Joylashuv: kursor turgan monitorning ish maydoni, pastki markaz, pastdan ~10% yuqorida
    POINT pt;
    GetCursorPos(&pt);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), &mi);
    const RECT& wa = mi.rcWork;

    int w = (tr.right - tr.left) + 2 * MulDiv(22, dpi, 96);
    int h = (tr.bottom - tr.top) + 2 * MulDiv(12, dpi, 96);
    if (w > wa.right - wa.left) w = wa.right - wa.left;
    int x = wa.left + ((wa.right - wa.left) - w) / 2;
    int y = wa.bottom - h - (wa.bottom - wa.top) / 10;

    if (!s_hOsd)
    {
        s_hOsd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            kOsdClass, L"", WS_POPUP, x, y, w, h, NULL, NULL, g_hInst, NULL);
        if (!s_hOsd)
        {
            if (s_font) { DeleteObject(s_font); s_font = NULL; }
            return;
        }
        SetLayeredWindowAttributes(s_hOsd, 0, kOsdAlpha, LWA_ALPHA);
    }

    // Yumaloq burchak (region oynaga o'tadi — o'chirish shart emas)
    int r = MulDiv(24, dpi, 96);
    SetWindowRgn(s_hOsd, CreateRoundRectRgn(0, 0, w + 1, h + 1, r, r), FALSE);
    SetWindowPos(s_hOsd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
    ShowWindow(s_hOsd, SW_SHOWNOACTIVATE);
    InvalidateRect(s_hOsd, NULL, TRUE);

    SetTimer(s_hOsd, kOsdTimer, kOsdMs, NULL);   // qayta chaqiruv taymerni qayta boshlaydi
}
