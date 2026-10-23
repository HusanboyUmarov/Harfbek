// Tanishtiruv (birinchi ishga tushish wizard'i) — yangi dizayn: mavzuga mos,
// AA chizilgan 6 sahifa. Sahifalar orasida kontent sirpanib + so'nib almashadi
// (220 ms), 1-sahifada harf juftliklari ketma-ket paydo bo'ladi, 3-sahifada
// kichik "yozish" animatsiyasi bor. 4-sahifada ishlash rejimi kartochka bilan
// tanlanadi ("Tugatish" bosilganda saqlanadi; Esc / X bilan yopilsa o'zgarmaydi).
// Hamma narsa o'zimiz chizamiz (child oyna yo'q): hover/press/fokus holatlari
// oynaning o'zida; taymer faqat animatsiya ketayotganda ishlaydi.
// Barcha piksel o'lchamlari 96 DPI uchun yozilgan va S() orqali system DPI
// ga masshtablanadi (manifestda dpiAware=true).
#include "app.h"

#define STEP_COUNT 6
#define STEP_MODE  3        // rejim tanlash sahifasi (0 dan boshlab)

// Boshqariladigan elementlar (fokus / hover / bosish uchun)
enum { IT_NONE = -1, IT_BACK = 0, IT_NEXT, IT_CARD0, IT_CARD1, IT_TRY, IT_COUNT };

// "Hoziroq sinab köring" bosilganda true: wizard yopilgach Sozlamalarning sinov sahifasi ochiladi
static bool s_tryNow = false;

// Oyna ichki o'lchami va kontent joylashuvi (px, 96 DPI)
static const int CW96  = 640;
static const int CH96  = 480;
static const int CT96  = 52;     // kontent yuqori chegarasi (tepada progress)
static const int CHT96 = 360;    // kontent balandligi

// Sahifa animatsiyalari davomiyligi (ms)
static const int TIME_PAIR = 520;    // 1-sahifa: harf juftliklari
static const int TIME_TYPE = 1500;   // 3-sahifa: yozish animatsiyasi

// System DPI va unga masshtablangan oyna ichki o'lchami (ShowTutorial to'ldiradi)
static int s_dpi = 96;
static int s_cw  = CW96;
static int s_ch  = CH96;

// Sahifa koordinatalari boshi (qurilma pikselida): jonli chizishda (0, kontent
// tepasi), qatlamga chizishda (0, 0)
static int s_ox = 0;
static int s_oy = 0;

// 96 DPI dagi pikselni joriy DPI ga o'tkazadi
static int S(int x) { return Ui_S(x, s_dpi); }

// Sahifa koordinatasidagi (96 DPI) to'rtburchak -> qurilma pikseli
static RECT PR(int x, int y, int w, int h)
{
    RECT r = { s_ox + S(x), s_oy + S(y), s_ox + S(x + w), s_oy + S(y + h) };
    return r;
}

static int Px1() { int v = S(1); return v > 0 ? v : 1; }

// Sahifa animatsiyasi (yoki o'tish kadri) uchun vaqtinchalik DIB qatlam
struct Layer { HDC dc; HBITMAP bmp; HGDIOBJ old; DWORD* bits; int w, h; };

struct TutState {
    int       step, from, dir;      // joriy sahifa, o'tishdagi eski sahifa, yo'nalish (+1 oldinga)
    InputMode mode;                 // 4-sahifada tanlangan rejim
    int       focus, hot, down;     // fokus, hover va bosilgan element
    bool      kbd, tracking, transOn;
    UiAnim    pos, trans, hov[IT_COUNT], prs[IT_COUNT], sel[2];
    Layer     lA, lB, lO;           // o'tish paytida: eski / yangi / natija
    DWORD     t0[2];                // sahifa animatsiyalari boshlangan vaqt
    bool      started[2], done[2];
    HFONT     fTitle, fTitleSm, fBody, fBodySm, fKeySm, fKey, fRes0, fResSm, fRes,
              fSymSm, fSym, fBtn, fStep, fCard, fCardSm, fType, fIcon;
};

// ---- qatlam (DIB) yordamchilari ----
static bool Layer_Create(HDC ref, Layer& L, int w, int h)
{
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = w;
    bi.bmiHeader.biHeight      = -h;        // yuqoridan pastga
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    L.dc = CreateCompatibleDC(ref);
    void* p = NULL;
    L.bmp = L.dc ? CreateDIBSection(L.dc, &bi, DIB_RGB_COLORS, &p, NULL, 0) : NULL;
    if (!L.bmp || !p) {
        if (L.bmp) DeleteObject(L.bmp);
        if (L.dc)  DeleteDC(L.dc);
        L = Layer();
        return false;
    }
    L.old  = SelectObject(L.dc, L.bmp);
    L.bits = (DWORD*)p;
    L.w = w; L.h = h;
    return true;
}

static void Layer_Free(Layer& L)
{
    if (L.dc) {
        SelectObject(L.dc, L.old);
        DeleteObject(L.bmp);
        DeleteDC(L.dc);
    }
    L = Layer();
}

static void FillColor(HDC dc, const RECT& r, COLORREF c)
{
    SetDCBrushColor(dc, c);
    FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
}

// COLORREF (0x00BBGGRR) -> DIB pikseli (0x00RRGGBB)
static DWORD PxOf(COLORREF c)
{
    return ((DWORD)GetRValue(c) << 16) | ((DWORD)GetGValue(c) << 8) | GetBValue(c);
}

static float Clamp01(float v) { return v < 0 ? 0.f : (v > 1 ? 1.f : v); }

