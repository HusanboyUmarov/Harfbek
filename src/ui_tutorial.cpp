// Qo'llanma oynasi: 4 qadamli, klaviatura tugmalari rasm (GDI) ko'rinishida
// chiziladi. Keyingi / Orqaga tugmalari va qadam ko'rsatkichi bor.
#include "app.h"
#include <windowsx.h>

#define ID_BACK  1001
#define ID_NEXT  1002
#define STEP_COUNT 4

// Oyna ichki o'lchami (px, 96 DPI)
static const int CW = 480;
static const int CH = 392;

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
    int   step;
    HFONT fTitle, fBody, fKey, fKeySm, fSym, fSymSm, fStep;
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
    RoundRect(dc, r.left, r.top + 3, r.right, r.bottom + 3, 9, 9);
    SelectObject(dc, oldBr);
    DeleteObject(shb);

    // yuza
    HPEN   pen = CreatePen(PS_SOLID, 1, accent ? C_ACCENT : C_CAPBRD);
    HBRUSH br  = CreateSolidBrush(accent ? C_ACC_BG : RGB(255,255,255));
    SelectObject(dc, pen);
    SelectObject(dc, br);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, 9, 9);
    SelectObject(dc, oldPen);
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

// tugmalar ketma-ketligini markazga chizadi
static void DrawSeq(HDC dc, int cy, int capH, const SeqItem* it, int n,
                    HFONT fKey, HFONT fSym)
{
    int w[16], total = 0;
    const int gap = 7;
    const int minCap = (capH > 38) ? 46 : 32;
    const int padCap = (capH > 38) ? 24 : 16;
    for (int i = 0; i < n; ++i) {
        if (it[i].kind == 2) {
            w[i] = MeasW(dc, it[i].t, fSym) + 10;
        } else {
            w[i] = MeasW(dc, it[i].t, fKey) + padCap;
            if (w[i] < minCap) w[i] = minCap;
        }
        total += w[i];
        if (i) total += gap;
    }
    int x = (CW - total) / 2;
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

// 4 ta harf jadvali (Qadam 3)
static void DrawLetterTable(HDC dc, const wchar_t* trig, TutState* st)
{
    struct { const wchar_t* key; const wchar_t* res; } rows[] = {
        { L"o", L"ö" }, { L"g", L"ğ" }, { L"s", L"ş" }, { L"c", L"ç" },
    };
    int y = 96;
    for (int i = 0; i < 4; ++i) {
        SeqItem seq[] = {
            { trig,        0 }, { L"+", 2 },
            { rows[i].key, 0 }, { L"→", 2 }, { rows[i].res, 1 },
        };
        DrawSeq(dc, y, 30, seq, 5, st->fKeySm, st->fSymSm);
        y += 34;
    }
}

// tray ikonka + o'ng-tugma menyusi (Qadam 4)
static void DrawTrayHint(HDC dc, TutState* st)
{
    int iconY = 118;
    HICON ic = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON),
                                 IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
    if (ic) {
        DrawIconEx(dc, 96, iconY, ic, 48, 48, 0, NULL, DI_NORMAL);
        DestroyIcon(ic);
    }

    // strelka
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(128, 134, 146));
    HGDIOBJ o = SelectObject(dc, st->fSym);
    RECT ar = { 150, iconY, 190, iconY + 48 };
    DrawTextW(dc, L"→", -1, &ar, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, o);

    // menyu qutisi
    RECT m = { 202, 92, 384, 188 };
    HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
    HBRUSH shb = CreateSolidBrush(C_SHADOW);
    HGDIOBJ ob = SelectObject(dc, shb);
    RoundRect(dc, m.left, m.top + 3, m.right, m.bottom + 3, 8, 8);
    SelectObject(dc, ob); DeleteObject(shb);

    HPEN   pen = CreatePen(PS_SOLID, 1, C_CAPBRD);
    HBRUSH br  = CreateSolidBrush(RGB(255, 255, 255));
    SelectObject(dc, pen); SelectObject(dc, br);
    RoundRect(dc, m.left, m.top, m.right, m.bottom, 8, 8);
    SelectObject(dc, op); DeleteObject(pen); DeleteObject(br);

    // birinchi qator — "Sozlamalar" ajratib ko'rsatiladi
    RECT hl = { m.left + 4, m.top + 6, m.right - 4, m.top + 28 };
    HBRUSH hb = CreateSolidBrush(C_ACC_BG);
    FillRect(dc, &hl, hb);
    DeleteObject(hb);

    const wchar_t* items[] = { T(S_MENU_SETTINGS), T(S_MENU_LANGUAGE), T(S_MENU_TUTORIAL) };
    HGDIOBJ of = SelectObject(dc, st->fBody);
    for (int i = 0; i < 3; ++i) {
        RECT ir = { m.left + 12, m.top + 6 + i*24, m.right - 8, m.top + 28 + i*24 };
        SetTextColor(dc, i == 0 ? C_ACC_TXT : C_BODY);
        DrawTextW(dc, items[i], -1, &ir, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    SelectObject(dc, of);
}

static void PaintTutorial(HWND hwnd, HDC dc, TutState* st)
{
    // fon
    RECT full = { 0, 0, CW, CH };
    HBRUSH bg = CreateSolidBrush(C_BG);
    FillRect(dc, &full, bg);
    DeleteObject(bg);

    SetBkMode(dc, TRANSPARENT);

    // sarlavha
    StrId titleId[STEP_COUNT] = { S_TUT_1_TITLE, S_TUT_2_TITLE, S_TUT_3_TITLE, S_TUT_4_TITLE };
    StrId bodyId [STEP_COUNT] = { S_TUT_1_BODY,  S_TUT_2_BODY,  S_TUT_3_BODY,  S_TUT_4_BODY  };

    HGDIOBJ of = SelectObject(dc, st->fTitle);
    SetTextColor(dc, C_TITLE);
    RECT tr = { 24, 22, CW - 24, 58 };
    DrawTextW(dc, T(titleId[st->step]), -1, &tr, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    SelectObject(dc, of);

    // qadam raqami (o'ng yuqori)
    wchar_t sc[32];
    wsprintfW(sc, T(S_TUT_STEP), st->step + 1, STEP_COUNT);
    of = SelectObject(dc, st->fStep);
    SetTextColor(dc, C_STEPTXT);
    RECT srr = { CW - 120, 20, CW - 20, 34 };
    DrawTextW(dc, sc, -1, &srr, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
    SelectObject(dc, of);

    // illyustratsiya
    wchar_t trig[24];
    TriggerKeyCap(g_settings.triggerVk, trig, 24);

    if (st->step == 0) {
        SeqItem seq[] = { { trig,0 }, { L"+",2 }, { L"O",0 }, { L"→",2 }, { L"ö",1 } };
        DrawSeq(dc, 138, 50, seq, 5, st->fKey, st->fSym);
    } else if (st->step == 1) {
        SeqItem seq[] = { { L"Shift",0 }, { L"+",2 }, { trig,0 }, { L"+",2 },
                          { L"O",0 }, { L"→",2 }, { L"Ö",1 } };
        DrawSeq(dc, 138, 50, seq, 7, st->fKey, st->fSym);
    } else if (st->step == 2) {
        DrawLetterTable(dc, trig, st);
    } else {
        DrawTrayHint(dc, st);
    }

    // tavsif matni
    of = SelectObject(dc, st->fBody);
    SetTextColor(dc, C_BODY);
    RECT br = { 34, 240, CW - 34, 300 };
    DrawTextW(dc, T(bodyId[st->step]), -1, &br,
              DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(dc, of);

    // qadam nuqtalari
    int dotY = 316, r = 5, span = STEP_COUNT * 18 - 6;
    int x0 = (CW - span) / 2;
    HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
    for (int i = 0; i < STEP_COUNT; ++i) {
        HBRUSH d = CreateSolidBrush(i == st->step ? C_ACCENT : C_DOT_OFF);
        HGDIOBJ ob = SelectObject(dc, d);
        int cx = x0 + i * 18;
        Ellipse(dc, cx, dotY - r, cx + 2*r, dotY + r);
        SelectObject(dc, ob);
        DeleteObject(d);
    }
    SelectObject(dc, op);
}

// tugmalar matni/holatini joriy qadamga moslaydi
static void UpdateButtons(HWND hwnd, TutState* st)
{
    SetDlgItemTextW(hwnd, ID_BACK, T(S_TUT_BACK));
    SetDlgItemTextW(hwnd, ID_NEXT,
                    st->step == STEP_COUNT - 1 ? T(S_TUT_FINISH) : T(S_TUT_NEXT));
    EnableWindow(GetDlgItem(hwnd, ID_BACK), st->step > 0);
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
        st->fTitle = CreateFontW(-24,0,0,0,700,0,0,0,DEFAULT_CHARSET,0,0,
                                 CLEARTYPE_QUALITY,0,L"Segoe UI");
        st->fBody  = CreateFontW(-15,0,0,0,400,0,0,0,DEFAULT_CHARSET,0,0,
                                 CLEARTYPE_QUALITY,0,L"Segoe UI");
        st->fKey   = CreateFontW(-20,0,0,0,600,0,0,0,DEFAULT_CHARSET,0,0,
                                 CLEARTYPE_QUALITY,0,L"Segoe UI");
        st->fKeySm = CreateFontW(-15,0,0,0,600,0,0,0,DEFAULT_CHARSET,0,0,
                                 CLEARTYPE_QUALITY,0,L"Segoe UI");
        st->fSym   = CreateFontW(-22,0,0,0,400,0,0,0,DEFAULT_CHARSET,0,0,
                                 CLEARTYPE_QUALITY,0,L"Segoe UI");
        st->fSymSm = CreateFontW(-16,0,0,0,400,0,0,0,DEFAULT_CHARSET,0,0,
                                 CLEARTYPE_QUALITY,0,L"Segoe UI");
        st->fStep  = CreateFontW(-12,0,0,0,400,0,0,0,DEFAULT_CHARSET,0,0,
                                 CLEARTYPE_QUALITY,0,L"Segoe UI");
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);

        int by = CH - 32 - 18;
        HWND b = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP,
                     24, by, 120, 32, hwnd, (HMENU)ID_BACK, g_hInst, NULL);
        HWND nx = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
                     CW - 24 - 140, by, 140, 32, hwnd, (HMENU)ID_NEXT, g_hInst, NULL);
        SendMessageW(b,  WM_SETFONT, (WPARAM)st->fBody, TRUE);
        SendMessageW(nx, WM_SETFONT, (WPARAM)st->fBody, TRUE);
        UpdateButtons(hwnd, st);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wp) == ID_BACK) {
            if (st->step > 0) { st->step--; UpdateButtons(hwnd, st);
                                InvalidateRect(hwnd, NULL, TRUE); }
        } else if (LOWORD(wp) == ID_NEXT) {
            if (st->step < STEP_COUNT - 1) { st->step++; UpdateButtons(hwnd, st);
                                             InvalidateRect(hwnd, NULL, TRUE); }
            else DestroyWindow(hwnd);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;   // WM_PAINT o'zi to'ldiradi (miltillashning oldini olamiz)

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        // ikki bosqichli (off-screen) chizish — silliq ko'rinish uchun
        HDC   mem = CreateCompatibleDC(dc);
        HBITMAP bmp = CreateCompatibleBitmap(dc, CW, CH);
        HGDIOBJ ob = SelectObject(mem, bmp);
        PaintTutorial(hwnd, mem, st);
        BitBlt(dc, 0, 0, CW, CH, mem, 0, 0, SRCCOPY);
        SelectObject(mem, ob);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCDESTROY:
        if (st) {
            DeleteObject(st->fTitle); DeleteObject(st->fBody);
            DeleteObject(st->fKey);   DeleteObject(st->fKeySm);
            DeleteObject(st->fSym);   DeleteObject(st->fSymSm);
            DeleteObject(st->fStep);
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

    DWORD style   = WS_CAPTION | WS_SYSMENU;
    DWORD exstyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    RECT rc = { 0, 0, CW, CH };
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
