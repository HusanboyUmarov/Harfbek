// Tanishtiruv (birinchi ishga tushish wizard'i): 6 qadam, klaviatura tugmalari
// rasm (GDI) ko'rinishida chiziladi. 4-qadamda ishlash rejimi tanlanadi
// ("Tugatish" bosilganda saqlanadi; Esc / X bilan yopilsa o'zgarmaydi).
// Orqaga / Keyingi tugmalari va qadam ko'rsatkichi bor.
// Barcha piksel o'lchamlari 96 DPI uchun yozilgan va S() orqali system DPI
// ga masshtablanadi (manifestda dpiAware=true).
#include "app.h"

#define ID_BACK        1001
#define ID_NEXT        1002
#define ID_RADIO_TRIG  1003
#define ID_RADIO_AUTO  1004
#define STEP_COUNT 6
#define STEP_MODE  3        // rejim tanlash sahifasi (0 dan boshlab)

// Oyna ichki o'lchami (px, 96 DPI)
static const int CW96 = 520;
static const int CH96 = 440;

// System DPI va unga masshtablangan oyna ichki o'lchami (ShowTutorial to'ldiradi)
static int s_dpi = 96;
static int s_cw  = CW96;
static int s_ch  = CH96;

// 96 DPI dagi pikselni joriy DPI ga o'tkazadi
static int S(int x) { return MulDiv(x, s_dpi, 96); }

// Ranglar
static const COLORREF C_BG      = RGB(248, 249, 251);
static const COLORREF C_TITLE   = RGB(24, 28, 38);
static const COLORREF C_BODY    = RGB(78, 84, 96);
static const COLORREF C_STEPTXT = RGB(150, 156, 168);
static const COLORREF C_ACCENT  = RGB(0, 120, 215);
static const COLORREF C_CAPBRD  = RGB(176, 182, 194);
static const COLORREF C_CAPTXT  = RGB(30, 34, 42);
static const COLORREF C_SHADOW  = RGB(214, 218, 226);
static const COLORREF C_ACC_BG  = RGB(232, 242, 254);
static const COLORREF C_ACC_TXT = RGB(0, 92, 178);
static const COLORREF C_DOT_OFF = RGB(206, 210, 220);

struct TutState {
    int    step;
    HFONT  fTitle, fTitleSm, fBody, fBodySm, fKey, fKeySm, fSym, fSymSm, fStep, fRadio;
    HBRUSH bgBrush;     // radio tugmalar foni uchun
};

// ---- bitta klaviatura tugmasini (keycap) chizadi ----
static void DrawKeyCap(HDC dc, int cx, int cy, int w, int h,
                       const wchar_t* txt, HFONT f, bool accent)
{
    RECT r = { cx - w/2, cy - h/2, cx + w/2, cy + h/2 };

    // soya
    HGDIOBJ oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
    HBRUSH  shb    = CreateSolidBrush(C_SHADOW);
    HGDIOBJ oldBr  = SelectObject(dc, shb);
    RoundRect(dc, r.left, r.top + S(3), r.right, r.bottom + S(3), S(9), S(9));
    SelectObject(dc, oldBr);
    DeleteObject(shb);

    // yuza
    HPEN   pen = CreatePen(PS_SOLID, S(1), accent ? C_ACCENT : C_CAPBRD);
    HBRUSH br  = CreateSolidBrush(accent ? C_ACC_BG : RGB(255,255,255));
    SelectObject(dc, pen);
    SelectObject(dc, br);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, S(9), S(9));
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBr);    // br DC'da tanlangan holda o'chirilmasin
    DeleteObject(pen);
    DeleteObject(br);

    // matn
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, accent ? C_ACC_TXT : C_CAPTXT);
    HGDIOBJ of = SelectObject(dc, f);
    DrawTextW(dc, txt, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, of);
}

// ketma-ketlik elementi: 0 = tugma, 1 = natija (rangli), 2 = belgi (+ ->)
struct SeqItem { const wchar_t* t; int kind; };

static int MeasW(HDC dc, const wchar_t* t, HFONT f)
{
    HGDIOBJ o = SelectObject(dc, f);
    SIZE s; GetTextExtentPoint32W(dc, t, lstrlenW(t), &s);
    SelectObject(dc, o);
    return s.cx;
}