// qatlamni fon rangi tomon so'ndirib (a: 0 = ko'rinmas, 1 = to'liq) dc ga ko'chiradi
static void FadeBlit(HDC dst, Layer& L, int x, int y, float a)
{
    GdiFlush();
    DWORD bg = PxOf(Ui_Theme().bg);
    int br = (bg >> 16) & 255, bgc = (bg >> 8) & 255, bb = bg & 255;
    int a256 = (int)(Clamp01(a) * 256.f + 0.5f);
    int n = L.w * L.h;
    for (int i = 0; i < n; ++i) {
        DWORD p = L.bits[i];
        int r = br  + ((((int)((p >> 16) & 255) - br)  * a256) >> 8);
        int g = bgc + ((((int)((p >> 8) & 255)  - bgc) * a256) >> 8);
        int b = bb  + ((((int)(p & 255)         - bb)  * a256) >> 8);
        L.bits[i] = ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
    }
    BitBlt(dst, x, y, L.w, L.h, L.dc, 0, 0, SRCCOPY);
}

// ---- matn o'lchami ----
static int MeasW(HDC dc, const wchar_t* t, HFONT f)
{
    HGDIOBJ o = SelectObject(dc, f);
    SIZE s; GetTextExtentPoint32W(dc, t, lstrlenW(t), &s);
    SelectObject(dc, o);
    return s.cx;
}

// "◀ Orqaga" / "Keyingi ▶" dan o'qlarni olib tashlaydi (tugmalar o'zimizniki)
static void CleanLabel(const wchar_t* in, wchar_t* out, int n)
{
    int j = 0;
    for (; *in && j < n - 1; ++in) {
        if (*in == L'◀' || *in == L'▶') continue;
        out[j++] = *in;
    }
    out[j] = 0;
    while (j > 0 && out[j - 1] == L' ') out[--j] = 0;
    int s = 0;
    while (out[s] == L' ') ++s;
    if (s) {
        int k = 0;
        while ((out[k] = out[k + s]) != 0) ++k;
    }
}

// ---- tugmalar (keycap) ketma-ketligi ----
// kind: 0 = tugma, 1 = natija harfi (accent, katta), 2 = belgi (+ →), 3 = ikonka tugma
// minW (96 DPI) — tugmaning eng kichik kengligi (0 = shriftga mos standart)
struct SeqItem { const wchar_t* t; int kind; int minW; };

struct Fnts { HFONT key, res, sym; int minCap, pad, gap; };

// tugma balandligiga qarab shriftlar to'plami
static Fnts PickFonts(TutState* st, int capH)
{
    Fnts f;
    if (capH <= S(38)) {
        f.key = st->fKeySm; f.res = st->fRes0;  f.sym = st->fSymSm;
        f.minCap = S(34); f.pad = S(20); f.gap = S(6);
    } else if (capH < S(54)) {
        f.key = st->fKey;   f.res = st->fResSm; f.sym = st->fSym;
        f.minCap = S(46); f.pad = S(28); f.gap = S(10);
    } else {
        f.key = st->fKey;   f.res = st->fRes;   f.sym = st->fSym;
        f.minCap = S(58); f.pad = S(34); f.gap = S(10);
    }
    return f;
}

static int SeqMeasure(HDC dc, int capH, const SeqItem* it, int n, int* w, TutState* st)
{
    Fnts f = PickFonts(st, capH);
    int total = 0;
    for (int i = 0; i < n; ++i) {
        switch (it[i].kind) {
        case 2:  w[i] = MeasW(dc, it[i].t, f.sym) + (capH <= S(38) ? S(8) : S(14)); break;
        case 1:  w[i] = MeasW(dc, it[i].t, f.res) + S(8);  break;
        case 3:  w[i] = f.minCap + S(10);                  break;
        default:
            w[i] = MeasW(dc, it[i].t, f.key) + f.pad;
            int mn = it[i].minW ? S(it[i].minW) : f.minCap;
            if (w[i] < mn) w[i] = mn;
        }
        total += w[i];
        if (i) total += f.gap;
    }
    return total;
}

