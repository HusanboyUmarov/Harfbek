// Ekrandagi qisqa bildirishnoma (OSD): fokus olmaydigan, shaffof "pill".
// Per-pixel alpha (UpdateLayeredWindow): yumaloq chekka, yumshoq soya va matn alpha-to'g'ri.
// Pill piksel hisobi bilan (SDF) chiziladi — GDI+ yuklanmaydi; matn/glif GDI niqobi orqali
// aralashtiriladi (oddiy GDI matn alpha'ni buzadi). Oyna faqat ko'rsatilganda yaratiladi,
// yo'qolgach DestroyWindow + DIB o'chiriladi (RAM).
#include "app.h"
#include "ui_gfx.h"
#include <math.h>

static const wchar_t* kOsdClass = L"HarfbekOsd";
static const UINT_PTR kHoldTimer = 1;
static const UINT     kHoldMs    = 1500;
static const DWORD    kInMs      = 180;
static const DWORD    kOutMs     = 200;
static const DWORD    kPulseMs   = 160;

enum OsdPhase { PH_IN, PH_HOLD, PH_OUT };

static HWND    s_hOsd = NULL;
static HBITMAP s_bmp = NULL;
static DWORD*  s_bits = NULL;
static int     s_w = 0, s_h = 0;      // oyna (soya chekkasi bilan) o'lchami
static int     s_x = 0, s_y = 0;      // oyna tinch holatdagi joyi
static int     s_dpi = 96;
static OsdPhase s_phase = PH_IN;
static DWORD   s_t0 = 0;              // joriy fazaning boshlanishi
static DWORD   s_pulse0 = 0;          // oxirgi qayta chaqiruv (pulse) vaqti; 0 = yo'q
static bool    s_animTimer = false;
static wchar_t s_text[128];

struct OsdRgbf { float r, g, b, a; };    // premultiplied

static float OsdClamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
static float OsdEaseOut(float t) { float u = 1.f - OsdClamp01(t); return 1.f - u * u * u; }

// d ustiga (rang, alpha) ni "over" qoidasi bilan qo'yadi
static void OsdOver(OsdRgbf& d, COLORREF c, float a)
{
    float k = 1.f - a;
    d.r = GetRValue(c) / 255.f * a + d.r * k;
    d.g = GetGValue(c) / 255.f * a + d.g * k;
    d.b = GetBValue(c) / 255.f * a + d.b * k;
    d.a = a + d.a * k;
}

// Yumaloq to'g'ri to'rtburchakning (markazga nisbatan) imzoli masofasi
static float OsdSdBox(float px, float py, float hw, float hh, float r)
{
    float qx = fabsf(px) - (hw - r), qy = fabsf(py) - (hh - r);
    float ox = qx > 0.f ? qx : 0.f, oy = qy > 0.f ? qy : 0.f;
    float in = (qx > qy ? qx : qy);
    if (in > 0.f) in = 0.f;
    return sqrtf(ox * ox + oy * oy) + in - r;
}

