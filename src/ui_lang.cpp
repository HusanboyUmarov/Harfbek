// Til tanlash oynasi (birinchi ishga tushishda ko'rsatiladi): ikonka, sarlavha
// va uchta katta kartochka (UZ / RU / EN). Hammasi o'zimiz chizamiz; hover va
// fokus animatsiyali, klaviatura bilan to'liq boshqariladi. Taymer faqat
// animatsiya ketayotganda ishlaydi, oyna yopilganda hamma narsa bo'shatiladi.
// Barcha piksel o'lchamlari 96 DPI uchun yozilgan va S() orqali system DPI
// ga masshtablanadi (manifestda dpiAware=true).
#include "app.h"

#define LANG_NONE (-1)

static const int CW96 = 400;
static const int CH96 = 408;

static int s_dpi = 96;
static int s_cw  = CW96;
static int s_ch  = CH96;
static int s_result = -1;      // tanlangan til (Lang) yoki -1 (Esc / X)

static int S(int x) { return Ui_S(x, s_dpi); }

struct LangState {
    int    focus, hot;
    bool   kbd, tracking;
    UiAnim hov[LANG_COUNT], foc[LANG_COUNT];
    HFONT  fTitle, fPrompt, fName, fCode, fIcon;
};

// Til nomlari - o'z tilida (neytral); 2 harfli kod pill'da ko'rsatiladi
static const wchar_t* const kNames[LANG_COUNT] = { L"Özbekça", L"Русский", L"English" };
static const wchar_t* const kCodes[LANG_COUNT] = { L"UZ", L"RU", L"EN" };

static RECT CardRect(int i)
{
    RECT r;
    r.left   = S(40);
    r.right  = s_cw - S(40);
    r.top    = S(184 + i * 70);
    r.bottom = r.top + S(60);
    return r;
}

static int Hit(int x, int y)
{
    POINT p = { x, y };
    for (int i = 0; i < LANG_COUNT; ++i) {
        RECT r = CardRect(i);
        if (PtInRect(&r, p)) return i;
    }
    return LANG_NONE;
}

static void Kick(HWND hwnd) { SetTimer(hwnd, UI_ANIM_TIMER, 15, NULL); }

