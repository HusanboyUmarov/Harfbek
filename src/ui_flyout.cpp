// Tray flyout: tray ikonkasi chap tugma bilan bosilganda chiqadigan kichik
// panel (Win11 quick settings uslubida). Ichida: sarlavha + holat, "Yoqilgan"
// toggle, rejim segmenti (Trigger | Avto), Sozlamalar / Tanişuv / Çiqiş.
//
// Resurslar: oyna faqat ochiq paytda yashaydi. Fokus yo'qolsa darhol
// DestroyWindow — shriftlar WM_NCDESTROY da o'chiriladi, Ui_End GDI+ ni
// bo'shatadi (boshqa UI oynasi ochiq bo'lmasa). Taymer faqat animatsiya
// paytida ishlaydi. Faqat main thread.
#include "app.h"
#include <shellapi.h>   // SHAppBarMessage

// Elementlar (hit-test va klaviatura fokus tartibi)
enum {
    FI_NONE = -1,
    FI_TOGGLE = 0,      // "Yoqilgan"
    FI_SEG_TRIGGER,     // segment: Trigger
    FI_SEG_AUTO,        // segment: Avto
    FI_PLAYGROUND,      // "Sinab köriş" (keng qator)
    FI_SETTINGS,
    FI_GUIDE,
    FI_EXIT,
    FI_COUNT
};

static const int  kW96       = 300;   // panel kengligi (96 DPI)
static const int  kH96       = 326;   // balandligi
static const int  kGap96     = 12;    // ish maydoni chetidan masofa
static const int  kSlide96   = 10;    // ochilishda sirpanish masofasi
static const DWORD kOpenMs   = 180;
static const DWORD kHoverMs  = 120;
static const DWORD kStateMs  = 200;

static const wchar_t* const kClass = L"HarfbekFlyout";

static struct {
    HWND   hwnd;
    int    dpi;
    HFONT  fTitle, fBody, fSmall, fSemi, fIcon, fSmallIcon;
    POINT  pos;           // yakuniy joy (ekran)
    int    slideX, slideY;// sirpanish yo'nalishi (yakuniy joyga nisbatan boshlanish siljishi)
    UiAnim open;          // 0 -> 1: shaffoflik + sirpanish
    UiAnim toggle;        // knob joyi
    UiAnim seg;           // segment indikatori: 0 Trigger, 1 Avto
    UiAnim hov[FI_COUNT];
    int    hot;           // sichqoncha ostidagi element
    int    pressed;       // bosib turilgan element
    int    focus;         // klaviatura fokusi
    bool   kbd;           // klaviatura ishlatildimi (fokus halqasi faqat shunda)
    bool   tracking;      // TrackMouseEvent o'rnatilgan
} g_fly;

static DWORD g_flyClosedTick = 0;   // fokus yo'qolib yopilgan vaqt (tray qayta bosilishini ajratish uchun)

// ---------------------------------------------------------------------
//  Joylashuv (96 DPI qiymatlar Ui_S bilan masshtablanadi)
// ---------------------------------------------------------------------
struct FlyLayout {
    RECT header, status, card, toggleRow, toggleLabel, toggle, divider,
         segLabel, seg, segOpt[2], play, btn[3];
};

static RECT Rc(int dpi, int x, int y, int w, int h)
{
    RECT r = { Ui_S(x, dpi), Ui_S(y, dpi), Ui_S(x + w, dpi), Ui_S(y + h, dpi) };
    return r;
}

static void Layout(int dpi, FlyLayout& L)
{
    const int p = 16, w = kW96 - 2 * p;
    L.header      = Rc(dpi, p, 16, 150, 28);
    L.status      = Rc(dpi, p + 120, 16, w - 120, 28);
    L.card        = Rc(dpi, p, 56, w, 130);
    L.toggleRow   = Rc(dpi, p, 56, w, 48);
    L.toggleLabel = Rc(dpi, p + 16, 56, w - 90, 48);
    L.toggle      = Rc(dpi, p + w - 16 - 48, 66, 48, 28);
    L.divider     = Rc(dpi, p + 12, 104, w - 24, 1);
    L.segLabel    = Rc(dpi, p + 16, 112, w - 32, 22);
    L.seg         = Rc(dpi, p + 12, 138, w - 24, 36);
    int half = (L.seg.right - L.seg.left) / 2;
    L.segOpt[0] = L.seg; L.segOpt[0].right = L.seg.left + half;
    L.segOpt[1] = L.seg; L.segOpt[1].left  = L.seg.left + half;
    L.play        = Rc(dpi, p, 198, w, 40);
    const int bw = (w - 2 * 8) / 3;
    for (int i = 0; i < 3; ++i) L.btn[i] = Rc(dpi, p + i * (bw + 8), 246, bw, 64);
}