// GDI bilan niqobga (oq/qora) chizilgan matn/glifni DIB ustiga rangda aralashtiradi
static void CompositeMask(const RECT& rc, const wchar_t* s, HFONT font, UINT flags, COLORREF col)
{
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = s_w;
    bi.bmiHeader.biHeight = -s_h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    DWORD* m = NULL;
    HDC dc = CreateCompatibleDC(NULL);
    HBITMAP mb = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, (void**)&m, NULL, 0);
    if (!mb || !m) { if (mb) DeleteObject(mb); DeleteDC(dc); return; }
    HGDIOBJ ob = SelectObject(dc, mb);
    HGDIOBJ of = SelectObject(dc, font);
    ZeroMemory(m, (size_t)s_w * s_h * 4);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    RECT r = rc;
    DrawTextW(dc, s, -1, &r, flags);
    GdiFlush();

    float cr = GetRValue(col), cg = GetGValue(col), cb = GetBValue(col);
    for (int y = rc.top; y < rc.bottom && y < s_h; ++y)
    {
        if (y < 0) continue;
        for (int x = rc.left; x < rc.right && x < s_w; ++x)
        {
            if (x < 0) continue;
            float c = ((m[(size_t)y * s_w + x] >> 8) & 0xFF) / 255.f;   // G kanali = qoplama
            if (c <= 0.f) continue;
            DWORD d = s_bits[(size_t)y * s_w + x];
            float a = (float)((d >> 24) & 0xFF), k = 1.f - c;
            float nr = ((d >> 16) & 0xFF) * k + cr * c;
            float ng = ((d >> 8) & 0xFF) * k + cg * c;
            float nb = (d & 0xFF) * k + cb * c;
            float na = a + (255.f - a) * c;
            s_bits[(size_t)y * s_w + x] = ((DWORD)(na + 0.5f) << 24) | ((DWORD)(nr + 0.5f) << 16) |
                                          ((DWORD)(ng + 0.5f) << 8) | (DWORD)(nb + 0.5f);
        }
    }
    SelectObject(dc, of);
    SelectObject(dc, ob);
    DeleteObject(mb);
    DeleteDC(dc);
}

static void FreeBitmap()
{
    if (s_bmp) { DeleteObject(s_bmp); s_bmp = NULL; }
    s_bits = NULL;
}

// Pillni DIB ga chizadi (s_w, s_h, s_dpi, s_text bo'yicha)
static bool RenderPill(int margin, int pillW, int pillH, const RECT& textRc, const RECT& glyphRc,
                       int circleR, int circleCx, int circleCy, HFONT font, HFONT iconFont)
{
    FreeBitmap();
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = s_w;
    bi.bmiHeader.biHeight = -s_h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    HDC sdc = GetDC(NULL);
    s_bmp = CreateDIBSection(sdc, &bi, DIB_RGB_COLORS, (void**)&s_bits, NULL, 0);
    ReleaseDC(NULL, sdc);
    if (!s_bmp || !s_bits) { FreeBitmap(); return false; }

    const UiTheme& th = Ui_Theme();
    COLORREF fill   = th.dark ? RGB(22, 22, 26) : RGB(252, 252, 254);
    float    fillA  = 0.94f;
    float    shMax  = th.dark ? 0.55f : 0.30f;
    float    bw     = (float)Ui_S(1, s_dpi);
    float    blur   = (float)Ui_S(14, s_dpi);
    float    shDy   = (float)Ui_S(4, s_dpi);

    float cx = s_w * 0.5f, cy = margin + pillH * 0.5f;
    float hw = pillW * 0.5f, hh = pillH * 0.5f, rad = hh;

    for (int y = 0; y < s_h; ++y)
    {
        for (int x = 0; x < s_w; ++x)
        {
            float px = x + 0.5f - cx, py = y + 0.5f - cy;
            OsdRgbf p = { 0, 0, 0, 0 };

            // yumshoq soya (pastga siljigan)
            float ds = OsdSdBox(px, py - shDy, hw, hh, rad);
            if (ds < blur)
            {
                float t = ds <= 0.f ? 1.f : 1.f - ds / blur;
                OsdOver(p, RGB(0, 0, 0), shMax * t * t);
            }

            // pill: shisha fon + ingichka chegara
            float d = OsdSdBox(px, py, hw, hh, rad);
            float cov = OsdClamp01(0.5f - d);
            if (cov > 0.f)
            {
                OsdOver(p, fill, fillA * cov);
                float inner = OsdClamp01(0.5f - (d + bw));
                float ring = cov - inner;
                if (ring > 0.f) OsdOver(p, th.border, 0.9f * ring);
            }

            // chapdagi accent doira
            float dc = sqrtf((x + 0.5f - circleCx) * (x + 0.5f - circleCx) +
                             (y + 0.5f - circleCy) * (y + 0.5f - circleCy)) - circleR;
            float cc = OsdClamp01(0.5f - dc);
            if (cc > 0.f) OsdOver(p, th.accent, cc);

            DWORD A = (DWORD)(OsdClamp01(p.a) * 255.f + 0.5f);
            DWORD R = (DWORD)(OsdClamp01(p.r) * 255.f + 0.5f);
            DWORD G = (DWORD)(OsdClamp01(p.g) * 255.f + 0.5f);
            DWORD B = (DWORD)(OsdClamp01(p.b) * 255.f + 0.5f);
            s_bits[(size_t)y * s_w + x] = (A << 24) | (R << 16) | (G << 8) | B;
        }
    }

    CompositeMask(glyphRc, L"\xE765", iconFont, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, th.onAccent);
    CompositeMask(textRc, s_text, font, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS, th.text);
    return true;
}