// ketma-ketlikni (cx, cy) markazga chizadi (qurilma pikseli)
static void DrawSeq(HDC dc, int cx, int cy, int capH, const SeqItem* it, int n, TutState* st)
{
    const UiTheme& th = Ui_Theme();
    Fnts f = PickFonts(st, capH);
    int w[16];
    int total = SeqMeasure(dc, capH, it, n, w, st);
    int x = cx - total / 2;
    for (int i = 0; i < n; ++i) {
        RECT r = { x, cy - capH / 2, x + w[i], cy + capH / 2 };
        switch (it[i].kind) {
        case 0: Ui_DrawKeycap(dc, r, it[i].t, f.key, false); break;
        case 3: Ui_DrawKeycap(dc, r, it[i].t, st->fIcon, false); break;
        case 1:
            r.top -= S(12); r.bottom += S(12);
            Ui_Text(dc, it[i].t, r, f.res, th.accent,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
            break;
        default:
            Ui_Text(dc, it[i].t, r, f.sym, th.textDim,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
        }
        x += w[i] + f.gap;
    }
}

// ---- 1-sahifa: o' g' sh ch -> ö ğ ş ç (juftliklar ketma-ket paydo bo'ladi) ----
static void DrawPair(HDC dc, TutState* st, int i, float a)
{
    static const struct { const wchar_t* k; const wchar_t* r; } kPairs[4] = {
        { L"o'", L"ö" }, { L"g'", L"ğ" }, { L"sh", L"ş" }, { L"ch", L"ç" },
    };
    if (a <= 0) return;
    int cx = s_ox + S(i % 2 ? 450 : 190);
    int cy = s_oy + S(i / 2 ? 296 : 190);
    SeqItem it[3] = { { kPairs[i].k, 0, 76 }, { L"→", 2 }, { kPairs[i].r, 1 } };   // bir xil kenglikdagi tugmalar
    if (a >= 1) { DrawSeq(dc, cx, cy, S(60), it, 3, st); return; }

    // paydo bo'layotgan juftlik: alohida qatlamga chizib, fon tomon so'ndiramiz
    int lw = S(250), lh = S(96);
    Layer L = Layer();
    if (!Layer_Create(dc, L, lw, lh)) { DrawSeq(dc, cx, cy, S(60), it, 3, st); return; }
    RECT full = { 0, 0, lw, lh };
    FillColor(L.dc, full, Ui_Theme().bg);
    DrawSeq(L.dc, lw / 2, lh / 2, S(60), it, 3, st);
    FadeBlit(dc, L, cx - lw / 2, cy - lh / 2 + (int)((1.f - a) * S(14)), a);
    Layer_Free(L);
}

static void DrawAlphabet(HDC dc, TutState* st, int el)
{
    for (int i = 0; i < 4; ++i) {
        float a = Clamp01((float)(el - i * 60) / 260.f);
        a = 1.f - (1.f - a) * (1.f - a) * (1.f - a);        // ease-out
        DrawPair(dc, st, i, a);
    }
}

// ---- 2-sahifa: trigger + harf (va Shift bilan katta harf) ----
static void DrawTriggerDemo(HDC dc, const wchar_t* trig, TutState* st)
{
    // trigger o'ng Shift bo'lsa birinchi tugma chap Shift ekani ko'rinsin
    const wchar_t* shiftCap = g_settings.triggerVk == VK_RSHIFT ? L"L Shift" : L"Shift";
    SeqItem a[] = { { trig,0 }, { L"+",2 }, { L"O",0 }, { L"→",2 }, { L"ö",1 } };
    SeqItem b[] = { { shiftCap,0 }, { L"+",2 }, { trig,0 }, { L"+",2 },
                    { L"O",0 }, { L"→",2 }, { L"Ö",1 } };
    DrawSeq(dc, s_ox + S(320), s_oy + S(188), S(58), a, 5, st);
    DrawSeq(dc, s_ox + S(320), s_oy + S(292), S(58), b, 7, st);
}

// ---- 3-sahifa: avto rejim — "yozish" animatsiyasi + ikki misol ----
static void DrawAutoDemo(HDC dc, TutState* st, int el)
{
    const UiTheme& th = Ui_Theme();

    // mock yozuv maydoni
    RECT fld = PR(120, 142, 400, 56);
    Ui_FillRound(dc, fld, S(8), th.surface);
    Ui_DrawInputFrame(dc, fld, true, 0);

    // s -> h -> (ş ga almashadi)
    float sA  = Clamp01((float)(el - 250)  / 150.f);
    float hA  = Clamp01((float)(el - 600)  / 150.f);
    float cvt = Clamp01((float)(el - 1000) / 220.f);       // ş ga almashish
    int ws  = MeasW(dc, L"s", st->fType);
    int wh  = MeasW(dc, L"h", st->fType);
    int xs  = (fld.left + fld.right) / 2 - (ws + wh) / 2;
    UINT fl = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP;
    // ascender'siz harflar ko'zga pastroq ko'rinadi — matnni biroz yuqoriga suramiz
    RECT tx = fld;
    tx.top -= S(6); tx.bottom -= S(6);
    if (cvt < 1.f) {
        RECT rs = { xs, tx.top, xs + ws + S(4), tx.bottom };
        RECT rh = { xs + ws, tx.top, xs + ws + wh + S(4), tx.bottom };
        Ui_Text(dc, L"s", rs, st->fType, Ui_Mix(th.surface, th.text, sA * (1.f - cvt)), fl);
        Ui_Text(dc, L"h", rh, st->fType, Ui_Mix(th.surface, th.text, hA * (1.f - cvt)), fl);
    }
    if (cvt > 0) {
        Ui_Text(dc, L"ş", tx, st->fType, Ui_Mix(th.surface, th.accent, cvt),
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
    } else {
        // yozish kursori (ş paydo bo'lgach yo'qoladi)
        int cx = xs + (hA > 0 ? ws + wh : (sA > 0 ? ws : 0)) + S(4);
        RECT car = { cx, fld.top + S(13), cx + S(2), fld.bottom - S(15) };
        Ui_FillRound(dc, car, 1, th.accent);
    }

    // misollar: ikki harf ketma-ket; Backspace bilan qaytarish
    SeqItem a[] = { { L"O",0 }, { L"'",0 }, { L"→",2 }, { L"ö",1 } };
    SeqItem b[] = { { L"ş",1 }, { L"",3 }, { L"→",2 }, { L"sh",0 } };
    DrawSeq(dc, s_ox + S(320), s_oy + S(256), S(44), a, 4, st);
    DrawSeq(dc, s_ox + S(320), s_oy + S(322), S(44), b, 4, st);
}

// ---- 4-sahifa: rejim kartochkalari ----
static RECT CardRectAt(int i, int oy)
{
    RECT r;
    r.left   = S(56 + i * 276);
    r.right  = r.left + S(252);
    r.top    = oy + S(140);
    r.bottom = r.top + S(200);
    return r;
}

static void DrawModeCards(HDC dc, const wchar_t* trig, TutState* st)
{
    const UiTheme& th = Ui_Theme();
    for (int i = 0; i < 2; ++i) {
        RECT rc = CardRectAt(i, s_oy);
        rc.left += s_ox; rc.right += s_ox;
        float sel = st->sel[i].value;
        Ui_DrawCard(dc, rc, sel > 0.02f, st->hov[IT_CARD0 + i].value, sel);

        // mini illyustratsiya
        int cx = (rc.left + rc.right) / 2;
        if (i == 0) {
            SeqItem it[] = { { trig,0 }, { L"+",2 }, { L"O",0 }, { L"→",2 }, { L"ö",1 } };
            DrawSeq(dc, cx, rc.top + S(56), S(34), it, 5, st);
        } else {
            SeqItem it[] = { { L"S",0 }, { L"H",0 }, { L"→",2 }, { L"ş",1 } };
            DrawSeq(dc, cx, rc.top + S(56), S(34), it, 4, st);
        }

        // sarlavha: S_MODE_AUTO dagi ":" dan oldingi qismi; izoh — misol matni
        wchar_t title[96], desc[160];
        if (i == 0) {
            lstrcpynW(title, T(S_MODE_TRIGGER), 96);
            wsprintfW(desc, T(S_SET_EXAMPLE_TRIGGER), trig);
        } else {
            lstrcpynW(title, T(S_MODE_AUTO), 96);
            for (int k = 0; title[k]; ++k) if (title[k] == L':') { title[k] = 0; break; }
            lstrcpynW(desc, T(S_SET_EXAMPLE_AUTO), 160);
        }
        RECT tr = { rc.left + S(14), rc.top + S(104), rc.right - S(14), rc.top + S(134) };
        HFONT cf = MeasW(dc, title, st->fCard) > tr.right - tr.left ? st->fCardSm : st->fCard;
        Ui_Text(dc, title, tr, cf, th.text,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        RECT dr = { rc.left + S(16), rc.top + S(138), rc.right - S(16), rc.bottom - S(14) };
        Ui_Text(dc, desc, dr, st->fBodySm, th.textDim,
                DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);

        if (st->kbd && st->focus == IT_CARD0 + i) {
            RECT fr = rc;
            InflateRect(&fr, S(4), S(4));
            Ui_DrawFocusRing(dc, fr, S(12));
        }
    }
}

// ---- 5-sahifa: tezkor tugmalar (bo'sh hotkey qatori ko'rsatilmaydi) ----
static void DrawHotkeys(HDC dc, TutState* st)
{
    const UiTheme& th = Ui_Theme();
    struct { StrId label; WORD hk; } rows[3] = {
        { S_HK_MODE,    g_settings.hkMode    },
        { S_HK_ENABLE,  g_settings.hkEnable  },
        { S_HK_CONVERT, g_settings.hkConvert },
    };
    int cnt = 0;
    for (int i = 0; i < 3; ++i) if (rows[i].hk) ++cnt;
    if (!cnt) return;

    const int rowH = S(56), gapY = S(14);
    int y = s_oy + S(243) - (cnt * rowH + (cnt - 1) * gapY) / 2;
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
                if (n) { seq[n].t = L"+"; seq[n].kind = 2; seq[n].minW = 0; ++n; }
                seq[n].t = part[np]; seq[n].kind = 0; seq[n].minW = 44; ++n;   // F9/F10/F11 bir xil kenglikda
                ++np;
            }
            while (*p && *p != L'+') ++p;
            if (*p == L'+') ++p;
        }

        RECT pr = { s_ox + S(60), y, s_ox + S(580), y + rowH };
        Ui_FillRound(dc, pr, S(12), th.surface);
        Ui_StrokeRound(dc, pr, S(12), th.border, Px1());

        // yorliq (oxiridagi ":" siz)
        wchar_t lbl[96];
        lstrcpynW(lbl, T(rows[i].label), 96);
        for (int k = lstrlenW(lbl); k > 0 && lbl[k - 1] == L':'; --k) lbl[k - 1] = 0;
        RECT lr = { pr.left + S(22), pr.top, pr.left + S(280), pr.bottom };
        HFONT lf = MeasW(dc, lbl, st->fBody) > lr.right - lr.left ? st->fBodySm : st->fBody;
        Ui_Text(dc, lbl, lr, lf, th.text,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

        if (n) {
            int w[16];
            int total = SeqMeasure(dc, S(34), seq, n, w, st);
            DrawSeq(dc, pr.right - S(22) - total / 2, (pr.top + pr.bottom) / 2, S(34), seq, n, st);
        }
        y += rowH + gapY;
    }
}

// ---- 6-sahifa: tray ikonka + o'ng-tugma menyusi ----
static void DrawTrayHint(HDC dc, TutState* st)
{
    const UiTheme& th = Ui_Theme();

    // ikonka (accent tusli doira ichida)
    int icx = s_ox + S(168), icy = s_oy + S(243);
    Ui_FillCircle(dc, icx, icy, S(54), th.accent, 36);
    int sz = S(64);
    HICON ic = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON),
                                 IMAGE_ICON, sz, sz, LR_DEFAULTCOLOR);
    if (ic) {
        DrawIconEx(dc, icx - sz / 2, icy - sz / 2, ic, sz, sz, 0, NULL, DI_NORMAL);
        DestroyIcon(ic);
    }

    // strelka
    RECT ar = { s_ox + S(236), icy - S(20), s_ox + S(292), icy + S(20) };
    Ui_Text(dc, L"→", ar, st->fSym, th.textDim,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // menyu kartochkasi
    RECT m = PR(314, 176, 244, 134);
    Ui_Shadow(dc, m, S(10), S(10), 46);
    Ui_FillRound(dc, m, S(10), th.surface);
    Ui_StrokeRound(dc, m, S(10), th.border, Px1());

    const wchar_t* items[3] = { T(S_MENU_SETTINGS), T(S_MENU_LANGUAGE), T(S_MENU_TUTORIAL) };
    int rowH = S(38), pad = S(8);
    for (int i = 0; i < 3; ++i) {
        RECT ir = { m.left + pad, m.top + pad + i * rowH, m.right - pad, m.top + pad + (i + 1) * rowH - S(2) };
        if (i == 0) Ui_FillRound(dc, ir, S(6), th.accent, 40);        // "Sozlamalar" ajratiladi
        RECT tr = ir; tr.left += S(12); tr.right -= S(8);
        Ui_Text(dc, items[i], tr, st->fBody, i == 0 ? th.accent : th.text,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
}

// ---- sahifa: sarlavha + tavsif + illyustratsiya ----
static int AnimIdx(int page) { return page == 0 ? 0 : (page == 2 ? 1 : -1); }

// sahifa animatsiyasining o'tgan vaqti (ms): boshlanmagan — 0, tugagan — katta qiymat
static int AnimEl(TutState* st, int page)
{
    int k = AnimIdx(page);
    if (k < 0) return 0;
    if (st->done[k]) return 100000;
    if (st->started[k]) return (int)(GetTickCount() - st->t0[k]);
    return 0;
}

static void DrawPage(HDC dc, TutState* st, int page)
{
    const UiTheme& th = Ui_Theme();
    static const StrId titleId[STEP_COUNT] = { S_WIZ_1_TITLE, S_WIZ_2_TITLE, S_WIZ_3_TITLE,
                                               S_WIZ_4_TITLE, S_WIZ_5_TITLE, S_WIZ_6_TITLE };
    static const StrId bodyId [STEP_COUNT] = { S_WIZ_1_BODY,  S_WIZ_2_BODY,  S_WIZ_3_BODY,
                                               S_WIZ_4_BODY,  S_WIZ_5_BODY,  S_WIZ_6_BODY  };

    // sarlavha (uzun bo'lsa kichikroq shrift)
    const wchar_t* title = T(titleId[page]);
    RECT tr = PR(32, 6, 576, 40);
    HFONT tf = MeasW(dc, title, st->fTitle) > tr.right - tr.left ? st->fTitleSm : st->fTitle;
    Ui_Text(dc, title, tr, tf, th.text,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

    // tavsif (sig'masa kichikroq shrift)
    const wchar_t* body = T(bodyId[page]);
    RECT br = PR(60, 52, 520, 72);
    RECT mr = br;
    HGDIOBJ of = SelectObject(dc, st->fBody);
    DrawTextW(dc, body, -1, &mr, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
    SelectObject(dc, of);
    HFONT bf = (mr.bottom - mr.top > br.bottom - br.top) ? st->fBodySm : st->fBody;
    Ui_Text(dc, body, br, bf, th.textDim, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);

    // illyustratsiya
    wchar_t trig[24];
    TriggerKeyCap(g_settings.triggerVk, trig, 24);
    switch (page) {
    case 0:         DrawAlphabet(dc, st, AnimEl(st, 0));     break;
    case 1:         DrawTriggerDemo(dc, trig, st);           break;
    case 2:         DrawAutoDemo(dc, st, AnimEl(st, 2));     break;
    case STEP_MODE: DrawModeCards(dc, trig, st);             break;
    case 4:         DrawHotkeys(dc, st);                     break;
    default:        DrawTrayHint(dc, st);                    break;
    }
}

// ---- elementlar joylashuvi (client koordinata) ----
static RECT ItemRect(int it)
{
    int bh = S(40), by = s_ch - bh - S(20);
    RECT r;
    switch (it) {
    case IT_BACK: r.left = S(32);                r.right = S(32) + S(120); break;
    case IT_NEXT: r.left = s_cw - S(32) - S(160); r.right = s_cw - S(32);  break;
    case IT_TRY:  r.left = s_cw - S(32) - S(160) - S(12) - S(210);         // "Tugatish" ning chap tomonida
                  r.right = r.left + S(210);                                break;
    default:      return CardRectAt(it - IT_CARD0, S(CT96));
    }
    r.top = by; r.bottom = by + bh;
    return r;
}

static int Hit(TutState* st, int x, int y)
{
    POINT p = { x, y };
    if (st->step == STEP_MODE && !st->transOn)
        for (int i = 0; i < 2; ++i) {
            RECT r = ItemRect(IT_CARD0 + i);
            if (PtInRect(&r, p)) return IT_CARD0 + i;
        }
    RECT rb = ItemRect(IT_BACK), rn = ItemRect(IT_NEXT), rt = ItemRect(IT_TRY);
    if (st->step == STEP_COUNT - 1 && PtInRect(&rt, p)) return IT_TRY;
    if (st->step > 0 && PtInRect(&rb, p)) return IT_BACK;
    if (PtInRect(&rn, p)) return IT_NEXT;
    return IT_NONE;
}

// ---- o'tish kadri: eski sahifa chapga/o'ngga sirpanib so'nadi, yangisi kelib paydo bo'ladi ----
static void ComposeFrame(TutState* st)
{
    Layer& A = st->lA; Layer& B = st->lB; Layer& O = st->lO;
    int w = O.w, h = O.h;
    float e = Clamp01(st->trans.value);
    int shift = S(40);
    int dxA = (int)(-st->dir * shift * e);              // eski: harakat yo'nalishida ketadi
    int dxB = (int)(st->dir * shift * (1.f - e));       // yangi: qarshi tomondan keladi
    // eski tez so'nadi, yangisi biroz kechikib paydo bo'ladi — ikkalasi bir-biriga aralashib ketmaydi
    int aA = (int)(Clamp01(1.f - e * 1.8f) * 256.f);
    int aB = (int)(Clamp01((e - 0.25f) / 0.75f) * 256.f);
    DWORD bg = PxOf(Ui_Theme().bg);
    int br = (bg >> 16) & 255, bgc = (bg >> 8) & 255, bb = bg & 255;
    for (int y = 0; y < h; ++y) {
        const DWORD* ra = A.bits + y * w;
        const DWORD* rb = B.bits + y * w;
        DWORD* ro = O.bits + y * w;
        for (int x = 0; x < w; ++x) {
            DWORD pa = bg, pb = bg;
            int xa = x - dxA, xb = x - dxB;
            if (xa >= 0 && xa < w) pa = ra[xa];
            if (xb >= 0 && xb < w) pb = rb[xb];
            int r = br  + ((((int)((pa >> 16) & 255) - br)  * aA + ((int)((pb >> 16) & 255) - br)  * aB) >> 8);
            int g = bgc + ((((int)((pa >> 8) & 255)  - bgc) * aA + ((int)((pb >> 8) & 255)  - bgc) * aB) >> 8);
            int b = bb  + ((((int)(pa & 255)         - bb)  * aA + ((int)(pb & 255)         - bb)  * aB) >> 8);
            ro[x] = ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
        }
    }
}

static void PaintTopBar(HDC dc, TutState* st)
{
    const UiTheme& th = Ui_Theme();

    // progress: faol qadam cho'zilgan "pill", qolganlari nuqta
    int h = S(8), cy = S(26), gap = S(8);
    int w[STEP_COUNT], total = 0;
    float k[STEP_COUNT];
    for (int i = 0; i < STEP_COUNT; ++i) {
        float d = (float)i - st->pos.value;
        if (d < 0) d = -d;
        k[i] = Clamp01(1.f - d);
        w[i] = S(8) + (int)(S(22) * k[i] + 0.5f);
        total += w[i];
        if (i) total += gap;
    }
    int x = (s_cw - total) / 2;
    for (int i = 0; i < STEP_COUNT; ++i) {
        RECT r = { x, cy - h / 2, x + w[i], cy + h / 2 };
        Ui_FillRound(dc, r, h / 2, Ui_Mix(th.border, th.accent, k[i]));
        x += w[i] + gap;
    }

    // "Qadam 3 / 6" (o'ng yuqori)
    wchar_t sc[32];
    wsprintfW(sc, T(S_TUT_STEP), st->step + 1, STEP_COUNT);
    RECT sr = { s_cw - S(170), S(16), s_cw - S(28), S(36) };
    Ui_Text(dc, sc, sr, st->fStep, th.textDim, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

static void PaintAll(HDC dc, TutState* st)
{
    RECT full = { 0, 0, s_cw, s_ch };
    FillColor(dc, full, Ui_Theme().bg);

    PaintTopBar(dc, st);

    // kontent
    if (st->transOn) {
        ComposeFrame(st);
        BitBlt(dc, 0, S(CT96), st->lO.w, st->lO.h, st->lO.dc, 0, 0, SRCCOPY);
    } else {
        s_ox = 0; s_oy = S(CT96);
        DrawPage(dc, st, st->step);
        s_oy = 0;
    }

    // tugmalar
    wchar_t back[48], next[48];
    CleanLabel(T(S_TUT_BACK), back, 48);
    CleanLabel(st->step == STEP_COUNT - 1 ? T(S_TUT_FINISH) : T(S_TUT_NEXT), next, 48);
    Ui_DrawButton(dc, ItemRect(IT_BACK), back, st->fBtn, UI_BTN_GHOST,
                  st->hov[IT_BACK].value, st->prs[IT_BACK].value, st->step > 0,
                  st->kbd && st->focus == IT_BACK && st->step > 0);
    Ui_DrawButton(dc, ItemRect(IT_NEXT), next, st->fBtn, UI_BTN_PRIMARY,
                  st->hov[IT_NEXT].value, st->prs[IT_NEXT].value, true,
                  st->kbd && st->focus == IT_NEXT);

    // oxirgi sahifada: "Hoziroq sinab köring" (tugatadi va sinov sahifasini ochadi)
    if (st->step == STEP_COUNT - 1)
        Ui_DrawButton(dc, ItemRect(IT_TRY), T(S_PG_TRY_NOW), st->fBtn, UI_BTN_SECONDARY,
                      st->hov[IT_TRY].value, st->prs[IT_TRY].value, true,
                      st->kbd && st->focus == IT_TRY);
}

// ---- animatsiya boshqaruvi ----
static void Kick(HWND hwnd) { SetTimer(hwnd, UI_ANIM_TIMER, 15, NULL); }

// sahifa ochilganda uning bir martalik animatsiyasini ishga tushiradi
static void OnArrive(HWND hwnd, TutState* st)
{
    int k = AnimIdx(st->step);
    if (k < 0 || st->started[k] || st->done[k]) return;
    if (Ui_AnimationsEnabled()) {
        st->started[k] = true;
        st->t0[k] = GetTickCount();
        Kick(hwnd);
    } else {
        st->done[k] = true;
    }
}

static void FinishTransition(HWND hwnd, TutState* st)
{
    if (!st->transOn) return;
    st->transOn = false;
    Layer_Free(st->lA); Layer_Free(st->lB); Layer_Free(st->lO);
    OnArrive(hwnd, st);
}

// true — o'tish animatsiyasi boshlandi (eski va yangi sahifa qatlamlarga chizildi)
static bool StartTransition(HWND hwnd, TutState* st)
{
    int w = s_cw, h = S(CHT96);
    HDC ref = GetDC(hwnd);
    bool ok = ref && Layer_Create(ref, st->lA, w, h) && Layer_Create(ref, st->lB, w, h) &&
              Layer_Create(ref, st->lO, w, h);
    if (ok) {
        RECT full = { 0, 0, w, h };
        int pages[2] = { st->from, st->step };
        Layer* ls[2] = { &st->lA, &st->lB };
        s_ox = s_oy = 0;
        for (int i = 0; i < 2; ++i) {
            FillColor(ls[i]->dc, full, Ui_Theme().bg);
            DrawPage(ls[i]->dc, st, pages[i]);
        }
        GdiFlush();
    } else {
        Layer_Free(st->lA); Layer_Free(st->lB); Layer_Free(st->lO);
    }
    if (ref) ReleaseDC(hwnd, ref);
    if (!ok) return false;
    st->transOn = true;
    ZeroMemory(&st->trans, sizeof(st->trans));
    Ui_AnimTo(st->trans, 1.f, 220);
    return true;
}

// barcha animatsiyalarni bir qadam suradi; hali davom etayotgan bo'lsa true
static bool TickAnims(HWND hwnd, TutState* st)
{
    bool run = false;
    for (int i = 0; i < IT_COUNT; ++i) {
        if (Ui_AnimStep(st->hov[i])) run = true;
        if (Ui_AnimStep(st->prs[i])) run = true;
    }
    if (Ui_AnimStep(st->sel[0])) run = true;
    if (Ui_AnimStep(st->sel[1])) run = true;
    if (Ui_AnimStep(st->pos))    run = true;
    if (st->transOn) {
        if (Ui_AnimStep(st->trans)) run = true;
        else FinishTransition(hwnd, st);
    }
    static const int dur[2] = { TIME_PAIR, TIME_TYPE };
    for (int k = 0; k < 2; ++k) {
        if (!st->started[k] || st->done[k]) continue;
        if ((int)(GetTickCount() - st->t0[k]) >= dur[k]) st->done[k] = true;
        else run = true;
    }
    return run;
}

// ---- 4-sahifadagi tanlovni sozlamalarga yozadi (faqat "Tugatish" da chaqiriladi) ----
static void ApplyMode(TutState* st)
{
    if (st->mode == g_settings.mode) return;
    g_settings.mode = st->mode;
    Settings_Save();
    Hook_ResetTriggerState();
}

static void SetHot(HWND hwnd, TutState* st, int it)
{
    if (it == st->hot) return;
    if (st->hot >= 0) Ui_AnimTo(st->hov[st->hot], 0.f, 160);
    st->hot = it;
    if (it >= 0) Ui_AnimTo(st->hov[it], 1.f, 160);
    Kick(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

static void SelectMode(HWND hwnd, TutState* st, int m)
{
    st->mode = m ? MODE_AUTO : MODE_TRIGGER;
    Ui_AnimTo(st->sel[0], m == 0 ? 1.f : 0.f, 200);
    Ui_AnimTo(st->sel[1], m == 1 ? 1.f : 0.f, 200);
    st->focus = IT_CARD0 + m;
    Kick(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

// bir qadam oldinga/orqaga; oxirgi qadamda "Tugatish" rejimni saqlab oynani yopadi
static void GoStep(HWND hwnd, TutState* st, int delta)
{
    int ns = st->step + delta;
    if (ns >= STEP_COUNT) { ApplyMode(st); DestroyWindow(hwnd); return; }
    if (ns < 0) return;

    FinishTransition(hwnd, st);     // avvalgi o'tish tugamagan bo'lsa, darhol yakunlanadi
    st->from = st->step;
    st->dir  = delta;
    st->step = ns;
    st->focus = ns == STEP_MODE ? IT_CARD0 + (st->mode == MODE_AUTO ? 1 : 0)
              : (st->focus >= IT_CARD0 || (st->focus == IT_BACK && ns == 0)) ? IT_NEXT
              : st->focus;
    Ui_AnimTo(st->pos, (float)ns, 220);
    if (!(Ui_AnimationsEnabled() && StartTransition(hwnd, st))) OnArrive(hwnd, st);
    Kick(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

static void Activate(HWND hwnd, TutState* st, int it)
{
    switch (it) {
    case IT_BACK:  GoStep(hwnd, st, -1); break;
    case IT_NEXT:  GoStep(hwnd, st, +1); break;
    case IT_CARD0: SelectMode(hwnd, st, 0); break;
    case IT_CARD1: SelectMode(hwnd, st, 1); break;
    case IT_TRY:   // "Tugatish" bilan bir xil (rejim saqlanadi), so'ng sinov sahifasi ochiladi
        s_tryNow = true;
        ApplyMode(st);
        DestroyWindow(hwnd);
        break;
    }
}

// Tab / Shift+Tab: tanlangan kartochka (rejim sahifasida) -> Orqaga -> Keyingi
static void MoveFocus(HWND hwnd, TutState* st, int dir)
{
    int stops[4], n = 0, cur = 0;
    if (st->step == STEP_MODE) stops[n++] = IT_CARD0 + (st->mode == MODE_AUTO ? 1 : 0);
    if (st->step > 0)          stops[n++] = IT_BACK;
    if (st->step == STEP_COUNT - 1) stops[n++] = IT_TRY;
    stops[n++] = IT_NEXT;
    for (int i = 0; i < n; ++i) if (stops[i] == st->focus) cur = i;
    st->focus = stops[(cur + dir + n) % n];
    InvalidateRect(hwnd, NULL, FALSE);
}

static void OnKey(HWND hwnd, TutState* st, WPARAM vk)
{
    st->kbd = true;
    switch (vk) {
    case VK_ESCAPE: DestroyWindow(hwnd); return;                // yopish (rejim saqlanmaydi)
    case VK_RETURN: GoStep(hwnd, st, +1); return;               // Enter = "Keyingi"
    case VK_TAB:    MoveFocus(hwnd, st, GetKeyState(VK_SHIFT) < 0 ? -1 : +1); return;
    case VK_SPACE:  Activate(hwnd, st, st->focus); return;
    case VK_LEFT: case VK_UP:
        if (st->step == STEP_MODE) SelectMode(hwnd, st, 0); else GoStep(hwnd, st, -1);
        return;
    case VK_RIGHT: case VK_DOWN:
        if (st->step == STEP_MODE) SelectMode(hwnd, st, 1);
        else if (st->step < STEP_COUNT - 1) GoStep(hwnd, st, +1);
        return;
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

static LRESULT CALLBACK TutProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TutState* st = (TutState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        Ui_Begin();     // GDI+ (topilmasa GDI fallback); WM_NCDESTROY da Ui_End
        st = new TutState();
        st->step  = 0;
        st->focus = IT_NEXT;
        st->hot   = st->down = IT_NONE;
        st->mode  = g_settings.mode;
        st->sel[st->mode == MODE_AUTO ? 1 : 0].from = st->sel[st->mode == MODE_AUTO ? 1 : 0].to =
            st->sel[st->mode == MODE_AUTO ? 1 : 0].value = 1.f;
        st->fTitle   = Ui_Font(s_dpi, 26, 700);
        st->fTitleSm = Ui_Font(s_dpi, 21, 700);
        st->fBody    = Ui_Font(s_dpi, 15, 400);
        st->fBodySm  = Ui_Font(s_dpi, 13, 400);
        st->fKeySm   = Ui_Font(s_dpi, 15, 600);
        st->fKey     = Ui_Font(s_dpi, 20, 600);
        st->fRes0    = Ui_Font(s_dpi, 24, 700);
        st->fResSm   = Ui_Font(s_dpi, 32, 700);
        st->fRes     = Ui_Font(s_dpi, 44, 700);
        st->fSymSm   = Ui_Font(s_dpi, 16, 400);
        st->fSym     = Ui_Font(s_dpi, 22, 400);
        st->fBtn     = Ui_Font(s_dpi, 15, 600);
        st->fStep    = Ui_Font(s_dpi, 12, 400);
        st->fCard    = Ui_Font(s_dpi, 17, 600);
        st->fCardSm  = Ui_Font(s_dpi, 15, 600);
        st->fType    = Ui_Font(s_dpi, 34, 600);
        st->fIcon    = Ui_IconFont(s_dpi, 18);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);
        Ui_ApplyWindowStyle(hwnd);
        return 0;
    }

    case WM_KEYDOWN:
        if (st) OnKey(hwnd, st, wp);
        return 0;

    case WM_MOUSEMOVE:
        if (st) {
            if (!st->tracking) {
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                st->tracking = true;
            }
            SetHot(hwnd, st, Hit(st, (short)LOWORD(lp), (short)HIWORD(lp)));
        }
        return 0;

    case WM_MOUSELEAVE:
        if (st) { st->tracking = false; SetHot(hwnd, st, IT_NONE); }
        return 0;

    case WM_SETCURSOR:
        if (st && LOWORD(lp) == HTCLIENT && st->hot >= 0) {
            SetCursor(LoadCursorW(NULL, IDC_HAND));
            return TRUE;
        }
        break;

    case WM_LBUTTONDOWN:
        if (st) {
            int it = Hit(st, (short)LOWORD(lp), (short)HIWORD(lp));
            st->kbd = false;
            if (it >= 0) {
                st->down = it;
                SetCapture(hwnd);
                Ui_AnimTo(st->prs[it], 1.f, 90);
                Kick(hwnd);
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_LBUTTONUP:
        if (st && st->down >= 0) {
            int d = st->down;
            st->down = IT_NONE;
            ReleaseCapture();
            Ui_AnimTo(st->prs[d], 0.f, 140);
            Kick(hwnd);
            int it = Hit(st, (short)LOWORD(lp), (short)HIWORD(lp));
            SetHot(hwnd, st, it);
            if (it == d) Activate(hwnd, st, d);     // oynani yopishi mumkin — st'ga tegmaymiz
        }
        return 0;

    case WM_TIMER:
        if (st && wp == UI_ANIM_TIMER) {
            bool run = TickAnims(hwnd, st);
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
        return 1;   // WM_PAINT o'zi to'ldiradi (miltillashning oldini olamiz)

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
            Layer_Free(st->lA); Layer_Free(st->lB); Layer_Free(st->lO);
            HFONT* fs[] = { &st->fTitle, &st->fTitleSm, &st->fBody, &st->fBodySm, &st->fKeySm,
                            &st->fKey, &st->fRes0, &st->fResSm, &st->fRes, &st->fSymSm,
                            &st->fSym, &st->fBtn, &st->fStep, &st->fCard, &st->fCardSm, &st->fType, &st->fIcon };
            for (HFONT* f : fs) if (*f) DeleteObject(*f);
            delete st;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            Ui_End();
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
    s_dpi = Ui_Dpi(NULL);
    if (s_dpi <= 0) s_dpi = 96;
    s_cw = S(CW96);
    s_ch = S(CH96);

    DWORD style   = WS_CAPTION | WS_SYSMENU;
    DWORD exstyle = 0;
    RECT rc = { 0, 0, s_cw, s_ch };
    AdjustWindowRectEx(&rc, style, FALSE, exstyle);
    int ww = rc.right - rc.left, wh = rc.bottom - rc.top;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;

    s_tryNow = false;
    HWND hwnd = CreateWindowExW(exstyle, L"HarfbekTutorialWnd", T(S_TUT_TITLE),
                                style, sx, sy, ww, wh, parent, NULL, g_hInst, NULL);
    if (!hwnd) return;

    BOOL parentWasEnabled = IsWindowEnabled(parent);
    if (parent && parentWasEnabled) EnableWindow(parent, FALSE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    OnArrive(hwnd, (TutState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA));   // 1-sahifa animatsiyasi

    MSG msg;
    while (IsWindow(hwnd) && GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (parent && parentWasEnabled) EnableWindow(parent, TRUE);

    // "Hoziroq sinab köring": wizard yopildi — endi Sozlamalarning sinov sahifasi
    if (s_tryNow) {
        s_tryNow = false;
        ShowSettingsPage(parent, SP_PLAYGROUND);
    }
    if (parent && parentWasEnabled) SetForegroundWindow(parent);
}