// fokusni boshqa kartochkaga o'tkazadi (animatsiya bilan)
static void FocusTo(HWND hwnd, LangState* st, int i)
{
    if (i == st->focus) return;
    if (st->focus >= 0) Ui_AnimTo(st->foc[st->focus], 0.f, 160);
    st->focus = i;
    if (i >= 0) Ui_AnimTo(st->foc[i], 1.f, 160);
    Kick(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

static void SetHot(HWND hwnd, LangState* st, int i)
{
    if (i == st->hot) return;
    if (st->hot >= 0) Ui_AnimTo(st->hov[st->hot], 0.f, 160);
    st->hot = i;
    if (i >= 0) { Ui_AnimTo(st->hov[i], 1.f, 160); st->kbd = false; FocusTo(hwnd, st, i); }
    Kick(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

static void Choose(HWND hwnd, int i)
{
    s_result = i;
    DestroyWindow(hwnd);
}

static void PaintAll(HDC dc, LangState* st)
{
    const UiTheme& th = Ui_Theme();
    RECT full = { 0, 0, s_cw, s_ch };
    SetDCBrushColor(dc, th.bg);
    FillRect(dc, &full, (HBRUSH)GetStockObject(DC_BRUSH));

    // ikonka (accent tusli yumaloq plitka ichida)
    int tile = S(72);
    RECT tr = { (s_cw - tile) / 2, S(24), (s_cw + tile) / 2, S(24) + tile };
    Ui_FillRound(dc, tr, S(20), th.accent, th.dark ? 46 : 30);
    int isz = S(48);
    HICON ic = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON),
                                 IMAGE_ICON, isz, isz, LR_DEFAULTCOLOR);
    if (ic) {
        DrawIconEx(dc, (s_cw - isz) / 2, tr.top + (tile - isz) / 2, ic, isz, isz, 0, NULL, DI_NORMAL);
        DestroyIcon(ic);
    }

    // sarlavha va izoh
    RECT r1 = { S(20), S(108), s_cw - S(20), S(138) };
    Ui_Text(dc, T(S_LANG_TITLE), r1, st->fTitle, th.text,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    RECT r2 = { S(20), S(140), s_cw - S(20), S(164) };
    Ui_Text(dc, T(S_LANG_PROMPT), r2, st->fPrompt, th.textDim,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    // kartochkalar
    for (int i = 0; i < LANG_COUNT; ++i) {
        RECT rc = CardRect(i);
        float f = st->foc[i].value, h = st->hov[i].value;
        Ui_DrawCard(dc, rc, false, h, f);

        // 2 harfli kod (pill)
        int cy = (rc.top + rc.bottom) / 2;
        RECT pill = { rc.left + S(16), cy - S(15), rc.left + S(16) + S(48), cy + S(15) };
        Ui_FillRound(dc, pill, S(15), th.accent, (BYTE)(40 + 40 * f));
        Ui_Text(dc, kCodes[i], pill, st->fCode, th.accent,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        // til nomi
        RECT nr = { pill.right + S(16), rc.top, rc.right - S(48), rc.bottom };
        Ui_Text(dc, kNames[i], nr, st->fName, th.text,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

        // chevron (fokusda o'ngga siljiydi va accent bo'ladi)
        RECT cr = { rc.right - S(44) + (int)(S(4) * f), rc.top, rc.right - S(16) + (int)(S(4) * f), rc.bottom };
        Ui_Glyph(dc, L"", cr, st->fIcon, Ui_Mix(th.textDim, th.accent, f));

        if (st->kbd && st->focus == i) {
            RECT fr = rc;
            InflateRect(&fr, S(4), S(4));
            Ui_DrawFocusRing(dc, fr, S(12));
        }
    }
}

static LRESULT CALLBACK LangProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LangState* st = (LangState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        Ui_Begin();     // GDI+ (topilmasa GDI fallback); WM_NCDESTROY da Ui_End
        st = new LangState();
        st->hot   = LANG_NONE;
        st->focus = (g_settings.lang >= 0 && g_settings.lang < LANG_COUNT) ? (int)g_settings.lang : 0;
        st->foc[st->focus].from = st->foc[st->focus].to = st->foc[st->focus].value = 1.f;
        st->fTitle  = Ui_Font(s_dpi, 21, 700);
        st->fPrompt = Ui_Font(s_dpi, 14, 400);
        st->fName   = Ui_Font(s_dpi, 18, 600);
        st->fCode   = Ui_Font(s_dpi, 14, 700);
        st->fIcon   = Ui_IconFont(s_dpi, 16);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);
        Ui_ApplyWindowStyle(hwnd);
        return 0;
    }

    case WM_KEYDOWN:
        if (!st) return 0;
        st->kbd = true;
        switch (wp) {
        case VK_ESCAPE: Choose(hwnd, -1); return 0;
        case VK_RETURN:
        case VK_SPACE:  Choose(hwnd, st->focus); return 0;
        case VK_UP: case VK_LEFT:
            FocusTo(hwnd, st, (st->focus + LANG_COUNT - 1) % LANG_COUNT); break;
        case VK_DOWN: case VK_RIGHT:
            FocusTo(hwnd, st, (st->focus + 1) % LANG_COUNT); break;
        case VK_TAB:
            FocusTo(hwnd, st, (st->focus + (GetKeyState(VK_SHIFT) < 0 ? LANG_COUNT - 1 : 1)) % LANG_COUNT);
            break;
        case VK_HOME: FocusTo(hwnd, st, 0); break;
        case VK_END:  FocusTo(hwnd, st, LANG_COUNT - 1); break;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        if (st) {
            if (!st->tracking) {
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                st->tracking = true;
            }
            SetHot(hwnd, st, Hit((short)LOWORD(lp), (short)HIWORD(lp)));
        }
        return 0;

    case WM_MOUSELEAVE:
        if (st) { st->tracking = false; SetHot(hwnd, st, LANG_NONE); }
        return 0;

    case WM_SETCURSOR:
        if (st && LOWORD(lp) == HTCLIENT && st->hot >= 0) {
            SetCursor(LoadCursorW(NULL, IDC_HAND));
            return TRUE;
        }
        break;

    case WM_LBUTTONDOWN:
        if (st) {
            int i = Hit((short)LOWORD(lp), (short)HIWORD(lp));
            if (i >= 0) { st->focus = i; SetCapture(hwnd); }
        }
        return 0;

    case WM_LBUTTONUP:
        if (st && GetCapture() == hwnd) {
            ReleaseCapture();
            int i = Hit((short)LOWORD(lp), (short)HIWORD(lp));
            if (i >= 0 && i == st->focus) Choose(hwnd, i);
        }
        return 0;

    case WM_TIMER:
        if (st && wp == UI_ANIM_TIMER) {
            bool run = false;
            for (int i = 0; i < LANG_COUNT; ++i) {
                if (Ui_AnimStep(st->hov[i])) run = true;
                if (Ui_AnimStep(st->foc[i])) run = true;
            }
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
            DeleteObject(st->fTitle); DeleteObject(st->fPrompt); DeleteObject(st->fName);
            DeleteObject(st->fCode);  DeleteObject(st->fIcon);
            delete st;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            Ui_End();
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void ShowLanguageDialog(HWND parent)
{
    static HWND s_open = NULL;      // qayta kirishdan himoya: ikkinchi nusxa ochilmaydi
    if (s_open && IsWindow(s_open)) { SetForegroundWindow(s_open); return; }

    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = LangProc;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        wc.hIcon         = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON));
        wc.lpszClassName = L"HarfbekLangWnd";
        RegisterClassW(&wc);
        registered = true;
    }

    s_dpi = Ui_Dpi(NULL);
    if (s_dpi <= 0) s_dpi = 96;
    s_cw = S(CW96);
    s_ch = S(CH96);
    s_result = -1;

    DWORD style = WS_CAPTION | WS_SYSMENU;
    RECT rc = { 0, 0, s_cw, s_ch };
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    int ww = rc.right - rc.left, wh = rc.bottom - rc.top;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;

    HWND hwnd = CreateWindowExW(0, L"HarfbekLangWnd", T(S_LANG_TITLE),
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

    if (s_result >= 0 && s_result < LANG_COUNT) {
        g_settings.lang = (Lang)s_result;
        Settings_Save();
    }
}