// Joriy fazaga mos alpha/siljish bilan oynani yangilaydi
static void Present()
{
    if (!s_hOsd || !s_bmp) return;
    DWORD now = GetTickCount();
    float k = 1.f;
    int yoff = 0;
    if (s_phase == PH_IN)
    {
        float e = OsdEaseOut((float)(now - s_t0) / kInMs);
        k = e;
        yoff = (int)((1.f - e) * Ui_S(10, s_dpi));
    }
    else if (s_phase == PH_OUT)
    {
        k = 1.f - OsdEaseOut((float)(now - s_t0) / kOutMs);
    }
    if (s_pulse0)
    {
        float pt = OsdClamp01((float)(now - s_pulse0) / kPulseMs);
        k *= 0.72f + 0.28f * pt;
    }

    HDC sdc = GetDC(NULL);
    HDC mdc = CreateCompatibleDC(sdc);
    HGDIOBJ ob = SelectObject(mdc, s_bmp);
    POINT dst = { s_x, s_y + yoff }, src = { 0, 0 };
    SIZE sz = { s_w, s_h };
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)(OsdClamp01(k) * 255.f + 0.5f), AC_SRC_ALPHA };
    UpdateLayeredWindow(s_hOsd, sdc, &dst, &sz, mdc, &src, 0, &bf, ULW_ALPHA);
    SelectObject(mdc, ob);
    DeleteDC(mdc);
    ReleaseDC(NULL, sdc);
}

static void StartAnimTimer()
{
    if (s_hOsd && !s_animTimer) { SetTimer(s_hOsd, UI_ANIM_TIMER, 15, NULL); s_animTimer = true; }
}

static void StopAnimTimer()
{
    if (s_hOsd && s_animTimer) { KillTimer(s_hOsd, UI_ANIM_TIMER); s_animTimer = false; }
}

static void OnAnimTick()
{
    DWORD now = GetTickCount();
    if (s_pulse0 && now - s_pulse0 >= kPulseMs) s_pulse0 = 0;

    if (s_phase == PH_IN && now - s_t0 >= kInMs)
    {
        s_phase = PH_HOLD;
        SetTimer(s_hOsd, kHoldTimer, kHoldMs, NULL);
    }
    else if (s_phase == PH_OUT && now - s_t0 >= kOutMs)
    {
        DestroyWindow(s_hOsd);
        return;
    }
    Present();
    if (s_phase == PH_HOLD && !s_pulse0) StopAnimTimer();
}