// ketma-ketlik elementlarining kengliklarini w[] ga yozadi, jami kenglikni qaytaradi
static int SeqMeasure(HDC dc, int capH, const SeqItem* it, int n, int* w,
                      HFONT fKey, HFONT fSym)
{
    int total = 0;
    const int gap = S(7);
    const int minCap = (capH > S(38)) ? S(46) : S(32);
    const int padCap = (capH > S(38)) ? S(24) : S(16);
    for (int i = 0; i < n; ++i) {
        if (it[i].kind == 2) {
            w[i] = MeasW(dc, it[i].t, fSym) + S(10);
        } else {
            w[i] = MeasW(dc, it[i].t, fKey) + padCap;
            if (w[i] < minCap) w[i] = minCap;
        }
        total += w[i];
        if (i) total += gap;
    }
    return total;
}

// tugmalar ketma-ketligini chizadi (cy, capH — allaqachon masshtablangan).
// left < 0 bo'lsa oyna markaziga, aks holda chap chegarasi left da joylashadi.
static void DrawSeq(HDC dc, int left, int cy, int capH, const SeqItem* it, int n,
                    HFONT fKey, HFONT fSym)
{
    int w[16];
    const int gap = S(7);
    int total = SeqMeasure(dc, capH, it, n, w, fKey, fSym);
    int x = left < 0 ? (s_cw - total) / 2 : left;
    for (int i = 0; i < n; ++i) {
        if (it[i].kind == 2) {
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(128, 134, 146));
            HGDIOBJ o = SelectObject(dc, fSym);
            RECT r = { x, cy - capH/2, x + w[i], cy + capH/2 };
            DrawTextW(dc, it[i].t, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, o);
        } else {
            DrawKeyCap(dc, x + w[i]/2, cy, w[i], capH, it[i].t, fKey,
                       it[i].kind == 1);
        }
        x += w[i] + gap;
    }
}

// 1-qadam: o' g' sh ch -> ö ğ ş ç
static void DrawAlphabet(HDC dc, TutState* st)
{
    SeqItem seq[] = {
        { L"o'", 0 }, { L"g'", 0 }, { L"sh", 0 }, { L"ch", 0 },
        { L"→", 2 },
        { L"ö", 1 }, { L"ğ", 1 }, { L"ş", 1 }, { L"ç", 1 },
    };
    DrawSeq(dc, -1, S(140), S(40), seq, 9, st->fKey, st->fSym);
}

// 2-qadam: trigger + harf (va Shift bilan katta harf)
static void DrawTriggerDemo(HDC dc, const wchar_t* trig, TutState* st)
{
    // trigger o'ng Shift bo'lsa birinchi tugma chap Shift ekani ko'rinsin
    const wchar_t* shiftCap = g_settings.triggerVk == VK_RSHIFT ? L"L Shift" : L"Shift";
    SeqItem a[] = { { trig,0 }, { L"+",2 }, { L"O",0 }, { L"→",2 }, { L"ö",1 } };
    SeqItem b[] = { { shiftCap,0 }, { L"+",2 }, { trig,0 }, { L"+",2 },
                    { L"O",0 }, { L"→",2 }, { L"Ö",1 } };
    DrawSeq(dc, -1, S(108), S(46), a, 5, st->fKey, st->fSym);
    DrawSeq(dc, -1, S(176), S(46), b, 7, st->fKey, st->fSym);
}

// 3-qadam: avto rejim — ikki harf ketma-ket, va Backspace bilan qaytarish
static void DrawAutoDemo(HDC dc, TutState* st)
{
    SeqItem a[] = { { L"S",0 }, { L"H",0 }, { L"→",2 }, { L"ş",1 } };
    SeqItem b[] = { { L"O",0 }, { L"'",0 }, { L"→",2 }, { L"ö",1 } };
    SeqItem c[] = { { L"ş",1 }, { L"⌫",0 }, { L"→",2 }, { L"sh",0 } };
    DrawSeq(dc, -1, S(92),  S(36), a, 4, st->fKey, st->fSym);
    DrawSeq(dc, -1, S(138), S(36), b, 4, st->fKey, st->fSym);
    DrawSeq(dc, -1, S(184), S(36), c, 4, st->fKey, st->fSym);
}