static int HitTest(POINT pt)
{
    FlyLayout L; Layout(g_fly.dpi, L);
    RECT row = L.toggleRow;
    if (PtInRect(&row, pt))         return FI_TOGGLE;     // butun qator bosiladi
    if (PtInRect(&L.segOpt[0], pt)) return FI_SEG_TRIGGER;
    if (PtInRect(&L.segOpt[1], pt)) return FI_SEG_AUTO;
    if (PtInRect(&L.play, pt))      return FI_PLAYGROUND;
    for (int i = 0; i < 3; ++i)
        if (PtInRect(&L.btn[i], pt)) return FI_SETTINGS + i;
    return FI_NONE;
}

// "Sozlamalar..." -> "Sozlamalar", "Rejim:" -> "Rejim" (tugma/sarlavha uchun)
static void CleanLabel(const wchar_t* s, wchar_t* out, int n)
{
    lstrcpynW(out, s, n);
    int len = lstrlenW(out);
    while (len > 0 && (out[len - 1] == L'.' || out[len - 1] == 0x2026 ||
                       out[len - 1] == L':' || out[len - 1] == L' '))
        out[--len] = 0;
}

// ---------------------------------------------------------------------
//  Animatsiya
// ---------------------------------------------------------------------
static bool AnyRunning()
{
    if (g_fly.open.running || g_fly.toggle.running || g_fly.seg.running) return true;
    for (int i = 0; i < FI_COUNT; ++i) if (g_fly.hov[i].running) return true;
    return false;
}

static void StartTimer()
{
    if (g_fly.hwnd && AnyRunning()) SetTimer(g_fly.hwnd, UI_ANIM_TIMER, 15, NULL);
}

