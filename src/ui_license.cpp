// Litsenziya / ogohlantirish oynasi. Birinchi ishga tushishda va "Dastur haqida"
// dan ochiladi. Matn joriy interfeys tilida (uz/ru/en) o'zimiz chizilgan
// yumaloq kartochkada ko'rsatiladi: g'ildirak, klaviatura va yupqa scroll-bar
// (sudrab ham bo'ladi) bilan silliq aylantiriladi. Taymer faqat animatsiya
// ketayotganda ishlaydi, oyna yopilganda hamma narsa bo'shatiladi.
// Barcha piksel o'lchamlari 96 DPI uchun yozilgan va S() orqali system DPI
// ga masshtablanadi (manifestda dpiAware=true).
#include "app.h"
#include <shellapi.h>

enum { LI_NONE = -1, LI_GH = 0, LI_OK, LI_COUNT };

static const int CW96 = 520;
static const int CH96 = 440;

static int s_dpi = 96;
static int s_cw  = CW96;
static int s_ch  = CH96;

static int S(int x) { return Ui_S(x, s_dpi); }
static int Px1() { int v = S(1); return v > 0 ? v : 1; }

struct LicState {
    int    focus, hot, down;
    bool   kbd, tracking;
    bool   dragging;        // scroll-bar tutqichi sudralmoqda
    int    dragOff;         // sichqoncha tutqich ichida qayerdan ushlagani (px)
    int    contentH;        // matnning to'liq balandligi (px)
    UiAnim hov[LI_COUNT], prs[LI_COUNT], scroll;   // scroll.value — siljish (px)
    HFONT  fTitle, fBody, fBtn;
};

// ---- joylashuv (client koordinata) ----
static RECT CardRect()
{
    RECT r = { S(28), S(84), s_cw - S(28), S(84) + S(270) };
    return r;
}

static RECT TextRect()
{
    RECT c = CardRect();
    RECT r = { c.left + S(22), c.top + S(16), c.right - S(32), c.bottom - S(16) };
    return r;
}

static RECT ItemRect(int it)
{
    int bh = S(40), by = s_ch - bh - S(20);
    RECT r;
    if (it == LI_GH) { r.left = S(28); r.right = S(28) + S(140); }
    else             { r.left = s_cw - S(28) - S(160); r.right = s_cw - S(28); }
    r.top = by; r.bottom = by + bh;
    return r;
}

static int MaxScroll(LicState* st)
{
    RECT t = TextRect();
    int m = st->contentH - (t.bottom - t.top);
    return m > 0 ? m : 0;
}

// scroll-bar yo'li va tutqichi (faqat matn sig'masa)
static void TrackGeom(LicState* st, int* top, int* h, int* thumbTop, int* thumbH)
{
    RECT c = CardRect(), t = TextRect();
    int view = t.bottom - t.top;
    *top = c.top + S(14);
    *h   = (c.bottom - S(14)) - *top;
    int th = st->contentH > 0 ? MulDiv(*h, view, st->contentH) : *h;
    if (th < S(28)) th = S(28);
    if (th > *h) th = *h;
    int mx = MaxScroll(st);
    *thumbH = th;
    *thumbTop = *top + (mx > 0 ? (int)((*h - th) * (st->scroll.value / mx) + 0.5f) : 0);
}

static RECT ThumbHit(LicState* st)    // tutqich atrofidagi qulay (kengaytirilgan) hudud
{
    RECT c = CardRect();
    int top, h, tt, tH;
    TrackGeom(st, &top, &h, &tt, &tH);
    RECT r = { c.right - S(24), tt, c.right - S(4), tt + tH };
    return r;
}

static int Hit(int x, int y)
{
    POINT p = { x, y };
    for (int i = 0; i < LI_COUNT; ++i) {
        RECT r = ItemRect(i);
        if (PtInRect(&r, p)) return i;
    }
    return LI_NONE;
}

static void Kick(HWND hwnd) { SetTimer(hwnd, UI_ANIM_TIMER, 15, NULL); }