// 5-qadam: tezkor tugmalar (bo'sh hotkey qatori ko'rsatilmaydi)
static void DrawHotkeys(HDC dc, TutState* st)
{
    struct { StrId label; WORD hk; } rows[3] = {
        { S_HK_MODE,    g_settings.hkMode    },
        { S_HK_ENABLE,  g_settings.hkEnable  },
        { S_HK_CONVERT, g_settings.hkConvert },
    };
    int cnt = 0;
    for (int i = 0; i < 3; ++i) if (rows[i].hk) ++cnt;
    if (!cnt) return;

    const int pitch = S(46);
    int y = S(142) - (cnt - 1) * pitch / 2;
    for (int i = 0; i < 3; ++i) {
        if (!rows[i].hk) continue;

        // "Ctrl+Shift+F9" -> alohida keycap'lar
        wchar_t txt[64];
        Hotkey_Format(rows[i].hk, txt, 64);
        wchar_t part[6][16];
        SeqItem seq[11];
        int n = 0, np = 0;
        for (wchar_t* p = txt; *p && np < 6; ) {
            int len = 0;
            while (p[len] && p[len] != L'+') ++len;
            if (len) {
                if (len > 15) len = 15;
                for (int k = 0; k < len; ++k) part[np][k] = p[k];
                part[np][len] = 0;
                if (n) { seq[n].t = L"+"; seq[n].kind = 2; ++n; }
                seq[n].t = part[np]; seq[n].kind = 0; ++n;
                ++np;
            }
            while (*p && *p != L'+') ++p;
            if (*p == L'+') ++p;
        }

        // yorliq (o'ngga tekislangan) va tugmalar (chapga tekislangan)
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, C_BODY);
        RECT lr = { S(8), y - S(15), S(232), y + S(15) };
        const wchar_t* lbl = T(rows[i].label);
        HFONT lf = MeasW(dc, lbl, st->fBody) > lr.right - lr.left ? st->fBodySm : st->fBody;
        HGDIOBJ of = SelectObject(dc, lf);
        DrawTextW(dc, lbl, -1, &lr,
                  DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        SelectObject(dc, of);

        if (n) DrawSeq(dc, S(244), y, S(34), seq, n, st->fKeySm, st->fSymSm);
        y += pitch;
    }
}