// Ochilish kadri: joy (sirpanish) va shaffoflik. Tugagach WS_EX_LAYERED
// olib tashlanadi — oddiy oyna (DWM yumaloq burchak/soya to'liq ishlaydi).
static void ApplyOpenFrame()
{
    float v = g_fly.open.value;
    int dx = (int)(g_fly.slideX * (1.0f - v));
    int dy = (int)(g_fly.slideY * (1.0f - v));
    SetWindowPos(g_fly.hwnd, NULL, g_fly.pos.x + dx, g_fly.pos.y + dy, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    LONG_PTR ex = GetWindowLongPtrW(g_fly.hwnd, GWL_EXSTYLE);
    if (g_fly.open.running) {
        SetLayeredWindowAttributes(g_fly.hwnd, 0, (BYTE)(255 * v), LWA_ALPHA);
    } else if (ex & WS_EX_LAYERED) {
        SetWindowLongPtrW(g_fly.hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
        RedrawWindow(g_fly.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
    }
}

static void SetHot(int item)
{
    if (item == g_fly.hot) return;
    if (g_fly.hot != FI_NONE) Ui_AnimTo(g_fly.hov[g_fly.hot], 0, kHoverMs);
    g_fly.hot = item;
    if (item != FI_NONE) Ui_AnimTo(g_fly.hov[item], 1, kHoverMs);
    StartTimer();
    InvalidateRect(g_fly.hwnd, NULL, FALSE);
}

// ---------------------------------------------------------------------
//  Amallar
// ---------------------------------------------------------------------
static void Close()
{
    if (g_fly.hwnd) DestroyWindow(g_fly.hwnd);
}

static void Activate(int item)
{
    switch (item) {
    case FI_TOGGLE:
        ToggleEnabled();                         // u Flyout_Refresh ni chaqiradi
        break;
    case FI_SEG_TRIGGER:
        if (g_settings.mode != MODE_TRIGGER) ToggleMode();
        break;
    case FI_SEG_AUTO:
        if (g_settings.mode != MODE_AUTO) ToggleMode();
        break;
    case FI_PLAYGROUND:
    case FI_SETTINGS:
    case FI_GUIDE:
    case FI_EXIT: {
        // Modal oyna flyout yopilgandan KEYIN, main oynaning navbatidan ochiladi
        int cmd = item == FI_PLAYGROUND ? CMD_PLAYGROUND :
                  item == FI_SETTINGS   ? CMD_SETTINGS   :
                  item == FI_GUIDE      ? CMD_TUTORIAL   : CMD_EXIT;
        Close();
        PostMessageW(g_hWnd, WM_COMMAND, cmd, 0);
        break;
    }
    }
}

// ---------------------------------------------------------------------
//  Chizish
// ---------------------------------------------------------------------
static void Paint(HDC dc)
{
    const UiTheme& t = Ui_Theme();
    const int dpi = g_fly.dpi;
    FlyLayout L; Layout(dpi, L);
    wchar_t buf[128];

    // Sarlavha + holat
    Ui_Text(dc, L"Harfbek", L.header, g_fly.fTitle, t.text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    bool on = g_settings.enabled;
    const wchar_t* st = T(on ? S_FLY_ACTIVE : S_FLY_PAUSED);
    SIZE sz = {};
    HDC     mdc = dc;
    HGDIOBJ of = SelectObject(mdc, g_fly.fSmall);
    GetTextExtentPoint32W(mdc, st, lstrlenW(st), &sz);
    SelectObject(mdc, of);
    // Holat "pill": nuqta + matn, o'ng tomonda
    int pillW = sz.cx + Ui_S(28, dpi), pillH = Ui_S(24, dpi);
    RECT pill = { L.status.right - pillW, (L.status.top + L.status.bottom - pillH) / 2, L.status.right, 0 };
    pill.bottom = pill.top + pillH;
    COLORREF green = t.dark ? RGB(0x4A, 0xDE, 0x80) : RGB(0x16, 0xA3, 0x4A);
    COLORREF dotC  = on ? green : t.textDim;
    Ui_FillRound(dc, pill, pillH / 2, on ? Ui_Mix(t.bg, green, t.dark ? 0.16f : 0.10f) : t.surfaceHover);
    Ui_FillCircle(dc, pill.left + Ui_S(13, dpi), (pill.top + pill.bottom) / 2, Ui_S(4, dpi), dotC);
    RECT pt = pill; pt.left += Ui_S(22, dpi);
    Ui_Text(dc, st, pt, g_fly.fSmall, on ? t.text : t.textDim, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

    // Kartochka
    Ui_FillRound(dc, L.card, Ui_S(8, dpi), t.surface);
    Ui_StrokeRound(dc, L.card, Ui_S(8, dpi), t.border, 1);

    // "Yoqilgan" qatori: nom + (hotkey) va toggle
    float hT = g_fly.hov[FI_TOGGLE].value;
    if (hT > 0) {
        RECT hr = L.toggleRow; InflateRect(&hr, -Ui_S(4, dpi), -Ui_S(4, dpi));
        Ui_FillRound(dc, hr, Ui_S(6, dpi), t.surfaceHover, (BYTE)(255 * hT));
    }
    wchar_t hk[64];
    Hotkey_Format(g_settings.hkEnable, hk, 64);
    RECT lr = L.toggleLabel;
    if (hk[0]) {
        RECT a = lr; a.bottom = (lr.top + lr.bottom) / 2 + Ui_S(2, dpi);
        RECT b = lr; b.top = a.bottom;
        Ui_Text(dc, T(S_MENU_ENABLED), a, g_fly.fBody, t.text, DT_SINGLELINE | DT_BOTTOM | DT_NOPREFIX | DT_END_ELLIPSIS);
        Ui_Text(dc, hk, b, g_fly.fSmall, t.textDim, DT_SINGLELINE | DT_TOP | DT_NOPREFIX);
    } else {
        Ui_Text(dc, T(S_MENU_ENABLED), lr, g_fly.fBody, t.text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    }
    Ui_DrawToggle(dc, L.toggle, g_fly.toggle.value, hT, true, g_fly.kbd && g_fly.focus == FI_TOGGLE);

    HBRUSH div = CreateSolidBrush(t.border);
    FillRect(dc, &L.divider, div);
    DeleteObject(div);

    // Rejim: sarlavha + hotkey, segment
    CleanLabel(T(S_TAB_MODE), buf, 128);
    Ui_Text(dc, buf, L.segLabel, g_fly.fSmall, t.textDim, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    Hotkey_Format(g_settings.hkMode, hk, 64);
    if (hk[0]) Ui_Text(dc, hk, L.segLabel, g_fly.fSmall, t.textDim, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX);

    int segR = Ui_S(8, dpi), inset = Ui_S(3, dpi);
    Ui_FillRound(dc, L.seg, segR, t.bg);
    Ui_StrokeRound(dc, L.seg, segR, t.border, 1);
    float s = g_fly.seg.value;
    for (int i = 0; i < 2; ++i) {                // tanlanmagan variant ustida hover
        float h = g_fly.hov[FI_SEG_TRIGGER + i].value * (i == 0 ? s : 1.0f - s);
        if (h > 0) {
            RECT o = L.segOpt[i]; InflateRect(&o, -inset, -inset);
            Ui_FillRound(dc, o, segR - inset, t.surfaceHover, (BYTE)(255 * h));
        }
    }
    // Sirpanuvchi indikator
    int half = L.segOpt[0].right - L.segOpt[0].left;
    RECT ind = L.segOpt[0];
    OffsetRect(&ind, (int)(half * s + 0.5f), 0);
    InflateRect(&ind, -inset, -inset);
    Ui_FillRound(dc, ind, segR - inset, t.accent);
    for (int i = 0; i < 2; ++i) {
        float sel = i == 0 ? 1.0f - s : s;       // indikator shu variant ustida qanchalik
        COLORREF c = Ui_Mix(t.text, t.onAccent, sel);
        Ui_Text(dc, T(i == 0 ? S_OSD_MODE_TRIGGER : S_OSD_MODE_AUTO), L.segOpt[i],
                sel > 0.5f ? g_fly.fSemi : g_fly.fBody, c,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    }
    if (g_fly.kbd && (g_fly.focus == FI_SEG_TRIGGER || g_fly.focus == FI_SEG_AUTO)) {
        RECT fr = L.seg; InflateRect(&fr, Ui_S(3, dpi), Ui_S(3, dpi));
        Ui_DrawFocusRing(dc, fr, segR + Ui_S(3, dpi));
    }

    // "Sinab köriş" — keng qator: accent tusli fon, chapda accent kvadrat ichida
    // qalam glifi, o'ngda chevron (hover'da biroz o'ngga suriladi)
    {
        float h  = g_fly.hov[FI_PLAYGROUND].value;
        bool  pr = g_fly.pressed == FI_PLAYGROUND && g_fly.hot == FI_PLAYGROUND;
        RECT  b  = L.play;
        int   r8 = Ui_S(8, dpi);
        COLORREF fill = Ui_Mix(t.surface, t.accent, (t.dark ? 0.10f : 0.06f) + 0.06f * h);
        if (pr) fill = Ui_Mix(fill, t.accent, 0.12f);
        Ui_FillRound(dc, b, r8, fill);
        Ui_StrokeRound(dc, b, r8, Ui_Mix(t.border, t.accent, 0.35f + 0.4f * h), 1);

        int  sq = Ui_S(28, dpi), m = (b.bottom - b.top - sq) / 2;
        RECT ic = { b.left + m, b.top + m, b.left + m + sq, b.top + m + sq };
        Ui_FillRound(dc, ic, Ui_S(6, dpi), Ui_Mix(t.accent, t.accentHover, h));
        Ui_Glyph(dc, L"\xE70F", ic, g_fly.fIcon, t.onAccent);              // Edit (qalam)

        RECT tx = b; tx.left = ic.right + Ui_S(10, dpi); tx.right -= Ui_S(32, dpi);
        Ui_Text(dc, T(S_NAV_PLAYGROUND), tx, g_fly.fSemi, t.text,
                DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

        RECT ch = b; ch.left = b.right - Ui_S(32, dpi);
        OffsetRect(&ch, (int)(h * Ui_S(3, dpi) + 0.5f), 0);
        Ui_Glyph(dc, L"\xE76C", ch, g_fly.fSmallIcon, Ui_Mix(t.textDim, t.accent, h));   // ChevronRight

        if (g_fly.kbd && g_fly.focus == FI_PLAYGROUND) {
            RECT fr = b; InflateRect(&fr, Ui_S(2, dpi), Ui_S(2, dpi));
            Ui_DrawFocusRing(dc, fr, Ui_S(10, dpi));
        }
    }

    // Pastki tugmalar: ikonka + matn (tile)
    static const wchar_t* const glyphs[3] = { L"\xE713", L"\xE897", L"\xE7E8" };   // Settings, Help, Power
    const StrId labels[3] = { S_MENU_SETTINGS, S_FLY_GUIDE, S_MENU_EXIT };
    for (int i = 0; i < 3; ++i) {
        int   id = FI_SETTINGS + i;
        float h  = g_fly.hov[id].value;
        bool  pr = g_fly.pressed == id && g_fly.hot == id;
        RECT  b  = L.btn[i];
        COLORREF fill = Ui_Mix(t.surface, t.surfaceHover, h);
        if (pr) fill = Ui_Mix(fill, t.border, 0.6f);
        Ui_FillRound(dc, b, Ui_S(8, dpi), fill);
        Ui_StrokeRound(dc, b, Ui_S(8, dpi), Ui_Mix(t.border, t.textDim, 0.3f * h), 1);
        COLORREF gc = (id == FI_EXIT) ? Ui_Mix(t.text, t.danger, h) : Ui_Mix(t.text, t.accent, h);
        RECT g = b; g.top += Ui_S(10, dpi); g.bottom = g.top + Ui_S(24, dpi);
        if (pr) OffsetRect(&g, 0, Ui_S(1, dpi));
        Ui_Glyph(dc, glyphs[i], g, g_fly.fIcon, gc);
        RECT tx = b; tx.top = g.bottom + Ui_S(4, dpi); tx.left += Ui_S(4, dpi); tx.right -= Ui_S(4, dpi);
        CleanLabel(T(labels[i]), buf, 128);
        Ui_Text(dc, buf, tx, g_fly.fSmall, t.text, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        if (g_fly.kbd && g_fly.focus == id) {
            RECT fr = b; InflateRect(&fr, Ui_S(2, dpi), Ui_S(2, dpi));
            Ui_DrawFocusRing(dc, fr, Ui_S(10, dpi));
        }
    }
}

// ---------------------------------------------------------------------
//  Klaviatura
// ---------------------------------------------------------------------
// Fokus to'xtash joylari: toggle, segment (bitta), "Sinab köriş", 3 ta tugma
static int NextFocus(int f, int dir)
{
    static const int order[6] = { FI_TOGGLE, FI_SEG_TRIGGER, FI_PLAYGROUND, FI_SETTINGS, FI_GUIDE, FI_EXIT };
    int cur = 0;
    int ff = (f == FI_SEG_AUTO) ? FI_SEG_TRIGGER : f;
    for (int i = 0; i < 6; ++i) if (order[i] == ff) cur = i;
    cur = (cur + dir + 6) % 6;
    return order[cur];
}

static void OnKey(WPARAM vk)
{
    int f = g_fly.focus;
    bool seg = (f == FI_SEG_TRIGGER || f == FI_SEG_AUTO);
    bool btn = (f >= FI_SETTINGS);
    switch (vk) {
    case VK_ESCAPE: Close(); return;
    case VK_TAB:    f = NextFocus(f, (GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1); break;
    case VK_DOWN:   f = NextFocus(f, 1);  break;
    case VK_UP:     f = NextFocus(f, -1); break;
    case VK_LEFT:
    case VK_RIGHT:
        if (seg) {                               // segment: strelka rejimni tanlaydi
            Activate(vk == VK_LEFT ? FI_SEG_TRIGGER : FI_SEG_AUTO);
        } else if (btn) {
            int i = f - FI_SETTINGS + (vk == VK_LEFT ? -1 : 1);
            if (i >= 0 && i < 3) f = FI_SETTINGS + i;
        }
        break;
    case VK_RETURN:
    case VK_SPACE:
        if (seg) Activate(g_settings.mode == MODE_AUTO ? FI_SEG_TRIGGER : FI_SEG_AUTO);
        else     Activate(f);
        if (!g_fly.hwnd) return;                 // tugma oynani yopdi
        break;
    default:
        return;
    }
    g_fly.kbd   = true;
    g_fly.focus = f;
    InvalidateRect(g_fly.hwnd, NULL, FALSE);
}

// ---------------------------------------------------------------------
//  Oyna
// ---------------------------------------------------------------------
static void DeleteFonts()
{
    HFONT* fs[6] = { &g_fly.fTitle, &g_fly.fBody, &g_fly.fSmall, &g_fly.fSemi, &g_fly.fIcon, &g_fly.fSmallIcon };
    for (int i = 0; i < 6; ++i) if (*fs[i]) { DeleteObject(*fs[i]); *fs[i] = NULL; }
}

static void CreateFonts()
{
    DeleteFonts();
    g_fly.fTitle = Ui_Font(g_fly.dpi, 16, FW_SEMIBOLD);
    g_fly.fBody  = Ui_Font(g_fly.dpi, 14, FW_NORMAL);
    g_fly.fSmall = Ui_Font(g_fly.dpi, 12, FW_NORMAL);
    g_fly.fSemi  = Ui_Font(g_fly.dpi, 14, FW_SEMIBOLD);
    g_fly.fIcon  = Ui_IconFont(g_fly.dpi, 18);
    g_fly.fSmallIcon = Ui_IconFont(g_fly.dpi, 12);
}

static LRESULT CALLBACK FlyProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        g_fly.hwnd = h;
        Ui_Begin();
        CreateFonts();
        Ui_ApplyWindowStyle(h);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps; HDC mem;
        Ui_BeginBuffer(h, &ps, &mem);
        Paint(mem);
        Ui_EndBuffer(h, &ps, mem);
        return 0;
    }

    case WM_TIMER:
        if (wp == UI_ANIM_TIMER) {
            bool opening = g_fly.open.running;
            Ui_AnimStep(g_fly.open);
            Ui_AnimStep(g_fly.toggle);
            Ui_AnimStep(g_fly.seg);
            for (int i = 0; i < FI_COUNT; ++i) Ui_AnimStep(g_fly.hov[i]);
            if (opening) ApplyOpenFrame();
            if (!AnyRunning()) KillTimer(h, UI_ANIM_TIMER);
            InvalidateRect(h, NULL, FALSE);
        }
        return 0;

    case WM_MOUSEMOVE: {
        if (!g_fly.tracking) {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, h, 0 };
            g_fly.tracking = TrackMouseEvent(&tme) != FALSE;
        }
        POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
        SetHot(HitTest(pt));
        return 0;
    }
    case WM_MOUSELEAVE:
        g_fly.tracking = false;
        SetHot(FI_NONE);
        return 0;

    case WM_LBUTTONDOWN: {
        POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
        g_fly.pressed = HitTest(pt);
        g_fly.kbd = false;                        // sichqoncha — fokus halqasi yashiriladi
        if (g_fly.pressed != FI_NONE) { g_fly.focus = g_fly.pressed; SetCapture(h); }
        InvalidateRect(h, NULL, FALSE);
        return 0;
    }
    case WM_LBUTTONUP: {
        POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
        int was = g_fly.pressed;
        g_fly.pressed = FI_NONE;
        if (GetCapture() == h) ReleaseCapture();
        if (was != FI_NONE && HitTest(pt) == was) Activate(was);   // oyna yopilgan bo'lishi mumkin
        if (g_fly.hwnd) InvalidateRect(h, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
        OnKey(wp);
        return 0;

    case WM_ACTIVATE:
        if (LOWORD(wp) == WA_INACTIVE) {
            g_flyClosedTick = GetTickCount();
            PostMessageW(h, WM_CLOSE, 0, 0);       // boshqa oynaga o'tildi — yopamiz
        }
        return 0;

    case WM_THEMECHANGED:                         // mavzu o'zgardi (main.cpp yuboradi)
        Ui_ApplyWindowStyle(h);
        InvalidateRect(h, NULL, FALSE);
        return 0;

    case WM_CLOSE:
        DestroyWindow(h);
        return 0;

    case WM_NCDESTROY:
        DeleteFonts();
        Ui_End();                                 // oxirgi UI oynasi bo'lsa GDI+ bo'shaydi
        ZeroMemory(&g_fly, sizeof(g_fly));
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

// Panel joyi: taskbar qaysi chetda bo'lsa, shu chetga yopishadi, anchor
// (ikonka) bo'ylab markazlanadi va ish maydonidan chiqmaydi.
static void PlaceNear(POINT anchor, int w, int hgt, int dpi)
{
    HMONITOR    mon = MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(mon, &mi);
    RECT wa = mi.rcWork;
    int  gap = Ui_S(kGap96, dpi), sl = Ui_S(kSlide96, dpi);

    APPBARDATA ab = { sizeof(ab) };
    UINT edge = ABE_BOTTOM;
    if (SHAppBarMessage(ABM_GETTASKBARPOS, &ab)) edge = ab.uEdge;

    int x = anchor.x - w / 2, y = anchor.y - hgt / 2;
    g_fly.slideX = g_fly.slideY = 0;
    switch (edge) {
    case ABE_TOP:    y = wa.top + gap;            g_fly.slideY = -sl; break;
    case ABE_LEFT:   x = wa.left + gap;           g_fly.slideX = -sl; break;
    case ABE_RIGHT:  x = wa.right - w - gap;      g_fly.slideX =  sl; break;
    default:         y = wa.bottom - hgt - gap;   g_fly.slideY =  sl; break;
    }
    if (x < wa.left + gap)        x = wa.left + gap;
    if (x + w > wa.right - gap)   x = wa.right - gap - w;
    if (y < wa.top + gap)         y = wa.top + gap;
    if (y + hgt > wa.bottom - gap) y = wa.bottom - gap - hgt;
    g_fly.pos.x = x;
    g_fly.pos.y = y;
}

void Flyout_Refresh()
{
    if (!g_fly.hwnd) return;
    Ui_AnimTo(g_fly.toggle, g_settings.enabled ? 1.0f : 0.0f, kStateMs);
    Ui_AnimTo(g_fly.seg, g_settings.mode == MODE_AUTO ? 1.0f : 0.0f, kStateMs);
    StartTimer();
    InvalidateRect(g_fly.hwnd, NULL, FALSE);
}

void Flyout_Toggle(POINT anchor)
{
    if (g_fly.hwnd) { Close(); return; }
    // Tray ikonkasi bosilganda flyout avval fokusni yo'qotib yopiladi, keyin
    // shu bosish bu yerga keladi — uni "yopish" deb hisoblaymiz, qayta ochmaymiz.
    if (g_flyClosedTick && GetTickCount() - g_flyClosedTick < 400) return;

    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.style         = CS_DROPSHADOW;   // Win10 da soya; Win11 da DWM soyasi bilan birga
        wc.lpfnWndProc   = FlyProc;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        wc.lpszClassName = kClass;
        registered = RegisterClassW(&wc) != 0;
        if (!registered) return;
    }

    ZeroMemory(&g_fly, sizeof(g_fly));
    g_fly.hot = g_fly.pressed = FI_NONE;
    g_fly.focus = FI_TOGGLE;
    g_fly.dpi = Ui_Dpi(NULL);
    g_fly.toggle.value = g_fly.toggle.to = g_settings.enabled ? 1.0f : 0.0f;
    g_fly.seg.value    = g_fly.seg.to    = g_settings.mode == MODE_AUTO ? 1.0f : 0.0f;

    int w = Ui_S(kW96, g_fly.dpi), hgt = Ui_S(kH96, g_fly.dpi);
    PlaceNear(anchor, w, hgt, g_fly.dpi);

    bool anim = Ui_AnimationsEnabled();
    HWND h = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | (anim ? WS_EX_LAYERED : 0),
                             kClass, L"Harfbek", WS_POPUP,
                             g_fly.pos.x + (anim ? g_fly.slideX : 0), g_fly.pos.y + (anim ? g_fly.slideY : 0),
                             w, hgt, NULL, NULL, g_hInst, NULL);
    if (!h) return;
    if (anim) {
        SetLayeredWindowAttributes(h, 0, 0, LWA_ALPHA);
        g_fly.open.value = 0;
        Ui_AnimTo(g_fly.open, 1.0f, kOpenMs);
    } else {
        g_fly.open.value = g_fly.open.to = 1.0f;
    }
    ShowWindow(h, SW_SHOW);
    SetForegroundWindow(h);
    StartTimer();
}