static LRESULT CALLBACK OsdProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_TIMER:
        if (wp == UI_ANIM_TIMER) { OnAnimTick(); return 0; }
        if (wp == kHoldTimer)
        {
            KillTimer(h, kHoldTimer);
            if (!Ui_AnimationsEnabled()) { DestroyWindow(h); return 0; }
            s_phase = PH_OUT;
            s_t0 = GetTickCount();
            StartAnimTimer();
        }
        return 0;

    case WM_DESTROY:
        KillTimer(h, kHoldTimer);
        KillTimer(h, UI_ANIM_TIMER);
        s_animTimer = false;
        FreeBitmap();
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

    HDC sdc = GetDC(NULL);
    s_dpi = GetDeviceCaps(sdc, LOGPIXELSY);
    HFONT font = Ui_Font(s_dpi, 17, FW_SEMIBOLD);
    HFONT iconFont = Ui_IconFont(s_dpi, 15);
    HGDIOBJ old = SelectObject(sdc, font);
    RECT tr = { 0, 0, 0, 0 };
    DrawTextW(sdc, s_text, -1, &tr, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(sdc, old);
    ReleaseDC(NULL, sdc);

    // Joylashuv: kursor turgan monitorning ish maydoni, pastki markaz, pastdan ~10% yuqorida
    POINT pt;
    GetCursorPos(&pt);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), &mi);
    const RECT& wa = mi.rcWork;

    int margin  = Ui_S(18, s_dpi);                  // soya uchun chekka
    int pillH   = Ui_S(48, s_dpi);
    int circleD = Ui_S(28, s_dpi);
    int padL    = Ui_S(10, s_dpi), gap = Ui_S(12, s_dpi), padR = Ui_S(20, s_dpi);
    int textW   = tr.right - tr.left;
    int pillW   = padL + circleD + gap + textW + padR;
    int maxW    = (wa.right - wa.left) - 2 * margin;
    if (pillW > maxW) { textW -= pillW - maxW; pillW = maxW; }
    if (textW < 0) textW = 0;

    s_w = pillW + 2 * margin;
    s_h = pillH + 2 * margin;
    int pillX = wa.left + ((wa.right - wa.left) - pillW) / 2;
    int pillY = wa.bottom - pillH - (wa.bottom - wa.top) / 10;
    s_x = pillX - margin;
    s_y = pillY - margin;

    int circleCx = margin + padL + circleD / 2;
    int circleCy = margin + pillH / 2;
    RECT glyphRc = { circleCx - circleD / 2, circleCy - circleD / 2,
                     circleCx + circleD / 2, circleCy + circleD / 2 };
    RECT textRc  = { margin + padL + circleD + gap, margin, margin + padL + circleD + gap + textW, margin + pillH };

    bool ok = RenderPill(margin, pillW, pillH, textRc, glyphRc, circleD / 2, circleCx, circleCy, font, iconFont);
    DeleteObject(font);
    DeleteObject(iconFont);
    if (!ok) return;

    bool anim = Ui_AnimationsEnabled();
    DWORD now = GetTickCount();

    if (!s_hOsd)
    {
        s_hOsd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            kOsdClass, L"", WS_POPUP, s_x, s_y, s_w, s_h, NULL, NULL, g_hInst, NULL);
        if (!s_hOsd) { FreeBitmap(); return; }
        s_phase = anim ? PH_IN : PH_HOLD;
        s_t0 = now;
        s_pulse0 = 0;
        Present();
        ShowWindow(s_hOsd, SW_SHOWNOACTIVATE);
        if (anim) StartAnimTimer();
        else SetTimer(s_hOsd, kHoldTimer, kHoldMs, NULL);
        return;
    }

    // Qayta chaqiruv: matn yangilanadi, yengil pulse, taymer qayta boshlanadi
    SetWindowPos(s_hOsd, HWND_TOPMOST, s_x, s_y, s_w, s_h, SWP_NOACTIVATE | SWP_NOREDRAW);
    if (s_phase == PH_OUT) { s_phase = PH_HOLD; s_t0 = now; }
    if (s_phase == PH_HOLD)
    {
        KillTimer(s_hOsd, kHoldTimer);
        SetTimer(s_hOsd, kHoldTimer, kHoldMs, NULL);
    }
    if (anim) { s_pulse0 = now; StartAnimTimer(); }
    Present();
}