static void ScrollTo(HWND hwnd, LicState* st, float v, bool animate)
{
    float mx = (float)MaxScroll(st);
    if (v < 0) v = 0;
    if (v > mx) v = mx;
    if (animate) {
        Ui_AnimTo(st->scroll, v, 150);
        Kick(hwnd);
    } else {
        st->scroll.from = st->scroll.to = st->scroll.value = v;
        st->scroll.running = false;
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

static void SetHot(HWND hwnd, LicState* st, int it)
{
    if (it == st->hot) return;
    if (st->hot >= 0) Ui_AnimTo(st->hov[st->hot], 0.f, 160);
    st->hot = it;
    if (it >= 0) Ui_AnimTo(st->hov[it], 1.f, 160);
    Kick(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

static void Activate(HWND hwnd, int it)
{
    if (it == LI_GH) ShellExecuteW(hwnd, L"open", URL_GITHUB, NULL, NULL, SW_SHOWNORMAL);
    else             DestroyWindow(hwnd);
}

static void PaintAll(HDC dc, LicState* st)
{
    const UiTheme& th = Ui_Theme();
    RECT full = { 0, 0, s_cw, s_ch };
    SetDCBrushColor(dc, th.bg);
    FillRect(dc, &full, (HBRUSH)GetStockObject(DC_BRUSH));

    // sarlavha: ikonka plitkasi + matn
    int tile = S(44);
    RECT tr = { S(28), S(24), S(28) + tile, S(24) + tile };
    Ui_FillRound(dc, tr, S(12), th.accent, th.dark ? 46 : 30);
    int isz = S(32);
    HICON ic = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON),
                                 IMAGE_ICON, isz, isz, LR_DEFAULTCOLOR);
    if (ic) {
        DrawIconEx(dc, tr.left + (tile - isz) / 2, tr.top + (tile - isz) / 2, ic, isz, isz,
                   0, NULL, DI_NORMAL);
        DestroyIcon(ic);
    }
    RECT hr = { tr.right + S(14), tr.top, s_cw - S(28), tr.bottom };
    Ui_Text(dc, T(S_LIC_TITLE), hr, st->fTitle, th.text,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    // matn kartochkasi
    RECT card = CardRect();
    Ui_FillRound(dc, card, S(12), th.surface);
    Ui_StrokeRound(dc, card, S(12), th.border, Px1());

    RECT t = TextRect();
    int off = (int)(st->scroll.value + 0.5f);
    int saved = SaveDC(dc);
    IntersectClipRect(dc, t.left, t.top, t.right, t.bottom);
    RECT dr = { t.left, t.top - off, t.right, t.top - off + st->contentH };
    Ui_Text(dc, T(S_LIC_BODY), dr, st->fBody, th.text, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
    RestoreDC(dc, saved);

    // scroll-bar (faqat matn sig'masa)
    if (MaxScroll(st) > 0) {
        int top, h, tt, tH;
        TrackGeom(st, &top, &h, &tt, &tH);
        RECT thumb = { card.right - S(16), tt, card.right - S(10), tt + tH };
        COLORREF c = Ui_Mix(th.border, th.textDim, st->dragging ? 1.f : 0.6f);
        Ui_FillRound(dc, thumb, S(3), c);
    }

    // tugmalar
    Ui_DrawButton(dc, ItemRect(LI_GH), T(S_ABOUT_GITHUB), st->fBtn, UI_BTN_SECONDARY,
                  st->hov[LI_GH].value, st->prs[LI_GH].value, true,
                  st->kbd && st->focus == LI_GH);
    Ui_DrawButton(dc, ItemRect(LI_OK), T(S_LIC_OK), st->fBtn, UI_BTN_PRIMARY,
                  st->hov[LI_OK].value, st->prs[LI_OK].value, true,
                  st->kbd && st->focus == LI_OK);
}

static LRESULT CALLBACK LicenseProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LicState* st = (LicState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        Ui_Begin();     // GDI+ (topilmasa GDI fallback); WM_NCDESTROY da Ui_End
        st = new LicState();
        st->focus = LI_OK;
        st->hot = st->down = LI_NONE;
        st->fTitle = Ui_Font(s_dpi, 22, 700);
        st->fBody  = Ui_Font(s_dpi, 14, 400);
        st->fBtn   = Ui_Font(s_dpi, 15, 600);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);

        // matnning to'liq balandligini bir marta o'lchaymiz
        RECT t = TextRect();
        RECT mr = { 0, 0, t.right - t.left, 0 };
        HDC dc = GetDC(hwnd);
        if (dc) {
            HGDIOBJ of = SelectObject(dc, st->fBody);
            DrawTextW(dc, T(S_LIC_BODY), -1, &mr, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
            SelectObject(dc, of);
            ReleaseDC(hwnd, dc);
        }
        st->contentH = mr.bottom;
        Ui_ApplyWindowStyle(hwnd);
        return 0;
    }

    case WM_KEYDOWN:
        if (!st) return 0;
        st->kbd = true;
        {
            RECT t = TextRect();
            int view = t.bottom - t.top, line = S(20);
            switch (wp) {
            case VK_ESCAPE: DestroyWindow(hwnd); return 0;
            case VK_RETURN:
            case VK_SPACE:  Activate(hwnd, st->focus); return 0;
            case VK_TAB:    st->focus = st->focus == LI_GH ? LI_OK : LI_GH; break;
            case VK_LEFT:   st->focus = LI_GH; break;
            case VK_RIGHT:  st->focus = LI_OK; break;
            case VK_UP:     ScrollTo(hwnd, st, st->scroll.to - line, true); break;
            case VK_DOWN:   ScrollTo(hwnd, st, st->scroll.to + line, true); break;
            case VK_PRIOR:  ScrollTo(hwnd, st, st->scroll.to - view * 0.9f, true); break;
            case VK_NEXT:   ScrollTo(hwnd, st, st->scroll.to + view * 0.9f, true); break;
            case VK_HOME:   ScrollTo(hwnd, st, 0, true); break;
            case VK_END:    ScrollTo(hwnd, st, (float)MaxScroll(st), true); break;
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_MOUSEWHEEL:
        if (st) {
            int d = GET_WHEEL_DELTA_WPARAM(wp);
            ScrollTo(hwnd, st, st->scroll.to - (float)d * S(20) * 3 / WHEEL_DELTA, true);
        }
        return 0;

    case WM_MOUSEMOVE:
        if (st) {
            int x = (short)LOWORD(lp), y = (short)HIWORD(lp);
            if (st->dragging) {
                int top, h, tt, tH;
                TrackGeom(st, &top, &h, &tt, &tH);
                int room = h - tH;
                if (room > 0)
                    ScrollTo(hwnd, st, (float)MaxScroll(st) * (y - st->dragOff - top) / room, false);
                return 0;
            }
            if (!st->tracking) {
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                st->tracking = true;
            }
            SetHot(hwnd, st, Hit(x, y));
        }
        return 0;

    case WM_MOUSELEAVE:
        if (st) { st->tracking = false; SetHot(hwnd, st, LI_NONE); }
        return 0;

    case WM_SETCURSOR:
        if (st && LOWORD(lp) == HTCLIENT && st->hot >= 0) {
            SetCursor(LoadCursorW(NULL, IDC_HAND));
            return TRUE;
        }
        break;

    case WM_LBUTTONDOWN:
        if (st) {
            int x = (short)LOWORD(lp), y = (short)HIWORD(lp);
            POINT p = { x, y };
            st->kbd = false;
            int it = Hit(x, y);
            if (it >= 0) {
                st->down = it;
                st->focus = it;
                SetCapture(hwnd);
                Ui_AnimTo(st->prs[it], 1.f, 90);
                Kick(hwnd);
            } else if (MaxScroll(st) > 0) {
                RECT th = ThumbHit(st);
                RECT c = CardRect();
                RECT trackR = { c.right - S(24), c.top, c.right - S(4), c.bottom };
                if (PtInRect(&th, p)) {
                    int top, h, tt, tH;
                    TrackGeom(st, &top, &h, &tt, &tH);
                    st->dragging = true;
                    st->dragOff = y - tt;
                    SetCapture(hwnd);
                } else if (PtInRect(&trackR, p)) {
                    RECT t = TextRect();
                    float page = (float)(t.bottom - t.top) * 0.9f;
                    ScrollTo(hwnd, st, st->scroll.to + (y < th.top ? -page : page), true);
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_LBUTTONUP:
        if (st) {
            if (st->dragging) {
                st->dragging = false;
                ReleaseCapture();
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (st->down >= 0) {
                int d = st->down;
                st->down = LI_NONE;
                ReleaseCapture();
                Ui_AnimTo(st->prs[d], 0.f, 140);
                Kick(hwnd);
                int it = Hit((short)LOWORD(lp), (short)HIWORD(lp));
                SetHot(hwnd, st, it);
                if (it == d) Activate(hwnd, d);     // oynani yopishi mumkin — st'ga tegmaymiz
            }
        }
        return 0;

    case WM_TIMER:
        if (st && wp == UI_ANIM_TIMER) {
            bool run = false;
            for (int i = 0; i < LI_COUNT; ++i) {
                if (Ui_AnimStep(st->hov[i])) run = true;
                if (Ui_AnimStep(st->prs[i])) run = true;
            }
            if (Ui_AnimStep(st->scroll)) run = true;
            InvalidateRect(hwnd, NULL, FALSE);
            if (!run) KillTimer(hwnd, UI_ANIM_TIMER);
        }
        return 0;

    case WM_SETTINGCHANGE:
        // tizim mavzusi o'zgarsa (Windows "ImmersiveColorSet" yuboradi) qayta chizamiz
        if (lp && lstrcmpW((LPCWSTR)lp, L"ImmersiveColorSet") != 0) break;
        // fallthrough
    case WM_THEMECHANGED:
        Ui_RefreshTheme();
        Ui_ApplyWindowStyle(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC mem;
        Ui_BeginBuffer(hwnd, &ps, &mem);
        if (st) PaintAll(mem, st);
        Ui_EndBuffer(hwnd, &ps, mem);
        return 0;
    }

    case WM_PRINTCLIENT:    // PrintWindow (vizual tekshiruv stendi) uchun
        if (st) PaintAll((HDC)wp, st);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, UI_ANIM_TIMER);
        return 0;

    case WM_NCDESTROY:
        if (st) {
            DeleteObject(st->fTitle); DeleteObject(st->fBody); DeleteObject(st->fBtn);
            delete st;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            Ui_End();
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void ShowLicense(HWND parent)
{
    static HWND s_open = NULL;      // qayta kirishdan himoya: ikkinchi nusxa ochilmaydi
    if (s_open && IsWindow(s_open)) { SetForegroundWindow(s_open); return; }

    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = LicenseProc;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        wc.hIcon         = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON));
        wc.lpszClassName = L"HarfbekLicenseWnd";
        RegisterClassW(&wc);
        registered = true;
    }

    s_dpi = Ui_Dpi(NULL);
    if (s_dpi <= 0) s_dpi = 96;
    s_cw = S(CW96);
    s_ch = S(CH96);

    DWORD style = WS_CAPTION | WS_SYSMENU;
    RECT rc = { 0, 0, s_cw, s_ch };
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    int ww = rc.right - rc.left, wh = rc.bottom - rc.top;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;

    HWND hwnd = CreateWindowExW(0, L"HarfbekLicenseWnd", T(S_LIC_TITLE),
                                style, sx, sy, ww, wh, parent, NULL, g_hInst, NULL);
    if (!hwnd) return;
    s_open = hwnd;

    BOOL parentWasEnabled = IsWindowEnabled(parent);
    if (parent && parentWasEnabled) EnableWindow(parent, FALSE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    MSG msg;
    while (IsWindow(hwnd) && GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    s_open = NULL;

    if (parent && parentWasEnabled) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }
}