// tray ikonka + o'ng-tugma menyusi (6-qadam)
static void DrawTrayHint(HDC dc, TutState* st)
{
    const int ox = (s_cw - S(480)) / 2;     // 480 px uchun chizilgan rasmni markazga suradi
    int iconY = S(118);
    int iconSz = S(48);
    HICON ic = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON),
                                 IMAGE_ICON, iconSz, iconSz, LR_DEFAULTCOLOR);
    if (ic) {
        DrawIconEx(dc, ox + S(96), iconY, ic, iconSz, iconSz, 0, NULL, DI_NORMAL);
        DestroyIcon(ic);
    }

    // strelka
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(128, 134, 146));
    HGDIOBJ o = SelectObject(dc, st->fSym);
    RECT ar = { ox + S(150), iconY, ox + S(190), iconY + iconSz };
    DrawTextW(dc, L"→", -1, &ar, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, o);

    // menyu qutisi
    RECT m = { ox + S(202), S(92), ox + S(384), S(188) };
    HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
    HBRUSH shb = CreateSolidBrush(C_SHADOW);
    HGDIOBJ ob = SelectObject(dc, shb);
    RoundRect(dc, m.left, m.top + S(3), m.right, m.bottom + S(3), S(8), S(8));
    SelectObject(dc, ob); DeleteObject(shb);

    HPEN   pen = CreatePen(PS_SOLID, S(1), C_CAPBRD);
    HBRUSH br  = CreateSolidBrush(RGB(255, 255, 255));
    SelectObject(dc, pen); SelectObject(dc, br);
    RoundRect(dc, m.left, m.top, m.right, m.bottom, S(8), S(8));
    SelectObject(dc, op); SelectObject(dc, ob);     // br DC'da tanlangan holda o'chirilmasin
    DeleteObject(pen); DeleteObject(br);

    // birinchi qator — "Sozlamalar" ajratib ko'rsatiladi
    RECT hl = { m.left + S(4), m.top + S(6), m.right - S(4), m.top + S(28) };
    HBRUSH hb = CreateSolidBrush(C_ACC_BG);
    FillRect(dc, &hl, hb);
    DeleteObject(hb);

    const wchar_t* items[] = { T(S_MENU_SETTINGS), T(S_MENU_LANGUAGE), T(S_MENU_TUTORIAL) };
    HGDIOBJ of = SelectObject(dc, st->fBody);
    for (int i = 0; i < 3; ++i) {
        RECT ir = { m.left + S(12), m.top + S(6) + i*S(24),
                    m.right - S(8), m.top + S(28) + i*S(24) };
        SetTextColor(dc, i == 0 ? C_ACC_TXT : C_BODY);
        DrawTextW(dc, items[i], -1, &ir, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    SelectObject(dc, of);
}

static void PaintTutorial(HDC dc, TutState* st)
{
    // fon
    RECT full = { 0, 0, s_cw, s_ch };
    HBRUSH bg = CreateSolidBrush(C_BG);
    FillRect(dc, &full, bg);
    DeleteObject(bg);

    SetBkMode(dc, TRANSPARENT);

    StrId titleId[STEP_COUNT] = { S_WIZ_1_TITLE, S_WIZ_2_TITLE, S_WIZ_3_TITLE,
                                  S_WIZ_4_TITLE, S_WIZ_5_TITLE, S_WIZ_6_TITLE };
    StrId bodyId [STEP_COUNT] = { S_WIZ_1_BODY,  S_WIZ_2_BODY,  S_WIZ_3_BODY,
                                  S_WIZ_4_BODY,  S_WIZ_5_BODY,  S_WIZ_6_BODY  };

    // qadam raqami (o'ng yuqori)
    wchar_t sc[32];
    wsprintfW(sc, T(S_TUT_STEP), st->step + 1, STEP_COUNT);
    HGDIOBJ of = SelectObject(dc, st->fStep);
    SetTextColor(dc, C_STEPTXT);
    RECT srr = { s_cw - S(120), S(8), s_cw - S(20), S(22) };
    DrawTextW(dc, sc, -1, &srr, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
    SelectObject(dc, of);

    // sarlavha (uzun bo'lsa kichikroq shrift)
    const wchar_t* title = T(titleId[st->step]);
    RECT tr = { S(24), S(26), s_cw - S(24), S(62) };
    HFONT tf = st->fTitle;
    if (MeasW(dc, title, tf) > tr.right - tr.left) tf = st->fTitleSm;
    of = SelectObject(dc, tf);
    SetTextColor(dc, C_TITLE);
    DrawTextW(dc, title, -1, &tr,
              DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, of);

    // illyustratsiya
    wchar_t trig[24];
    TriggerKeyCap(g_settings.triggerVk, trig, 24);

    switch (st->step) {
    case 0:         DrawAlphabet(dc, st);          break;
    case 1:         DrawTriggerDemo(dc, trig, st); break;
    case 2:         DrawAutoDemo(dc, st);          break;
    case STEP_MODE: break;      // tanlov — radio tugmalar (child oynalar)
    case 4:         DrawHotkeys(dc, st);           break;
    default:        DrawTrayHint(dc, st);          break;
    }

    // tavsif matni (sig'masa kichikroq shrift)
    const wchar_t* body = T(bodyId[st->step]);
    RECT br = { S(34), S(222), s_cw - S(34), S(350) };
    UINT fl = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX;
    HFONT bf = st->fBody;
    RECT mr = br;
    of = SelectObject(dc, bf);
    DrawTextW(dc, body, -1, &mr, fl | DT_CALCRECT);
    if (mr.bottom - mr.top > br.bottom - br.top) {
        SelectObject(dc, st->fBodySm);
    }
    SetTextColor(dc, C_BODY);
    DrawTextW(dc, body, -1, &br, fl);
    SelectObject(dc, of);

    // qadam nuqtalari
    int dotY = S(366), r = S(5), pitch = S(18), span = STEP_COUNT * pitch - S(6);
    int x0 = (s_cw - span) / 2;
    HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
    for (int i = 0; i < STEP_COUNT; ++i) {
        HBRUSH d = CreateSolidBrush(i == st->step ? C_ACCENT : C_DOT_OFF);
        HGDIOBJ ob = SelectObject(dc, d);
        int cx = x0 + i * pitch;
        Ellipse(dc, cx, dotY - r, cx + 2*r, dotY + r);
        SelectObject(dc, ob);
        DeleteObject(d);
    }
    SelectObject(dc, op);
}

// tugmalar matni/holatini joriy qadamga moslaydi; radio tugmalar faqat rejim sahifasida
static void UpdateButtons(HWND hwnd, TutState* st)
{
    SetDlgItemTextW(hwnd, ID_BACK, T(S_TUT_BACK));
    SetDlgItemTextW(hwnd, ID_NEXT,
                    st->step == STEP_COUNT - 1 ? T(S_TUT_FINISH) : T(S_TUT_NEXT));
    EnableWindow(GetDlgItem(hwnd, ID_BACK), st->step > 0);

    int vis = st->step == STEP_MODE ? SW_SHOW : SW_HIDE;
    ShowWindow(GetDlgItem(hwnd, ID_RADIO_TRIG), vis);
    ShowWindow(GetDlgItem(hwnd, ID_RADIO_AUTO), vis);
}

// 4-qadamdagi tanlovni sozlamalarga yozadi (faqat "Tugatish" da chaqiriladi)
static void ApplyMode(HWND hwnd)
{
    InputMode m = SendDlgItemMessageW(hwnd, ID_RADIO_AUTO, BM_GETCHECK, 0, 0) == BST_CHECKED
                  ? MODE_AUTO : MODE_TRIGGER;
    if (m == g_settings.mode) return;
    g_settings.mode = m;
    Settings_Save();
    Hook_ResetTriggerState();
}

// bir qadam oldinga/orqaga; oxirgi qadamda "Tugatish" rejimni saqlab oynani yopadi
static void GoStep(HWND hwnd, TutState* st, int delta)
{
    int ns = st->step + delta;
    if (ns >= STEP_COUNT) { ApplyMode(hwnd); DestroyWindow(hwnd); return; }
    if (ns < 0) return;
    st->step = ns;
    UpdateButtons(hwnd, st);
    if (ns == STEP_MODE) {
        // tanlangan radioga fokus (strelka tugmalari bilan almashtirish mumkin)
        bool au = SendDlgItemMessageW(hwnd, ID_RADIO_AUTO, BM_GETCHECK, 0, 0) == BST_CHECKED;
        SetFocus(GetDlgItem(hwnd, au ? ID_RADIO_AUTO : ID_RADIO_TRIG));
    } else if (ns == 0 || GetFocus() == NULL || !IsWindowVisible(GetFocus())) {
        // "Orqaga" o'chib / radio yashirinib qolsa fokus yo'qolmasin (Enter/Esc ishlashda qolsin)
        SetFocus(GetDlgItem(hwnd, ID_NEXT));
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

static HFONT MakeFont(int px, int weight)
{
    return CreateFontW(-S(px),0,0,0,weight,0,0,0,DEFAULT_CHARSET,0,0,
                       CLEARTYPE_QUALITY,0,L"Segoe UI");
}

static LRESULT CALLBACK TutProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TutState* st = (TutState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        st = new TutState();
        st->step = 0;
        st->fTitle   = MakeFont(24, 700);
        st->fTitleSm = MakeFont(19, 700);
        st->fBody    = MakeFont(15, 400);
        st->fBodySm  = MakeFont(13, 400);
        st->fKey     = MakeFont(20, 600);
        st->fKeySm   = MakeFont(15, 600);
        st->fSym     = MakeFont(22, 400);
        st->fSymSm   = MakeFont(16, 400);
        st->fStep    = MakeFont(12, 400);
        st->fRadio   = MakeFont(17, 600);
        st->bgBrush  = CreateSolidBrush(C_BG);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);

        int bh = S(32);
        int by = s_ch - bh - S(18);
        HWND b = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP,
                     S(24), by, S(120), bh, hwnd, (HMENU)ID_BACK, g_hInst, NULL);
        HWND nx = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
                     s_cw - S(24) - S(140), by, S(140), bh, hwnd, (HMENU)ID_NEXT, g_hInst, NULL);
        SendMessageW(b,  WM_SETFONT, (WPARAM)st->fBody, TRUE);
        SendMessageW(nx, WM_SETFONT, (WPARAM)st->fBody, TRUE);

        // rejim tanlash: ikkita katta radio (boshida yashirin, faqat 4-qadamda ko'rinadi)
        int rw = S(460), rh = S(48), rx = (s_cw - rw) / 2;
        HWND r1 = CreateWindowW(L"BUTTON", T(S_MODE_TRIGGER),
                     WS_CHILD|WS_TABSTOP|WS_GROUP|BS_AUTORADIOBUTTON|BS_MULTILINE,
                     rx, S(88), rw, rh, hwnd, (HMENU)ID_RADIO_TRIG, g_hInst, NULL);
        HWND r2 = CreateWindowW(L"BUTTON", T(S_MODE_AUTO),
                     WS_CHILD|BS_AUTORADIOBUTTON|BS_MULTILINE,
                     rx, S(146), rw, rh, hwnd, (HMENU)ID_RADIO_AUTO, g_hInst, NULL);
        SendMessageW(r1, WM_SETFONT, (WPARAM)st->fRadio, TRUE);
        SendMessageW(r2, WM_SETFONT, (WPARAM)st->fRadio, TRUE);
        SendMessageW(g_settings.mode == MODE_AUTO ? r2 : r1, BM_SETCHECK, BST_CHECKED, 0);

        UpdateButtons(hwnd, st);
        return 0;
    }

    case DM_GETDEFID:
        // Enter har doim "Keyingi" (fokus radioda bo'lsa ham)
        return MAKELONG(ID_NEXT, DC_HASDEFID);

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case ID_BACK:   GoStep(hwnd, st, -1); break;
        case ID_NEXT:
        case IDOK:      GoStep(hwnd, st, +1); break;   // Enter = "Keyingi"
        case IDCANCEL:  DestroyWindow(hwnd);  break;   // Esc = yopish (rejim saqlanmaydi)
        }
        return 0;

    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
        // radio tugmalar matn foni oyna foniga mos bo'lsin
        if (st) {
            SetBkColor((HDC)wp, C_BG);
            SetTextColor((HDC)wp, C_TITLE);
            return (LRESULT)st->bgBrush;
        }
        break;

    case WM_ERASEBKGND:
        // asosiy chizish WM_PAINT da (off-screen); bu yerda faqat child (radio)
        // fonini oyna rangiga bo'yaymiz — ularning temali foni shu DC dan olinadi
        if (st) {
            RECT cr; GetClipBox((HDC)wp, &cr);
            FillRect((HDC)wp, &cr, st->bgBrush);
        }
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        // ikki bosqichli (off-screen) chizish — silliq ko'rinish uchun
        HDC   mem = CreateCompatibleDC(dc);
        HBITMAP bmp = CreateCompatibleBitmap(dc, s_cw, s_ch);
        HGDIOBJ ob = SelectObject(mem, bmp);
        PaintTutorial(mem, st);
        BitBlt(dc, 0, 0, s_cw, s_ch, mem, 0, 0, SRCCOPY);
        SelectObject(mem, ob);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCDESTROY:
        if (st) {
            DeleteObject(st->fTitle); DeleteObject(st->fTitleSm);
            DeleteObject(st->fBody);  DeleteObject(st->fBodySm);
            DeleteObject(st->fKey);   DeleteObject(st->fKeySm);
            DeleteObject(st->fSym);   DeleteObject(st->fSymSm);
            DeleteObject(st->fStep);  DeleteObject(st->fRadio);
            DeleteObject(st->bgBrush);
            delete st;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void ShowTutorial(HWND parent)
{
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = TutProc;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        wc.hIcon         = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON));
        wc.lpszClassName = L"HarfbekTutorialWnd";
        RegisterClassW(&wc);
        registered = true;
    }

    // system DPI (manifestda dpiAware=true bo'lgani uchun haqiqiy qiymat qaytadi)
    HDC sdc = GetDC(NULL);
    s_dpi = sdc ? GetDeviceCaps(sdc, LOGPIXELSY) : 96;
    if (sdc) ReleaseDC(NULL, sdc);
    if (s_dpi <= 0) s_dpi = 96;
    s_cw = S(CW96);
    s_ch = S(CH96);

    DWORD style   = WS_CAPTION | WS_SYSMENU;
    DWORD exstyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    RECT rc = { 0, 0, s_cw, s_ch };
    AdjustWindowRectEx(&rc, style, FALSE, exstyle);
    int ww = rc.right - rc.left, wh = rc.bottom - rc.top;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;

    HWND hwnd = CreateWindowExW(exstyle, L"HarfbekTutorialWnd", T(S_TUT_TITLE),
                                style, sx, sy, ww, wh, parent, NULL, g_hInst, NULL);
    if (!hwnd) return;

    BOOL parentWasEnabled = IsWindowEnabled(parent);
    if (parent && parentWasEnabled) EnableWindow(parent, FALSE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(GetDlgItem(hwnd, ID_NEXT));   // Enter darhol "Keyingi" ni bossin

    MSG msg;
    while (IsWindow(hwnd) && GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (parent && parentWasEnabled) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }
}
