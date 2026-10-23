// Sozlamalar oynasi: o'zimiz chizgan (ui_gfx) chap sidebar + kartochkali sahifalar
// (Umumiy | Rejim | Tezkor tugmalar | Istisnolar | Haqida). Native faqat matn kiritish
// uchun EDIT; qolgan hamma element shu yerda chiziladi, hit-test/hover/fokus mantig'i ham.
// Trigger tugma nomlari uchun yordamchi funksiya (TriggerKeyCap) ham shu yerda.
#include "app.h"
#include <shellapi.h>
#include <commctrl.h>
#include <wchar.h>

// Sozlamalar oynasi tezlikka muhtoj emas — exe hajmi byudjeti uchun shu fayl hajm bo'yicha optimallashtiriladi
#pragma optimize("s", on)

// ---------------------------------------------------------------------
//  Trigger tugma nomlari
// ---------------------------------------------------------------------

// Qisqa keycap nomi (qo'llanmada tugma rasmida, tray tooltip'da chiqadi).
// Har doim bo'sh bo'lmagan satr qaytaradi.
void TriggerKeyCap(UINT vk, wchar_t* out, int n)
{
    const wchar_t* s = NULL;
    switch (vk) {
    case VK_RMENU:    s = L"AltGr"; break;
    case VK_LMENU:
    case VK_MENU:     s = L"Alt";   break;
    case VK_RCONTROL: s = L"Ctrl";  break;
    case VK_RSHIFT:   s = L"R Shift"; break;
    case VK_CAPITAL:  s = L"Caps";  break;
    case VK_APPS:     s = L"Menu";  break;
    case VK_SCROLL:   s = L"ScrLk"; break;
    case VK_PAUSE:    s = L"Pause"; break;
    case VK_F12:      s = L"F12";   break;
    }
    if (s) {
        lstrcpynW(out, s, n);
    } else {
        // xavfsiz fallback: ruxsat etilmagan VK ham kod ko'rinishida chiqadi
        wchar_t b[16];   // "0x" + 8 hex raqam + NUL: istalgan UINT sig'adi
        wsprintfW(b, L"0x%02X", vk);
        lstrcpynW(out, b, n);
    }
}

// Trigger variantlari — FAQAT sinalgan tugmalar.
// Alt/Ctrl/Shift oilasi oddiy modifikator sifatida ishlaydi; qolganlari
// (Caps, Menyu, Scroll Lock, Pause, F12) hook tomonidan to'liq yutiladi,
// shuning uchun o'zining standart vazifasini bajarmaydi — bu ataylab shunday.
struct TrigOpt { StrId name; UINT vk; };
static const TrigOpt kTrigOpts[] = {
    { S_TRIG_RALT,    VK_RMENU    },
    { S_TRIG_LALT,    VK_LMENU    },
    { S_TRIG_BOTHALT, VK_MENU     },
    { S_TRIG_RCTRL,   VK_RCONTROL },
    { S_TRIG_RSHIFT,  VK_RSHIFT   },
    { S_TRIG_CAPS,    VK_CAPITAL  },
    { S_TRIG_APPS,    VK_APPS     },
    { S_TRIG_SCROLL,  VK_SCROLL   },
    { S_TRIG_PAUSE,   VK_PAUSE    },
    { S_TRIG_F12,     VK_F12      },
};
static const int kTrigOptCount = sizeof(kTrigOpts) / sizeof(kTrigOpts[0]);

// Berilgan VK sinalgan trigger ro'yxatida bormi (ini fayldan o'qishda
// tekshiriladi — eski/buzilgan qiymatlar standartga qaytadi).
bool Trigger_IsAllowed(UINT vk)
{
    for (int i = 0; i < kTrigOptCount; ++i)
        if (kTrigOpts[i].vk == vk) return true;
    return false;
}

// Apostrof variantlari (o' g' uchun chiqadigan belgi)
struct ApoOpt { const wchar_t* label; wchar_t ch; };
static const ApoOpt kApoOpts[] = {
    { L"ʻ  ( oʻzbek )", (wchar_t)0x02BB },   // rasmiy tutuq belgisi (standart)
    { L"'  ( ' )",   L'\''     },   // U+0027 oddiy apostrof
    { L"`  ( ` )",   L'`'      },   // U+0060 grave
    { L"’  ( ’ )", (wchar_t)0x2019 },        // egik apostrof
};
static const int kApoOptCount = sizeof(kApoOpts) / sizeof(kApoOpts[0]);

// ---------------------------------------------------------------------
//  Oyna holati va o'lchamlar
// ---------------------------------------------------------------------

#define WND_CLASS    L"HarfbekSettingsWnd"
// Hamma o'lchamlar 96 DPI pikselda; ishlatishda Ui_S bilan masshtablanadi
#define BASE_W       760     // klient maydoni
#define BASE_H       520
#define SIDE_W       200     // chap sidebar
#define BAR_H        68      // pastki panel
#define PAD          32      // kontent chetidan masofa
#define TOP_Y        84      // sahifa elementlari boshlanadigan y
#define ROW_H        56      // sozlama qatori
#define ROW_GAP      8
#define LROW_H       44      // istisnolar ro'yxati qatori
#define LROW_GAP     4
#define NAV_Y0       92
#define NAV_H        40
#define NAV_STEP     44

// Istisnolar ro'yxati uchun chegara (Settings_SetExcludes: maks 1023 belgi)
#define EXCLUDE_MAX  1024
#define MAX_ITEMS    72
#define MAX_POOL     16

// Element ID lari (NAV ID = sidebar tartibi; sahifa qiymati kNavPage orqali)
enum {
    ID_NAV0, ID_NAV1, ID_NAV2, ID_NAV3, ID_NAV4, ID_NAV5,   // sidebar tartibi (NavOf/kNavPage)
    ID_CANCEL, ID_SAVE,
    ID_LANG, ID_APO, ID_STARTUP, ID_NOTIFY,
    ID_CARD_TRIG, ID_CARD_AUTO, ID_TRIG,
    ID_RULE_O, ID_RULE_G, ID_RULE_SH, ID_RULE_CH,     // ketma-ket: AUTO_RULE_* bitlari
    ID_HK0, ID_HK1, ID_HK2, ID_HKX0, ID_HKX1, ID_HKX2,
    ID_EDIT, ID_ADD, ID_BROWSE, ID_LIST,
    ID_GITHUB, ID_LINKEDIN, ID_LICENSE,
    ID_PGEDIT, ID_PGCLEAR,                            // sinov maydoni
    ID_COUNT
};

// Animatsiyalar bitta massivda: [hover x ID_COUNT][toggle/karta x ID_COUNT][...]
enum {
    AN_HOV  = 0,
    AN_TOG  = ID_COUNT,
    AN_NAVY = 2 * ID_COUNT,     // sidebar tanlov fonining y si
    AN_SEG,                     // til segmentidagi tanlov (0..2)
    AN_PAGE,                    // sahifa almashuvi (0..1)
    AN_ERR,                     // hotkey rad etilganda qizil chaqnash (3 ta)
    AN_PGFOC = AN_ERR + 3,      // sinov maydoni fokus porlashi
    AN_BUMP,                    // hisoblagich "bump"
    AN_DONE,                    // "Barchasi bajarildi" banneri
    AN_CH,                      // sinovlar ✓ "pop" (4 ta)
    AN_COUNT = AN_CH + 4
};

// Element turlari
enum {
    K_ROW,      // sozlama qatori kartochkasi + chapda nom   (interaktiv emas)
    K_TEXT,     // oddiy matn                                 (interaktiv emas)
    K_ILLU,     // keycap illyustratsiyasi                    (interaktiv emas)
    K_ICON,     // katta ikonka                               (interaktiv emas)
    K_CHIPS,    // "almashtirish izi" chiplari                (interaktiv emas)
    K_CHALL,    // kichik sinov kartochkasi (sub = indeks)    (interaktiv emas)
    K_CHLBL,    // "Kichik sinovlar" yorlig'i / bajarildi banneri (interaktiv emas)
    K_PILL,     // ogohlantirish pill'i (matn)               (interaktiv emas)
    K_NAV, K_TOGGLE, K_DROP, K_SEG, K_CARD, K_BTN, K_HK, K_HKX, K_LIST, K_EDIT,
    K_PGEDIT    // sinov maydoni kartochkasi (ichida native EDIT)
};
#define K_FIRST_INTERACTIVE  K_NAV

// K_TEXT uslublari
enum { TS_TITLE, TS_HEAD, TS_BODY, TS_HINT, TS_SUB };

struct Item {
    int  id, kind;
    RECT rc;
    bool enabled, page;     // page: kontent sahifasiga tegishli (nav va pastki panel emas)
    const wchar_t* text;    // yorliq/qiymat (T() yoki literal — doimiy ko'rsatkich)
    int  sub;               // kind ga qarab: matn uslubi, tugma turi, illyustratsiya turi
};

// Sinov maydonidagi "almashtirish izi" chipi
struct Chip {
    bool used, dying;
    int  slot;
    wchar_t text[28];
    UiAnim in, pos, sp;     // kirish (fade+sirpanish), joy, porlash
};
#define CHIP_MAX 6

struct Win {
    HWND  hwnd, edit, pgEdit;
    int   dpi, cw, ch;
    int   page;             // joriy sahifa (SP_*)
    int   focus;            // fokusdagi element ID si (-1: yo'q)
    int   hot, pressed;     // sichqoncha ostidagi / bosilgan element ID si
    bool  kbd;              // fokus halqasi faqat klaviatura bilan ishlaganda ko'rinadi
    bool  tracking;         // TrackMouseEvent faol
    int   rec;              // yozib olinayotgan hotkey indeksi (-1: yo'q)
    bool  done, hkApplied;
    HFONT fTitle, fHead, fBody, fSmall, fCap, fBrand, fIcon, fIconSm, fIconLg, fPg, fResult, fRule;
    HICON icon, iconBig;
    HBRUSH editBr;
    COLORREF editBrCol;
    HMODULE msimg;
    BOOL (WINAPI *alphaBlend)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
    HBITMAP snap;           // sahifa almashuvi paytida yangi sahifaning surati
    bool  timerOn;

    // tahrirlanayotgan qiymatlar (Saqlashgacha g_settings'ga tegmaydi; til — jonli)
    Lang  origLang;
    wchar_t apo;
    bool  startup, notify;
    InputMode mode;
    UINT  trig, rules;
    WORD  hk[3];
    wchar_t ex[EXCLUDE_MAX];
    int   exSel, exScroll, exHot;
    bool  exHotX;

    UiAnim an[AN_COUNT];
    Item  it[MAX_ITEMS];
    int   nit;
    wchar_t pool[MAX_POOL][96];
    int   npool;
    wchar_t example[256];
    bool  addingPage;

    // sinov maydoni (matn hech qayerga saqlanmaydi)
    WNDPROC pgOrig;
    bool  wantPgFocus, pgEmpty, chDone[4], allDone;
    int   pgCount[8], pgTotal;
    Chip  chips[CHIP_MAX];
};

static HWND s_hSettings = NULL;     // qayta kirishdan himoya: oyna bittagina

static inline int S(const Win* w, int px) { return Ui_S(px, w->dpi); }

// Sidebar tartibi (SP_PLAYGROUND enum oxirida, lekin "Rejim" dan keyin turadi)
#define NAV_N 6
static const int kNavPage[NAV_N] = { SP_GENERAL, SP_MODE, SP_PLAYGROUND, SP_HOTKEYS, SP_EXCLUDE, SP_ABOUT };
static int NavOf(int page)
{
    for (int i = 0; i < NAV_N; ++i)
        if (kNavPage[i] == page) return i;
    return 0;
}

// ---------------------------------------------------------------------
//  Yordamchilar
// ---------------------------------------------------------------------

static void SetAnim(UiAnim& a, float v)
{
    a.from = a.to = a.value = v;
    a.start = 0; a.dur = 0; a.running = false;
}

// Animatsiya taymeri faqat animatsiya (yoki hotkey yozib olish) paytida ishlaydi
static void Kick(Win* w)
{
    if (!w->timerOn) {
        SetTimer(w->hwnd, UI_ANIM_TIMER, 15, NULL);
        w->timerOn = true;
    }
}

static void AnimTo(Win* w, int idx, float to, DWORD ms)
{
    Ui_AnimTo(w->an[idx], to, ms);
    if (w->an[idx].running) Kick(w);
}

static void Redraw(Win* w) { InvalidateRect(w->hwnd, NULL, FALSE); }

// Yorliq oxiridagi ':' va bo'shliqlarni olib tashlab, doimiy buferga ko'chiradi
static const wchar_t* Label(Win* w, StrId id)
{
    wchar_t* p = w->pool[w->npool < MAX_POOL ? w->npool++ : MAX_POOL - 1];
    lstrcpynW(p, T(id), 96);
    int n = lstrlenW(p);
    while (n > 0 && (p[n - 1] == L':' || p[n - 1] == L' ')) p[--n] = 0;
    return p;
}

static int TextW(HDC dc, HFONT f, const wchar_t* s)
{
    HGDIOBJ old = SelectObject(dc, f);
    SIZE sz = {};
    GetTextExtentPoint32W(dc, s, lstrlenW(s), &sz);
    SelectObject(dc, old);
    return sz.cx;
}

static int ItemIndex(const Win* w, int id)
{
    for (int i = 0; i < w->nit; ++i)
        if (w->it[i].id == id) return i;
    return -1;
}

static bool IsInteractive(const Item& i) { return i.kind >= K_FIRST_INTERACTIVE; }

static bool CardSelected(const Win* w, int id)
{
    return (id == ID_CARD_AUTO) == (w->mode == MODE_AUTO);
}

// Tab bilan o'tiladigan elementmi
static bool IsFocusable(const Win* w, const Item& i)
{
    if (!i.enabled) return false;
    switch (i.kind) {
    case K_NAV:  return i.id - ID_NAV0 == NavOf(w->page);    // sidebar — bitta to'xtash joyi
    case K_PGEDIT: return true;
    case K_CARD: return CardSelected(w, i.id);      // radio guruhi kabi
    case K_TOGGLE: case K_DROP: case K_SEG: case K_BTN: case K_HK:
    case K_LIST: case K_EDIT:
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------
//  Istisnolar ro'yxati (";" bilan ajratilgan qator ustida)
// ---------------------------------------------------------------------

static int ExCount(const wchar_t* list)
{
    int n = 0;
    const wchar_t* s = list;
    for (;;) {
        const wchar_t* e = s;
        while (*e && *e != L';') ++e;
        if (e > s) ++n;
        if (!*e) break;
        s = e + 1;
    }
    return n;
}

static bool ExGet(const wchar_t* list, int idx, wchar_t* out, int n)
{
    int k = 0;
    const wchar_t* s = list;
    for (;;) {
        const wchar_t* e = s;
        while (*e && *e != L';') ++e;
        if (e > s) {
            if (k == idx) {
                int len = (int)(e - s);
                if (len >= n) len = n - 1;
                CopyMemory(out, s, len * sizeof(wchar_t));
                out[len] = 0;
                return true;
            }
            ++k;
        }
        if (!*e) return false;
        s = e + 1;
    }
}

static void ExRemove(wchar_t* list, int idx)
{
    wchar_t out[EXCLUDE_MAX];
    int len = 0, k = 0;
    const wchar_t* s = list;
    for (;;) {
        const wchar_t* e = s;
        while (*e && *e != L';') ++e;
        if (e > s) {
            if (k != idx) {
                if (len) out[len++] = L';';
                CopyMemory(out + len, s, (e - s) * sizeof(wchar_t));
                len += (int)(e - s);
            }
            ++k;
        }
        if (!*e) break;
        s = e + 1;
    }
    out[len] = 0;
    lstrcpyW(list, out);
}

// Boshlang'ich ro'yxatni bo'sh elementlarsiz ko'rinishga keltiradi
static void ExLoad(Win* w)
{
    wchar_t buf[EXCLUDE_MAX];
    Settings_GetExcludes(buf, EXCLUDE_MAX);
    int len = 0;
    const wchar_t* s = buf;
    for (;;) {
        const wchar_t* e = s;
        while (*e && *e != L';') ++e;
        if (e > s) {
            if (len) w->ex[len++] = L';';
            CopyMemory(w->ex + len, s, (e - s) * sizeof(wchar_t));
            len += (int)(e - s);
        }
        if (!*e) break;
        s = e + 1;
    }
    w->ex[len] = 0;
}

static int ListContentH(const Win* w)
{
    int n = ExCount(w->ex);
    return n ? n * S(w, LROW_H) + (n - 1) * S(w, LROW_GAP) : 0;
}

static void ClampScroll(Win* w)
{
    int i = ItemIndex(w, ID_LIST);
    int view = i >= 0 ? w->it[i].rc.bottom - w->it[i].rc.top : 0;
    int maxS = ListContentH(w) - view;
    if (maxS < 0) maxS = 0;
    if (w->exScroll > maxS) w->exScroll = maxS;
    if (w->exScroll < 0) w->exScroll = 0;
}

// Tanlangan qator ko'rinadigan qilib suriladi
static void ScrollToSel(Win* w)
{
    int i = ItemIndex(w, ID_LIST);
    if (i < 0 || w->exSel < 0) return;
    int view = w->it[i].rc.bottom - w->it[i].rc.top;
    int step = S(w, LROW_H) + S(w, LROW_GAP);
    int top = w->exSel * step, bot = top + S(w, LROW_H);
    if (top < w->exScroll) w->exScroll = top;
    else if (bot > w->exScroll + view) w->exScroll = bot - view;
    ClampScroll(w);
}

// Nomni ro'yxatga qo'shadi: faqat fayl nomi, ".exe" bilan, kichik harfda, takrorsiz.
// Qo'shilgan (yoki allaqachon bor) bo'lsa true.
static bool ExAdd(Win* w, const wchar_t* raw)
{
    // yo'l berilgan bo'lsa fayl nomi qismi olinadi
    const wchar_t* base = raw;
    for (const wchar_t* c = raw; *c; ++c)
        if (*c == L'\\' || *c == L'/') base = c + 1;

    wchar_t name[MAX_PATH];
    lstrcpynW(name, base, MAX_PATH);
    int len = lstrlenW(name);
    while (len && name[len - 1] == L' ') name[--len] = 0;
    int skip = 0;
    while (name[skip] == L' ') ++skip;
    if (skip) { MoveMemory(name, name + skip, (len - skip + 1) * sizeof(wchar_t)); len -= skip; }
    if (!len) return false;

    // ";" ro'yxat ajratgichi, qolganlari fayl nomida bo'lishi mumkin emas
    for (int i = 0; i < len; ++i)
        if (wcschr(L";:*?\"<>|", name[i])) { MessageBeep(MB_ICONWARNING); return false; }

    if (len < 4 || lstrcmpiW(name + len - 4, L".exe") != 0) {
        if (len + 4 >= MAX_PATH) return false;
        lstrcpyW(name + len, L".exe");
        len += 4;
    }
    if (len == 4) return false;     // faqat ".exe"
    CharLowerBuffW(name, len);

    int cnt = ExCount(w->ex);
    for (int i = 0; i < cnt; ++i) {
        wchar_t cur[MAX_PATH];
        if (ExGet(w->ex, i, cur, MAX_PATH) && lstrcmpW(cur, name) == 0) {
            w->exSel = i;               // takror — qo'shilmaydi, faqat ko'rsatiladi
            ScrollToSel(w);
            return true;
        }
    }

    int cl = lstrlenW(w->ex);
    if (cl + 1 + len >= EXCLUDE_MAX) {  // saqlash chegarasi
        MessageBeep(MB_ICONWARNING);
        return false;
    }
    if (cl) w->ex[cl++] = L';';
    lstrcpyW(w->ex + cl, name);
    w->exSel = cnt;
    return true;
}

// ---------------------------------------------------------------------
//  Tezkor tugmalar qoidalari
// ---------------------------------------------------------------------

// Modifikatorsiz, yolg'iz Shift va Ctrl+Alt (= AltGr) kombinatsiyalari taqiqlanadi —
// oddiy yozishga va AltGr bilan harf kiritishga xalaqit beradi
static bool HkAllowed(WORD hk)
{
    BYTE m = HIBYTE(hk) & (HOTKEYF_SHIFT | HOTKEYF_CONTROL | HOTKEYF_ALT);
    if (!m) return false;
    if (m == HOTKEYF_SHIFT) return false;
    if (m == (HOTKEYF_CONTROL | HOTKEYF_ALT)) return false;
    return true;
}

// "Ctrl+Shift+F9" -> ["Ctrl","Shift","F9"] (keycap'lar uchun)
static int HkTokens(WORD hk, wchar_t tok[6][24])
{
    wchar_t s[96];
    Hotkey_Format(hk, s, 96);
    int n = 0;
    const wchar_t* p = s;
    while (*p && n < 6) {
        const wchar_t* e = p;
        while (*e && *e != L'+') ++e;
        int len = (int)(e - p);
        if (len > 0) {
            if (len > 23) len = 23;
            CopyMemory(tok[n], p, len * sizeof(wchar_t));
            tok[n][len] = 0;
            ++n;
        }
        if (!*e) break;
        p = e + 1;
    }
    return n;
}

// Modifikator tugmalarmi (yozib olishda e'tiborga olinmaydi)
static bool IsModVk(UINT vk)
{
    return vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || vk == VK_LSHIFT ||
           vk == VK_RSHIFT || vk == VK_LCONTROL || vk == VK_RCONTROL ||
           vk == VK_LMENU || vk == VK_RMENU || vk == VK_LWIN || vk == VK_RWIN;
}

// ---------------------------------------------------------------------
//  Joylashuv: elementlar ro'yxatini quradi
// ---------------------------------------------------------------------

static Item* Add(Win* w, int id, int kind, int l, int t, int r, int b,
                 const wchar_t* text = NULL, bool en = true, int sub = 0)
{
    Item* i = &w->it[w->nit < MAX_ITEMS ? w->nit++ : MAX_ITEMS - 1];
    i->id = id; i->kind = kind;
    i->rc.left = l; i->rc.top = t; i->rc.right = r; i->rc.bottom = b;
    i->enabled = en; i->page = w->addingPage; i->text = text; i->sub = sub;
    return i;
}

static int X0(const Win* w) { return S(w, SIDE_W + PAD); }
static int X1(const Win* w) { return w->cw - S(w, PAD); }

// Sozlama qatori: kartochka + chapda nom. O'ngdagi boshqaruv uchun joy (cw x chh) qaytariladi.
static RECT AddRow(Win* w, int y, const wchar_t* name, int cw, int chh)
{
    RECT row = { X0(w), y, X1(w), y + S(w, ROW_H) };
    Item* it = Add(w, -1, K_ROW, row.left, row.top, row.right, row.bottom, name);
    // nom matni boshqaruv joyigacha cho'ziladi
    it->sub = cw + S(w, 28);
    int ry = y + (S(w, ROW_H) - chh) / 2;
    RECT c = { row.right - S(w, 16) - cw, ry, row.right - S(w, 16), ry + chh };
    return c;
}

static void PageTitle(Win* w, const wchar_t* text)
{
    Add(w, -1, K_TEXT, X0(w), S(w, 22), X1(w), S(w, 66), text, true, TS_TITLE);
}

static void LayoutGeneral(Win* w)
{
    int y = S(w, TOP_Y);
    int step = S(w, ROW_H + ROW_GAP);
    RECT c = AddRow(w, y, Label(w, S_SET_LANGUAGE), S(w, 300), S(w, 34));
    Add(w, ID_LANG, K_SEG, c.left, c.top, c.right, c.bottom);
    y += step;

    c = AddRow(w, y, Label(w, S_SET_APOSTROPHE), S(w, 190), S(w, 34));
    const wchar_t* apoText = kApoOpts[0].label;
    for (int i = 0; i < kApoOptCount; ++i)
        if (kApoOpts[i].ch == w->apo) apoText = kApoOpts[i].label;
    Add(w, ID_APO, K_DROP, c.left, c.top, c.right, c.bottom, apoText);
    y += step;

    c = AddRow(w, y, T(S_SET_STARTUP), S(w, 44), S(w, 22));
    Add(w, ID_STARTUP, K_TOGGLE, c.left, c.top, c.right, c.bottom);
    y += step;

    c = AddRow(w, y, T(S_SET_NOTIFY), S(w, 44), S(w, 22));
    Add(w, ID_NOTIFY, K_TOGGLE, c.left, c.top, c.right, c.bottom);
    y += step + S(w, 4);

    Add(w, -1, K_TEXT, X0(w), y, X1(w), y + S(w, 40), T(S_SET_APPEARANCE_HINT), true, TS_HINT);
}

static void LayoutMode(Win* w)
{
    int gap = S(w, 16);
    int cardW = (X1(w) - X0(w) - gap) / 2;
    int top = S(w, TOP_Y), cardH = S(w, 252);
    bool trigOn = (w->mode == MODE_TRIGGER);

    for (int k = 0; k < 2; ++k) {
        int l = X0(w) + k * (cardW + gap), r = l + cardW;
        int p = S(w, 18);
        Add(w, k ? ID_CARD_AUTO : ID_CARD_TRIG, K_CARD, l, top, r, top + cardH);
        // qisqa sarlavha (bir qator) + kichik izoh qatori; o'ng yuqoridagi ✓ belgisiga tegmasligi uchun sarlavha torroq
        Add(w, -1, K_TEXT, l + p, top + S(w, 16), r - S(w, 44), top + S(w, 40),
            T(k ? S_OSD_MODE_AUTO : S_OSD_MODE_TRIGGER), true, TS_HEAD);
        Add(w, -1, K_TEXT, l + p, top + S(w, 42), r - p, top + S(w, 62),
            k ? L"o' g' sh ch → ö ğ ş ç" : T(S_MODE_TRIGGER), true, TS_SUB);
        Add(w, -1, K_ILLU, l + p, top + S(w, 70), r - p, top + S(w, 114), NULL, true, k);
        bool on = (k == 1) == !trigOn;      // shu kartochka tanlanganmi
        if (k == 0) {
            Add(w, -1, K_TEXT, l + p, top + S(w, 130), r - p, top + S(w, 150),
                Label(w, S_SET_TRIGGER), on, TS_HINT);
            const wchar_t* name = L"";
            for (int i = 0; i < kTrigOptCount; ++i)
                if (kTrigOpts[i].vk == w->trig) name = T(kTrigOpts[i].name);
            Add(w, ID_TRIG, K_DROP, l + p, top + S(w, 156), r - p, top + S(w, 156) + S(w, 36),
                name, on);
        } else {
            Add(w, -1, K_TEXT, l + p, top + S(w, 130), r - p, top + S(w, 150),
                Label(w, S_SET_RULES), on, TS_HINT);
            static const wchar_t* const kRule[4] = { L"o' → ö", L"g' → ğ", L"sh → ş", L"ch → ç" };
            int cell = (cardW - 2 * p) / 2;
            for (int i = 0; i < 4; ++i) {
                int cx = l + p + (i % 2) * cell, cy = top + S(w, 158) + (i / 2) * S(w, 38);
                Add(w, ID_RULE_O + i, K_TOGGLE, cx, cy, cx + cell, cy + S(w, 30), kRule[i], on);
            }
        }
    }

    // pastdagi misol matni
    if (w->mode == MODE_AUTO) {
        lstrcpynW(w->example, T(S_SET_EXAMPLE_AUTO), 256);
    } else {
        wchar_t cap[24];
        TriggerKeyCap(w->trig, cap, 24);
        wsprintfW(w->example, T(S_SET_EXAMPLE_TRIGGER), cap);
    }
    Add(w, -1, K_TEXT, X0(w), top + cardH + S(w, 18), X1(w), top + cardH + S(w, 48),
        w->example, true, TS_BODY);

    // tanlov hali saqlanmagan: sinov maydoni eski rejimda ishlashini eslatadi
    if (w->mode != g_settings.mode)
        Add(w, -1, K_PILL, X0(w), top + cardH + S(w, 58), X1(w), top + cardH + S(w, 84), T(S_PG_UNSAVED));
}

static void LayoutHotkeys(Win* w)
{
    static const StrId names[3] = { S_HK_MODE, S_HK_ENABLE, S_HK_CONVERT };
    int y = S(w, TOP_Y);
    for (int i = 0; i < 3; ++i) {
        RECT c = AddRow(w, y, Label(w, names[i]), S(w, 240), S(w, 36));
        Add(w, ID_HK0 + i, K_HK, c.left, c.top, c.right, c.bottom);
        if (w->hk[i] && w->rec != i) {
            int b = S(w, 28);
            Add(w, ID_HKX0 + i, K_HKX, c.right - b - S(w, 4), c.top + (c.bottom - c.top - b) / 2,
                c.right - S(w, 4), c.top + (c.bottom - c.top - b) / 2 + b);
        }
        y += S(w, ROW_H + ROW_GAP);
    }
    Add(w, -1, K_TEXT, X0(w), y + S(w, 4), X1(w), y + S(w, 56), T(S_HK_HINT), true, TS_HINT);
}

static void LayoutExclude(Win* w)
{
    int y = S(w, TOP_Y);
    Add(w, -1, K_TEXT, X0(w), y, X1(w), y + S(w, 38), T(S_EX_HINT), true, TS_HINT);
    y += S(w, 44);

    int h = S(w, 40), gap = S(w, 8);
    int addW = S(w, 100), brW = S(w, 118);
    int addL = X1(w) - brW - gap - addW;
    RECT fr = { X0(w), y, addL - gap, y + h };
    Add(w, ID_EDIT, K_EDIT, fr.left, fr.top, fr.right, fr.bottom);
    Add(w, ID_ADD, K_BTN, addL, y, addL + addW, y + h, T(S_EX_ADD), true, UI_BTN_PRIMARY);
    Add(w, ID_BROWSE, K_BTN, X1(w) - brW, y, X1(w), y + h, T(S_EX_BROWSE), true, UI_BTN_SECONDARY);

    // native EDIT ramka ichida
    int eh = S(w, 20);
    MoveWindow(w->edit, fr.left + S(w, 14), fr.top + (h - eh) / 2,
               fr.right - fr.left - S(w, 28), eh, TRUE);

    int lt = y + h + S(w, 12);
    Add(w, ID_LIST, K_LIST, X0(w), lt, X1(w), w->ch - S(w, BAR_H) - S(w, 14));
}

// Matn berilgan kenglikda necha piksel balandlikni egallaydi
static int TextH(Win* w, HFONT f, const wchar_t* s, int width)
{
    HDC dc = GetDC(w->hwnd);
    HGDIOBJ old = SelectObject(dc, f);
    RECT r = { 0, 0, width, 0 };
    DrawTextW(dc, s, -1, &r, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(dc, old);
    ReleaseDC(w->hwnd, dc);
    return r.bottom;
}

static void LayoutAbout(Win* w)
{
    int top = S(w, TOP_Y), x0 = X0(w);
    int isz = S(w, 72);
    Add(w, -1, K_ICON, x0, top, x0 + isz, top + isz);
    Add(w, -1, K_TEXT, x0 + isz + S(w, 20), top + S(w, 2), X1(w), top + S(w, 44), L"Harfbek", true, TS_TITLE);
    Add(w, -1, K_TEXT, x0 + isz + S(w, 20), top + S(w, 46), X1(w), top + S(w, 68), L"v1.0.0", true, TS_HINT);
    int dy = top + S(w, 92);
    int dh = TextH(w, w->fBody, T(S_ABOUT_DESC), X1(w) - x0);
    Add(w, -1, K_TEXT, x0, dy, X1(w), dy + dh, T(S_ABOUT_DESC), true, TS_BODY);
    dy += dh + S(w, 14);
    Add(w, -1, K_TEXT, x0, dy, X1(w), dy + S(w, 24), T(S_ABOUT_DEV), true, TS_HINT);

    int by = dy + S(w, 44), bh = S(w, 36), bw = S(w, 116), gap = S(w, 10);
    Add(w, ID_GITHUB,   K_BTN, x0,                 by, x0 + bw,                 by + bh, T(S_ABOUT_GITHUB), true, UI_BTN_SECONDARY);
    Add(w, ID_LINKEDIN, K_BTN, x0 + bw + gap,      by, x0 + 2 * bw + gap,       by + bh, T(S_ABOUT_OPEN),   true, UI_BTN_SECONDARY);
    Add(w, ID_LICENSE,  K_BTN, x0 + 2 * (bw + gap), by, x0 + 3 * bw + 2 * gap,   by + bh, T(S_LIC_TITLE),    true, UI_BTN_SECONDARY);
}

// ---- Sinov maydoni: maxsus harflar, chiplar va kichik sinovlar ----
static const wchar_t kSpecial[8] = { L'ö', L'ğ', L'ş', L'ç', L'Ö', L'Ğ', L'Ş', L'Ç' };
static const wchar_t kBase[8]    = { L'o', L'g', L's', L'c', L'O', L'G', L'S', L'C' };
static const wchar_t* const kDigraph[8] = {
    L"o' → ö", L"g' → ğ", L"sh → ş", L"ch → ç", L"O' → Ö", L"G' → Ğ", L"Sh → Ş", L"Ch → Ç" };

// Kichik sinov: maqsad so'z (kichik harfda) matnda paydo bo'lsa bajarildi
struct Challenge { const wchar_t* label; const wchar_t* target; };
static const Challenge kChall[4] = {
    { L"o'zbek → özbek",     L"özbek"   },
    { L"Toshkent → Toşkent", L"toşkent" },
    { L"cho'l → çöl",        L"çöl"     },
    { L"tog' → toğ",         L"toğ"     },
};

static COLORREF GreenOf(const UiTheme& th) { return th.dark ? RGB(0x4A, 0xDE, 0x80) : RGB(0x15, 0x80, 0x3D); }
static COLORREF WarnOf(const UiTheme& th)  { return th.dark ? RGB(0xFB, 0xBF, 0x24) : RGB(0xB4, 0x53, 0x09); }

static void LayoutPlayground(Win* w)
{
    int x0 = X0(w), x1 = X1(w), y = S(w, TOP_Y);
    int hh = TextH(w, w->fSmall, T(S_PG_HINT), x1 - x0);
    Add(w, -1, K_TEXT, x0, y, x1, y + hh, T(S_PG_HINT), true, TS_HINT);
    y += hh + S(w, 10);

    // yozuv kartochkasi (ichida native EDIT)
    int cardH = S(w, 148);
    Add(w, ID_PGEDIT, K_PGEDIT, x0, y, x1, y + cardH);
    MoveWindow(w->pgEdit, x0 + S(w, 18), y + S(w, 46), x1 - x0 - S(w, 36), cardH - S(w, 46) - S(w, 34), TRUE);
    y += cardH + S(w, 12);

    // "almashtirish izi" chiplari
    Add(w, -1, K_CHIPS, x0, y, x1, y + S(w, 30));
    y += S(w, 30) + S(w, 12);

    // kichik sinovlar sarlavhasi + Tozalash
    Add(w, -1, K_CHLBL, x0, y, x1 - S(w, 110), y + S(w, 28), T(S_PG_CHALLENGES));
    Add(w, ID_PGCLEAR, K_BTN, x1 - S(w, 100), y, x1, y + S(w, 28), T(S_PG_CLEAR), true, UI_BTN_GHOST);
    y += S(w, 28) + S(w, 8);

    int gap = S(w, 8), cwid = (x1 - x0 - gap) / 2, chh = S(w, 36);
    for (int i = 0; i < 4; ++i) {
        int cx = x0 + (i % 2) * (cwid + gap), cy = y + (i / 2) * (chh + gap);
        Add(w, -1, K_CHALL, cx, cy, cx + cwid, cy + chh, kChall[i].label, true, i);
    }
}

static const StrId kNavNames[NAV_N] = { S_TAB_GENERAL, S_TAB_MODE, S_NAV_PLAYGROUND, S_TAB_HOTKEYS, S_TAB_EXCLUDE, S_NAV_ABOUT };
static const wchar_t* const kNavGlyph[NAV_N] = { L"\xE713", L"\xE8AB", L"\xE768", L"\xE765", L"\xE8F8", L"\xE946" };

// ni — sidebar tartibidagi indeks
static int NavTop(const Win* w, int ni) { return S(w, NAV_Y0) + ni * S(w, NAV_STEP); }

static void Layout(Win* w)
{
    w->nit = 0;
    w->npool = 0;

    // sidebar
    w->addingPage = false;
    for (int i = 0; i < NAV_N; ++i)
        Add(w, ID_NAV0 + i, K_NAV, S(w, 12), NavTop(w, i), S(w, SIDE_W - 12), NavTop(w, i) + S(w, NAV_H),
            T(kNavNames[i]));

    // sahifa
    w->addingPage = true;
    PageTitle(w, T(kNavNames[NavOf(w->page)]));
    switch (w->page) {
    case SP_GENERAL: LayoutGeneral(w); break;
    case SP_MODE:    LayoutMode(w);    break;
    case SP_HOTKEYS: LayoutHotkeys(w); break;
    case SP_EXCLUDE: LayoutExclude(w); break;
    case SP_ABOUT:   LayoutAbout(w);   break;
    case SP_PLAYGROUND: LayoutPlayground(w); break;
    }
    w->addingPage = false;

    // pastki panel: [Bekor qiliş] [Saqlaş] — o'ngda
    int bh = S(w, 36), bt = w->ch - S(w, BAR_H) + (S(w, BAR_H) - bh) / 2;
    int sr = w->cw - S(w, PAD), sl = sr - S(w, 116);
    int cr = sl - S(w, 10),     cl = cr - S(w, 124);
    Add(w, ID_CANCEL, K_BTN, cl, bt, cr, bt + bh, T(S_SET_CANCEL), true, UI_BTN_SECONDARY);
    Add(w, ID_SAVE,   K_BTN, sl, bt, sr, bt + bh, T(S_SET_SAVE),   true, UI_BTN_PRIMARY);

    if (w->page == SP_EXCLUDE) ClampScroll(w);
}

// ---------------------------------------------------------------------
//  Chizish
// ---------------------------------------------------------------------

static COLORREF Dim(const UiTheme& th, float hover)
{
    return Ui_Mix(th.textDim, th.text, hover * 0.6f);
}

// Qator ichidagi boshqaruvlar (dropdown, segment, maydon) fon rangi
static COLORREF CtlFill(const UiTheme& th, float hover)
{
    return Ui_Mix(th.bg, th.surfaceHover, hover);
}

static void DrawRowCard(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    int r = S(w, 10);
    Ui_FillRound(dc, it.rc, r, th.surface);
    Ui_StrokeRound(dc, it.rc, r, th.border, 1);
    // uzun nom ikki qatorga o'raladi va qator ichida vertikal markazlanadi
    RECT t = { it.rc.left + S(w, 18), it.rc.top, it.rc.right - it.sub, it.rc.bottom };
    HGDIOBJ old = SelectObject(dc, w->fBody);
    RECT m = { 0, 0, t.right - t.left, 0 };
    DrawTextW(dc, it.text, -1, &m, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(dc, old);
    int off = (t.bottom - t.top - m.bottom) / 2;
    if (off > 0) t.top += off;
    Ui_Text(dc, it.text, t, w->fBody, th.text, DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
}

static void DrawText_(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    HFONT f = w->fBody;
    COLORREF c = it.enabled ? th.text : th.textDim;
    UINT fl = DT_WORDBREAK | DT_NOPREFIX;
    switch (it.sub) {
    case TS_TITLE: f = w->fTitle; fl = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS; break;
    case TS_HEAD:  f = w->fHead;  break;
    case TS_HINT:  f = w->fSmall; c = th.textDim; break;
    case TS_SUB:   f = w->fBody;  c = th.textDim; break;
    }
    Ui_Text(dc, it.text, it.rc, f, c, fl);
}

// Keycap qatori: x ni oldinga suradi
static void PutCap(Win* w, HDC dc, int* x, int y, int h, const wchar_t* s, bool accent)
{
    int cw = TextW(dc, w->fCap, s) + S(w, 22);
    if (cw < S(w, 36)) cw = S(w, 36);
    RECT r = { *x, y, *x + cw, y + h };
    Ui_DrawKeycap(dc, r, s, w->fCap, accent);
    *x += cw + S(w, 8);
}

static void PutSym(Win* w, HDC dc, int* x, int y, int h, const wchar_t* s)
{
    const UiTheme& th = Ui_Theme();
    int sw = TextW(dc, w->fBody, s) + S(w, 4);
    RECT r = { *x, y, *x + sw, y + h };
    Ui_Text(dc, s, r, w->fBody, th.textDim, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
    *x += sw + S(w, 8);
}

static void DrawIllu(Win* w, HDC dc, const Item& it)
{
    int h = S(w, 34), y = it.rc.top + (it.rc.bottom - it.rc.top - h) / 2, x = it.rc.left;
    if (it.sub == 0) {
        wchar_t cap[24];
        TriggerKeyCap(w->trig, cap, 24);
        PutCap(w, dc, &x, y, h, cap, false);
        PutSym(w, dc, &x, y, h, L"+");
        PutCap(w, dc, &x, y, h, L"o", false);
    } else {
        PutCap(w, dc, &x, y, h, L"o", false);
        PutCap(w, dc, &x, y, h, L"'", false);
    }
    PutSym(w, dc, &x, y, h, L"→");
    // natija harfi keycap ichida emas — katta accent shriftda (kichik o'lchamda ikki nuqta
    // keycap fonida qo'shilib "ō" ga o'xshab qolmasin)
    RECT rr = { x, it.rc.top, x + S(w, 36), it.rc.bottom };
    Ui_Text(dc, L"ö", rr, w->fResult, Ui_Theme().accent, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
}


static void DrawNav(Win* w, HDC dc)
{
    const UiTheme& th = Ui_Theme();
    int rad = S(w, 8);

    // tanlov foni — sirpanib yuradi
    RECT pill = { S(w, 12), (int)w->an[AN_NAVY].value, S(w, SIDE_W - 12), (int)w->an[AN_NAVY].value + S(w, NAV_H) };
    Ui_FillRound(dc, pill, rad, Ui_Mix(th.bg, th.accent, th.dark ? 0.24f : 0.12f));
    RECT ind = { pill.left + S(w, 4), pill.top + (S(w, NAV_H) - S(w, 16)) / 2,
                 pill.left + S(w, 7), pill.top + (S(w, NAV_H) - S(w, 16)) / 2 + S(w, 16) };
    Ui_FillRound(dc, ind, S(w, 2), th.accent);

    for (int i = 0; i < w->nit; ++i) {
        const Item& it = w->it[i];
        if (it.kind != K_NAV) continue;
        float hv = w->an[AN_HOV + it.id].value;
        bool sel = (it.id - ID_NAV0 == NavOf(w->page));
        if (!sel && hv > 0.01f)
            Ui_FillRound(dc, it.rc, rad, Ui_Mix(th.bg, th.text, 0.07f * hv));
        COLORREF tc = sel ? th.text : Dim(th, hv);
        RECT g = { it.rc.left + S(w, 14), it.rc.top, it.rc.left + S(w, 44), it.rc.bottom };
        Ui_Glyph(dc, kNavGlyph[it.id - ID_NAV0], g, w->fIcon, sel ? th.accent : tc);
        RECT t = { it.rc.left + S(w, 50), it.rc.top, it.rc.right - S(w, 8), it.rc.bottom };
        Ui_Text(dc, it.text, t, w->fBody, tc, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        if (w->kbd && w->focus == it.id) Ui_DrawFocusRing(dc, it.rc, rad);
    }
}

static void DrawBrand(Win* w, HDC dc)
{
    const UiTheme& th = Ui_Theme();
    int isz = S(w, 32), x = S(w, 20), y = S(w, 28);
    if (w->icon) DrawIconEx(dc, x, y, w->icon, isz, isz, 0, NULL, DI_NORMAL);
    RECT t = { x + isz + S(w, 12), y, S(w, SIDE_W - 8), y + isz };
    Ui_Text(dc, L"Harfbek", t, w->fBrand, th.text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

static void DrawToggleItem(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    bool focused = w->kbd && w->focus == it.id;
    int ty = it.rc.top + (it.rc.bottom - it.rc.top - S(w, 22)) / 2;
    RECT tg = it.rc;
    if (it.text) { tg.top = ty; tg.bottom = ty + S(w, 22); tg.right = tg.left + S(w, 40); }
    Ui_DrawToggle(dc, tg, w->an[AN_TOG + it.id].value, w->an[AN_HOV + it.id].value, it.enabled, focused);
    if (it.text) {
        RECT t = { tg.right + S(w, 8), it.rc.top, it.rc.right, it.rc.bottom };
        Ui_Text(dc, it.text, t, w->fRule, it.enabled ? th.text : th.textDim,
                DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
}

static void DrawDrop(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    float hv = it.enabled ? w->an[AN_HOV + it.id].value : 0.f;
    int r = S(w, 8);
    Ui_FillRound(dc, it.rc, r, CtlFill(th, hv));
    Ui_StrokeRound(dc, it.rc, r, th.border, 1);
    RECT t = { it.rc.left + S(w, 14), it.rc.top, it.rc.right - S(w, 36), it.rc.bottom };
    Ui_Text(dc, it.text, t, w->fBody, it.enabled ? th.text : th.textDim,
            DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    RECT g = { it.rc.right - S(w, 34), it.rc.top, it.rc.right - S(w, 8), it.rc.bottom };
    Ui_Glyph(dc, L"\xE70D", g, w->fIconSm, it.enabled ? th.textDim : Ui_Mix(th.textDim, th.bg, 0.5f));
    if (w->kbd && w->focus == it.id) Ui_DrawFocusRing(dc, it.rc, r);
}

static void DrawSeg(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    static const wchar_t* const kNames[3] = { L"Özbekça", L"Русский", L"English" };
    int r = S(w, 8), pad = S(w, 3);
    Ui_FillRound(dc, it.rc, r, th.bg);
    Ui_StrokeRound(dc, it.rc, r, th.border, 1);
    int sw = (it.rc.right - it.rc.left - 2 * pad) / 3;
    int px = it.rc.left + pad + (int)(w->an[AN_SEG].value * (float)sw + 0.5f);
    RECT pill = { px, it.rc.top + pad, px + sw, it.rc.bottom - pad };
    Ui_FillRound(dc, pill, r - 2, th.accent);
    for (int i = 0; i < 3; ++i) {
        RECT t = { it.rc.left + pad + i * sw, it.rc.top, it.rc.left + pad + (i + 1) * sw, it.rc.bottom };
        // pill ustidagi matn rangi pill holatiga qarab o'tadi
        float d = w->an[AN_SEG].value - (float)i;
        float on = 1.f - (d < 0.f ? -d : d);
        if (on < 0.f) on = 0.f;
        Ui_Text(dc, kNames[i], t, w->fBody, Ui_Mix(th.text, th.onAccent, on),
                DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
    }
    if (w->kbd && w->focus == it.id) Ui_DrawFocusRing(dc, it.rc, r);
}

static void DrawHkField(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    int k = it.id - ID_HK0;
    bool rec = (w->rec == k);
    float hv = w->an[AN_HOV + it.id].value;
    int r = S(w, 8);
    Ui_FillRound(dc, it.rc, r, CtlFill(th, hv));

    COLORREF bc = th.border;
    int bw = 1;
    if (rec) {
        // yengil pulsatsiya: accent ramka rangi silliq o'zgarib turadi
        float t = (float)(GetTickCount() % 1200) / 1200.f;
        float p = t < 0.5f ? t * 2.f : (1.f - t) * 2.f;
        bc = Ui_Mix(th.accent, th.border, 0.15f + 0.55f * p);
        bw = 2;
    }
    float er = w->an[AN_ERR + k].value;
    if (er > 0.01f) bc = Ui_Mix(bc, th.danger, er);
    Ui_StrokeRound(dc, it.rc, r, bc, bw);

    int pad = S(w, 10);
    if (rec) {
        RECT t = { it.rc.left + S(w, 14), it.rc.top, it.rc.right - pad, it.rc.bottom };
        Ui_Text(dc, T(S_HK_RECORD), t, w->fBody, th.accent, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    } else if (!w->hk[k]) {
        RECT t = { it.rc.left + S(w, 14), it.rc.top, it.rc.right - pad, it.rc.bottom };
        Ui_Text(dc, T(S_HK_NONE), t, w->fBody, th.textDim, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    } else {
        wchar_t tok[6][24];
        int n = HkTokens(w->hk[k], tok);
        int ch = S(w, 26), x = it.rc.left + pad, y = it.rc.top + (it.rc.bottom - it.rc.top - ch) / 2;
        for (int i = 0; i < n; ++i) PutCap(w, dc, &x, y, ch, tok[i], false);
    }
    if (w->kbd && w->focus == it.id && !rec) Ui_DrawFocusRing(dc, it.rc, r);
}

static void DrawHkx(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    float hv = w->an[AN_HOV + it.id].value;
    if (hv > 0.01f)
        Ui_FillCircle(dc, (it.rc.left + it.rc.right) / 2, (it.rc.top + it.rc.bottom) / 2,
                      (it.rc.right - it.rc.left) / 2, Ui_Mix(th.surface, th.danger, 0.22f * hv));
    Ui_Glyph(dc, L"\xE711", it.rc, w->fIconSm, Ui_Mix(th.textDim, th.danger, hv));
}

static void DrawList(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    int cnt = ExCount(w->ex);
    int saved = SaveDC(dc);
    IntersectClipRect(dc, it.rc.left, it.rc.top, it.rc.right, it.rc.bottom);

    if (!cnt) {
        // bo'sh holat: glif + matn markazda
        int mid = (it.rc.top + it.rc.bottom) / 2;
        RECT g = { it.rc.left, mid - S(w, 52), it.rc.right, mid - S(w, 10) };
        Ui_Glyph(dc, L"\xE7C3", g, w->fIconLg, Ui_Mix(th.textDim, th.bg, 0.35f));
        RECT t = { it.rc.left, mid, it.rc.right, mid + S(w, 28) };
        Ui_Text(dc, T(S_EX_EMPTY), t, w->fBody, th.textDim, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
    } else {
        int step = S(w, LROW_H) + S(w, LROW_GAP);
        bool bar = ListContentH(w) > it.rc.bottom - it.rc.top;
        int right = it.rc.right - (bar ? S(w, 12) : 0);
        int r = S(w, 10);
        for (int i = 0; i < cnt; ++i) {
            int y = it.rc.top - w->exScroll + i * step;
            if (y + S(w, LROW_H) < it.rc.top) continue;
            if (y > it.rc.bottom) break;
            RECT row = { it.rc.left, y, right, y + S(w, LROW_H) };
            bool hot = (w->exHot == i), sel = (w->exSel == i);
            Ui_FillRound(dc, row, r, hot ? Ui_Mix(th.surface, th.surfaceHover, 0.8f) : th.surface);
            Ui_StrokeRound(dc, row, r, sel ? th.accent : th.border, sel ? 2 : 1);
            RECT g = { row.left + S(w, 12), row.top, row.left + S(w, 40), row.bottom };
            Ui_Glyph(dc, L"\xE7C3", g, w->fIcon, th.textDim);
            wchar_t name[MAX_PATH];
            if (ExGet(w->ex, i, name, MAX_PATH)) {
                RECT t = { row.left + S(w, 48), row.top, row.right - S(w, 48), row.bottom };
                Ui_Text(dc, name, t, w->fBody, th.text, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
            }
            if (hot || sel) {
                // hover'da ko'rinadigan "×"
                RECT x = { row.right - S(w, 40), row.top + S(w, 6), row.right - S(w, 8), row.bottom - S(w, 6) };
                bool xh = hot && w->exHotX;
                if (xh) Ui_FillCircle(dc, (x.left + x.right) / 2, (x.top + x.bottom) / 2, S(w, 14),
                                      Ui_Mix(th.surface, th.danger, 0.22f));
                Ui_Glyph(dc, L"\xE711", x, w->fIconSm, xh ? th.danger : th.textDim);
            }
        }
        if (bar) {
            // yupqa scrollbar ko'rsatkichi
            int view = it.rc.bottom - it.rc.top, total = ListContentH(w);
            int th_ = view * view / total;
            if (th_ < S(w, 24)) th_ = S(w, 24);
            int maxS = total - view;
            int ty = it.rc.top + (maxS > 0 ? (view - th_) * w->exScroll / maxS : 0);
            RECT sb = { it.rc.right - S(w, 5), ty, it.rc.right - S(w, 2), ty + th_ };
            Ui_FillRound(dc, sb, S(w, 2), Ui_Mix(th.bg, th.textDim, 0.6f));
        }
    }
    RestoreDC(dc, saved);
    if (w->kbd && w->focus == it.id) Ui_DrawFocusRing(dc, it.rc, S(w, 10));
}

// Ogohlantirish pill'i (glif + matn). Kengligini qaytaradi.
static int DrawWarnPill(Win* w, HDC dc, COLORREF base, int x, int y, int h, const wchar_t* text, int maxW)
{
    const UiTheme& th = Ui_Theme();
    COLORREF wc = WarnOf(th);
    int pw = TextW(dc, w->fSmall, text) + S(w, 46);
    if (pw > maxW) pw = maxW;
    RECT r = { x, y, x + pw, y + h };
    Ui_FillRound(dc, r, h / 2, Ui_Mix(base, wc, th.dark ? 0.18f : 0.14f));
    Ui_StrokeRound(dc, r, h / 2, Ui_Mix(base, wc, 0.55f), 1);
    RECT g = { r.left + S(w, 8), r.top, r.left + S(w, 30), r.bottom };
    Ui_Glyph(dc, L"\xE7BA", g, w->fIconSm, wc);
    RECT t = { r.left + S(w, 32), r.top, r.right - S(w, 12), r.bottom };
    Ui_Text(dc, text, t, w->fSmall, wc, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    return pw;
}

// Sinov maydoni kartochkasi: yumaloq karta, fokusda accent porlash, rejim badge,
// (rejim saqlanmagan bo'lsa — ogohlantirish pill'i) va maxsus harflar hisoblagichi
static void DrawPgCard(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    float foc = w->an[AN_PGFOC].value;
    int rad = S(w, 14);
    Ui_FillRound(dc, it.rc, rad, th.surface);
    for (int i = 3; i >= 1 && foc > 0.01f; --i) {       // yumshoq porlash: kengayib so'nuvchi halqalar
        RECT g = it.rc;
        InflateRect(&g, S(w, i * 2), S(w, i * 2));
        Ui_StrokeRound(dc, g, rad + S(w, i * 2), Ui_Mix(th.bg, th.accent, 0.22f * foc * (float)(4 - i) / 3.f), S(w, 2));
    }
    Ui_StrokeRound(dc, it.rc, rad, Ui_Mix(th.border, th.accent, foc), foc > 0.5f ? 2 : 1);

    int by = it.rc.top + S(w, 12), bh = S(w, 24);
    if (w->mode != g_settings.mode) {
        // tanlangan rejim hali saqlanmagan: maydon eski rejimda ishlaydi
        DrawWarnPill(w, dc, th.surface, it.rc.left + S(w, 16), by, bh, T(S_PG_UNSAVED), it.rc.right - it.rc.left - S(w, 32));
    } else {
        wchar_t badge[96];
        wsprintfW(badge, T(S_PG_MODE_BADGE), T(g_settings.mode == MODE_AUTO ? S_OSD_MODE_AUTO : S_OSD_MODE_TRIGGER));
        int bw = TextW(dc, w->fSmall, badge) + S(w, 32);
        RECT r = { it.rc.right - S(w, 16) - bw, by, it.rc.right - S(w, 16), by + bh };
        Ui_FillRound(dc, r, bh / 2, Ui_Mix(th.surface, th.accent, th.dark ? 0.22f : 0.10f));
        Ui_FillCircle(dc, r.left + S(w, 12), (r.top + r.bottom) / 2, S(w, 3), th.accent);
        RECT t = { r.left + S(w, 22), r.top, r.right - S(w, 10), r.bottom };
        Ui_Text(dc, badge, t, w->fSmall, th.accent, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    }

    // hisoblagich (pastki o'ng): son o'zgarganda "bump"
    wchar_t cn[64];
    wsprintfW(cn, T(S_PG_COUNTER), w->pgTotal);
    float bump = w->an[AN_BUMP].value;
    int up = (int)(bump * (float)S(w, 4));
    RECT c = { it.rc.right - S(w, 200), it.rc.bottom - S(w, 30) - up, it.rc.right - S(w, 16), it.rc.bottom - S(w, 8) - up };
    Ui_Text(dc, cn, c, w->fCap, Ui_Mix(th.textDim, th.accent, bump), DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX);
}

// "Almashtirish izi": oxirgi 4 ta chip; yangisi chapdan sirpanib kiradi, eskisi o'ngda so'nadi
static void DrawChips(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    int cw = S(w, 116), gap = S(w, 8), h = it.rc.bottom - it.rc.top;
    for (int i = 0; i < CHIP_MAX; ++i) {
        const Chip& c = w->chips[i];
        if (!c.used) continue;
        float in = c.in.value;
        int x = it.rc.left + (int)(c.pos.value * (float)(cw + gap)) - (int)((1.f - in) * (float)S(w, 28));
        RECT r = { x, it.rc.top, x + cw, it.rc.bottom };
        COLORREF face = Ui_Mix(th.bg, th.accent, th.dark ? 0.24f : 0.13f);
        Ui_FillRound(dc, r, h / 2, Ui_Mix(th.bg, face, in));
        Ui_StrokeRound(dc, r, h / 2, Ui_Mix(th.bg, th.accent, 0.55f * in), 1);
        // oddiy (yarim qalin emas) 14 px: kichik qalin shriftda ö ning ikki nuqtasi qo'shilib "ō" ga o'xshab qoladi
        Ui_Text(dc, c.text, r, w->fBody, Ui_Mix(th.bg, th.text, in), DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);

        // bir martalik porlash: chip atrofidan tarqalib so'nuvchi kichik doiralar
        float sp = c.sp.value;
        if (c.sp.running && sp < 0.98f) {
            static const int fx[5] = { 12, 50, 88, 30, 70 };        // chip eni bo'yicha foiz
            static const int dir[5] = { -1, -1, -1, 1, 1 };         // yuqoriga / pastga
            for (int k = 0; k < 5; ++k) {
                int px = r.left + cw * fx[k] / 100;
                int py = (dir[k] < 0 ? r.top : r.bottom) + dir[k] * (S(w, 3) + (int)(sp * (float)S(w, 14)));
                Ui_FillCircle(dc, px, py, S(w, 3) - (int)(sp * 1.5f), th.accent, (BYTE)(230.f * (1.f - sp)));
            }
        }
    }
}

static void DrawChall(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    int i = it.sub;
    float pop = w->an[AN_CH + i].value;
    COLORREF g = GreenOf(th);
    int rad = S(w, 10);
    Ui_FillRound(dc, it.rc, rad, Ui_Mix(th.surface, g, 0.12f * pop));
    Ui_StrokeRound(dc, it.rc, rad, Ui_Mix(th.border, g, pop), 1);

    int cx = it.rc.left + S(w, 22), cy = (it.rc.top + it.rc.bottom) / 2, r0 = S(w, 9);
    Ui_FillCircle(dc, cx, cy, r0, Ui_Mix(th.border, th.textDim, 0.45f));     // bo'sh halqa
    Ui_FillCircle(dc, cx, cy, r0 - (S(w, 2) > 1 ? S(w, 2) : 2) + 1, th.surface);
    if (pop > 0.02f) {
        // "pop": biroz oshib ketib joyiga qaytadigan masshtab
        float sc = pop < 0.6f ? pop / 0.6f * 1.18f : 1.18f - (pop - 0.6f) / 0.4f * 0.18f;
        int rr = (int)((float)r0 * sc + 0.5f);
        Ui_FillCircle(dc, cx, cy, rr, g);
        RECT gl = { cx - rr, cy - rr, cx + rr, cy + rr };
        Ui_Glyph(dc, L"\xE73E", gl, w->fIconSm, Ui_Mix(g, th.onAccent, pop));
    }
    RECT t = { it.rc.left + S(w, 42), it.rc.top, it.rc.right - S(w, 10), it.rc.bottom };
    Ui_Text(dc, it.text, t, w->fBody, Ui_Mix(th.text, g, 0.55f * pop), DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
}

// "Kichik sinovlar" yorlig'i; hammasi bajarilsa yumshoq ravishda "Barchasi bajarildi!" banneriga o'tadi
static void DrawChLbl(Win* w, HDC dc, const Item& it)
{
    const UiTheme& th = Ui_Theme();
    float dn = w->an[AN_DONE].value;
    Ui_Text(dc, it.text, it.rc, w->fCap, Ui_Mix(th.textDim, th.bg, dn), DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    if (dn > 0.01f) {
        const wchar_t* s = T(S_PG_DONE);
        int pw = TextW(dc, w->fCap, s) + S(w, 50);
        RECT r = { it.rc.left, it.rc.top, it.rc.left + pw, it.rc.bottom };
        Ui_FillRound(dc, r, (r.bottom - r.top) / 2, Ui_Mix(th.bg, GreenOf(th), dn));
        RECT gl = { r.left + S(w, 8), r.top, r.left + S(w, 30), r.bottom };
        Ui_Glyph(dc, L"\xE73E", gl, w->fIconSm, Ui_Mix(th.bg, th.onAccent, dn));
        RECT t = { r.left + S(w, 32), r.top, r.right - S(w, 12), r.bottom };
        Ui_Text(dc, s, t, w->fCap, Ui_Mix(th.bg, th.onAccent, dn), DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    }
}

static void DrawItem(Win* w, HDC dc, const Item& it)
{
    float hv = w->an[AN_HOV + (it.id >= 0 ? it.id : 0)].value;
    bool focused = w->kbd && w->focus == it.id;
    switch (it.kind) {
    case K_ROW:    DrawRowCard(w, dc, it); break;
    case K_TEXT:   DrawText_(w, dc, it); break;
    case K_ILLU:   DrawIllu(w, dc, it); break;
    case K_ICON:
        if (w->iconBig) DrawIconEx(dc, it.rc.left, it.rc.top, w->iconBig,
                                   it.rc.right - it.rc.left, it.rc.bottom - it.rc.top, 0, NULL, DI_NORMAL);
        break;
    case K_TOGGLE: DrawToggleItem(w, dc, it); break;
    case K_DROP:   DrawDrop(w, dc, it); break;
    case K_SEG:    DrawSeg(w, dc, it); break;
    case K_CARD:
        Ui_DrawCard(dc, it.rc, CardSelected(w, it.id), hv, w->an[AN_TOG + it.id].value);
        if (focused) Ui_DrawFocusRing(dc, it.rc, S(w, 12));
        break;
    case K_BTN:
        Ui_DrawButton(dc, it.rc, it.text, w->fBody, (UiBtnKind)it.sub, hv,
                      w->pressed == it.id ? 1.f : 0.f, it.enabled, focused);
        break;
    case K_PGEDIT: DrawPgCard(w, dc, it); break;
    case K_CHIPS:  DrawChips(w, dc, it); break;
    case K_CHALL:  DrawChall(w, dc, it); break;
    case K_CHLBL:  DrawChLbl(w, dc, it); break;
    case K_PILL:   DrawWarnPill(w, dc, Ui_Theme().bg, it.rc.left, it.rc.top, it.rc.bottom - it.rc.top, it.text,
                                it.rc.right - it.rc.left); break;
    case K_HK:     DrawHkField(w, dc, it); break;
    case K_HKX:    DrawHkx(w, dc, it); break;
    case K_LIST:   DrawList(w, dc, it); break;
    case K_EDIT:
        Ui_DrawInputFrame(dc, it.rc, GetFocus() == w->edit, 0.f);   // EDIT foni doim surface — hover farqi chiqmasin
        break;
    }
}

static void DrawPageItems(Win* w, HDC dc)
{
    for (int i = 0; i < w->nit; ++i)
        if (w->it[i].page) DrawItem(w, dc, w->it[i]);
}

static void Paint(Win* w, HDC dc)
{
    const UiTheme& th = Ui_Theme();
    RECT all = { 0, 0, w->cw, w->ch };
    HBRUSH bg = CreateSolidBrush(th.bg);
    FillRect(dc, &all, bg);
    DeleteObject(bg);

    // sidebar: ajratuvchi chiziq + brend + bandlar
    RECT sep = { S(w, SIDE_W) - 1, 0, S(w, SIDE_W), w->ch };
    HBRUSH bb = CreateSolidBrush(th.border);
    FillRect(dc, &sep, bb);
    // pastki panel ustidagi chiziq
    RECT bsep = { S(w, SIDE_W), w->ch - S(w, BAR_H), w->cw, w->ch - S(w, BAR_H) + 1 };
    FillRect(dc, &bsep, bb);
    DeleteObject(bb);
    DrawBrand(w, dc);
    DrawNav(w, dc);
    RECT ver = { S(w, 20), w->ch - S(w, 44), S(w, SIDE_W - 12), w->ch - S(w, 20) };
    Ui_Text(dc, L"v1.0.0", ver, w->fSmall, th.textDim, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

    // kontent: sahifa almashuvi paytida fade + 8 px siljish
    float pa = w->an[AN_PAGE].value;
    if (w->snap && w->alphaBlend && w->an[AN_PAGE].running) {
        float e = pa;
        int dy = (int)((1.f - e) * (float)S(w, 8));
        HDC sdc = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(sdc, w->snap);
        int x = S(w, SIDE_W), cwid = w->cw - x, chei = w->ch - S(w, BAR_H) - dy;
        BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)(e * 255.f), 0 };
        w->alphaBlend(dc, x, dy, cwid, chei, sdc, x, 0, cwid, chei, bf);
        SelectObject(sdc, old);
        DeleteDC(sdc);
    } else {
        DrawPageItems(w, dc);
    }

    // pastki panel tugmalari
    for (int i = 0; i < w->nit; ++i)
        if (!w->it[i].page && w->it[i].kind == K_BTN) DrawItem(w, dc, w->it[i]);
}

// Yangi sahifaning suratini oladi (fade uchun)
static void MakeSnapshot(Win* w)
{
    if (w->snap) { DeleteObject(w->snap); w->snap = NULL; }
    HDC wdc = GetDC(w->hwnd);
    HDC mdc = CreateCompatibleDC(wdc);
    w->snap = CreateCompatibleBitmap(wdc, w->cw, w->ch);
    ReleaseDC(w->hwnd, wdc);
    if (!w->snap) { DeleteDC(mdc); return; }
    HGDIOBJ old = SelectObject(mdc, w->snap);
    const UiTheme& th = Ui_Theme();
    RECT all = { 0, 0, w->cw, w->ch };
    HBRUSH bg = CreateSolidBrush(th.bg);
    FillRect(mdc, &all, bg);
    DeleteObject(bg);
    DrawPageItems(w, mdc);
    SelectObject(mdc, old);
    DeleteDC(mdc);
}

// ---------------------------------------------------------------------
//  Holat o'zgarishlari
// ---------------------------------------------------------------------

static void SetFocusId(Win* w, int id, bool kbd);

// Native EDIT lar faqat o'z sahifasida (va sahifa almashuvi animatsiyasi tugagach) ko'rinadi.
// Sinov maydoniga kirilganda fokus shu yerga qo'yiladi.
static void ShowPageEdits(Win* w)
{
    bool tr = w->an[AN_PAGE].running;
    bool ex = (w->page == SP_EXCLUDE && !tr), pg = (w->page == SP_PLAYGROUND && !tr);
    ShowWindow(w->edit,   ex ? SW_SHOW : SW_HIDE);
    ShowWindow(w->pgEdit, pg ? SW_SHOW : SW_HIDE);
    if (pg && w->wantPgFocus) {
        w->wantPgFocus = false;
        SetFocusId(w, ID_PGEDIT, false);
    }
}

static void Relayout(Win* w)
{
    Layout(w);
    ShowPageEdits(w);
    Redraw(w);
}

static void SetFocusId(Win* w, int id, bool kbd)
{
    w->focus = id;
    w->kbd = kbd;
    if (id == ID_EDIT) SetFocus(w->edit);
    else if (id == ID_PGEDIT) SetFocus(w->pgEdit);
    else if (GetFocus() == w->edit || GetFocus() == w->pgEdit) SetFocus(w->hwnd);
    Redraw(w);
}

static void GoPage(Win* w, int page, bool force = false, bool takeFocus = true)
{
    if (page < 0) page = 0;
    if (page > SP_PLAYGROUND) page = SP_PLAYGROUND;
    if (page == w->page && !force) return;
    if (w->rec >= 0) w->rec = -1;
    bool first = (w->page == page);     // force: ilk ko'rsatish
    w->page = page;
    Layout(w);
    SetAnim(w->an[AN_PGFOC], 0.f);
    w->wantPgFocus = takeFocus && page == SP_PLAYGROUND;

    if (!first) {
        AnimTo(w, AN_NAVY, (float)NavTop(w, NavOf(page)), 240);
        SetAnim(w->an[AN_PAGE], 0.f);
        Ui_AnimTo(w->an[AN_PAGE], 1.f, 180);
        if (w->an[AN_PAGE].running && w->alphaBlend) {
            MakeSnapshot(w);
            Kick(w);
        } else {
            SetAnim(w->an[AN_PAGE], 1.f);
        }
    }
    if (w->focus >= ID_NAV0 && w->focus <= ID_NAV5) w->focus = ID_NAV0 + NavOf(page);
    else if (ItemIndex(w, w->focus) < 0) w->focus = ID_NAV0 + NavOf(page);
    ShowPageEdits(w);
    Redraw(w);
}

static void SetMode(Win* w, InputMode m)
{
    if (w->mode == m) return;
    w->mode = m;
    AnimTo(w, AN_TOG + ID_CARD_TRIG, m == MODE_TRIGGER ? 1.f : 0.f, 200);
    AnimTo(w, AN_TOG + ID_CARD_AUTO, m == MODE_AUTO ? 1.f : 0.f, 200);
    Relayout(w);
}

static void SetLang(Win* w, int l)
{
    if (l < 0 || l >= LANG_COUNT || (int)g_settings.lang == l) return;
    g_settings.lang = (Lang)l;      // jonli ko'rinish; Bekor qilinsa qaytariladi
    AnimTo(w, AN_SEG, (float)l, 220);
    SetWindowTextW(w->hwnd, T(S_SET_TITLE));
    Relayout(w);
}

static void ToggleItem(Win* w, int id)
{
    bool v = false;
    switch (id) {
    case ID_STARTUP: w->startup = !w->startup; v = w->startup; break;
    case ID_NOTIFY:  w->notify  = !w->notify;  v = w->notify;  break;
    default:
        if (id >= ID_RULE_O && id <= ID_RULE_CH) {
            UINT bit = 1u << (id - ID_RULE_O);      // AUTO_RULE_O..CH = 1,2,4,8
            w->rules ^= bit;
            v = (w->rules & bit) != 0;
        } else return;
    }
    AnimTo(w, AN_TOG + id, v ? 1.f : 0.f, 160);
    Redraw(w);
}

// Tanlov menyusi (native kontekst menyu) — qiymat indeksini qaytaradi, bekor bo'lsa -1
static int PopupChoose(Win* w, const Item& it, const wchar_t* const* labels, int n, int cur)
{
    HMENU m = CreatePopupMenu();
    for (int i = 0; i < n; ++i)
        AppendMenuW(m, MF_STRING | (i == cur ? MF_CHECKED : 0), (UINT_PTR)(1000 + i), labels[i]);
    POINT pt = { it.rc.left, it.rc.bottom + S(w, 4) };
    ClientToScreen(w->hwnd, &pt);
    SetForegroundWindow(w->hwnd);
    int cmd = (int)TrackPopupMenu(m, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN,
                                  pt.x, pt.y, 0, w->hwnd, NULL);
    DestroyMenu(m);
    w->pressed = -1;
    return cmd >= 1000 ? cmd - 1000 : -1;
}

static void OpenDrop(Win* w, int id)
{
    int ii = ItemIndex(w, id);
    if (ii < 0) return;
    const wchar_t* lab[16];
    int n = 0, cur = -1;
    if (id == ID_APO) {
        for (int i = 0; i < kApoOptCount; ++i) {
            lab[n++] = kApoOpts[i].label;
            if (kApoOpts[i].ch == w->apo) cur = i;
        }
    } else {
        for (int i = 0; i < kTrigOptCount; ++i) {
            lab[n++] = T(kTrigOpts[i].name);
            if (kTrigOpts[i].vk == w->trig) cur = i;
        }
    }
    int pick = PopupChoose(w, w->it[ii], lab, n, cur);
    if (pick < 0) return;
    if (id == ID_APO) w->apo = kApoOpts[pick].ch;
    else              w->trig = kTrigOpts[pick].vk;
    Relayout(w);
}

static void ListRemove(Win* w, int idx)
{
    if (idx < 0 || idx >= ExCount(w->ex)) return;
    ExRemove(w->ex, idx);
    int cnt = ExCount(w->ex);
    if (w->exSel >= cnt) w->exSel = cnt - 1;
    if (w->exHot >= cnt) w->exHot = -1;
    Relayout(w);
}

static void AddFromEdit(Win* w)
{
    wchar_t raw[MAX_PATH];
    GetWindowTextW(w->edit, raw, MAX_PATH);
    if (ExAdd(w, raw)) SetWindowTextW(w->edit, L"");
    Relayout(w);
    ScrollToSel(w);
    Redraw(w);
    SetFocusId(w, ID_EDIT, w->kbd);
}

// "Tanlaş..." — fayl dialogi. comdlg32 link qilinmaydi: faqat shu tugma bosilganda
// yuklanadi va darhol bo'shatiladi (RAM budjeti).
static void Browse(Win* w)
{
    HMODULE dll = LoadLibraryExW(L"comdlg32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!dll) return;
    typedef BOOL (WINAPI *PFN_GetOpenFileNameW)(LPOPENFILENAMEW);
    PFN_GetOpenFileNameW fn = (PFN_GetOpenFileNameW)GetProcAddress(dll, "GetOpenFileNameW");
    if (fn) {
        // filtr: "<nom>\0*.exe\0\0"
        wchar_t filter[160] = L"";
        lstrcpynW(filter, T(S_EX_FILTER), 128);
        int fl = lstrlenW(filter);
        lstrcpyW(filter + fl + 1, L"*.exe");    // keyingi \0 lar massiv nollari bilan to'ldirilgan

        wchar_t file[MAX_PATH] = L"";
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = w->hwnd;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile   = file;
        ofn.nMaxFile    = MAX_PATH;
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        if (fn(&ofn)) { ExAdd(w, file); Relayout(w); ScrollToSel(w); }
    }
    FreeLibrary(dll);
    w->pressed = -1;
    Redraw(w);
}

static void StartRecord(Win* w, int k)
{
    w->rec = k;
    SetFocusId(w, ID_HK0 + k, true);
    SetFocus(w->hwnd);
    Relayout(w);
    Kick(w);                // pulsatsiya taymeri (yozib olish tugaguncha)
}

static void StopRecord(Win* w)
{
    w->rec = -1;
    Relayout(w);
}

// Yozib olish paytida bosilgan tugma
static void RecordKey(Win* w, UINT vk, LPARAM lp)
{
    int k = w->rec;
    if (vk == VK_ESCAPE) { StopRecord(w); return; }
    if (vk == VK_BACK || vk == VK_DELETE) { w->hk[k] = 0; StopRecord(w); return; }
    if (IsModVk(vk)) return;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) return;     // Win kombinatsiyalari qo'llanmaydi

    BYTE m = 0;
    if (GetKeyState(VK_SHIFT)   & 0x8000) m |= HOTKEYF_SHIFT;
    if (GetKeyState(VK_CONTROL) & 0x8000) m |= HOTKEYF_CONTROL;
    if (GetKeyState(VK_MENU)    & 0x8000) m |= HOTKEYF_ALT;
    if (lp & (1 << 24))           m |= HOTKEYF_EXT;
    WORD hk = MAKEWORD((BYTE)vk, m);
    if (!HkAllowed(hk)) {
        // taqiqlangan kombinatsiya: yozib olish davom etadi, ramka qisqa qizaradi
        SetAnim(w->an[AN_ERR + k], 1.f);
        AnimTo(w, AN_ERR + k, 0.f, 700);
        MessageBeep(MB_OK);
        Redraw(w);
        return;
    }
    w->hk[k] = hk;
    StopRecord(w);
}

// ---------------------------------------------------------------------
//  Saqlash
// ---------------------------------------------------------------------

// Ikki maydonda bir xil kombinatsiya bo'lsa ogohlantiradi va sahifani ko'rsatadi
static bool HotkeysConflict(Win* w)
{
    for (int i = 0; i < 3; ++i)
        for (int j = i + 1; j < 3; ++j)
            if (w->hk[i] && w->hk[i] == w->hk[j]) {
                wchar_t key[64], msg[256];
                Hotkey_Format(w->hk[i], key, 64);
                wsprintfW(msg, T(S_HK_BUSY), key);
                GoPage(w, SP_HOTKEYS);
                SetFocusId(w, ID_HK0 + j, true);
                MessageBoxW(w->hwnd, msg, T(S_SET_TITLE), MB_OK | MB_ICONWARNING);
                return true;
            }
    return false;
}

// Hotkeys_Apply natijasidagi band tugmalar haqida bitta xabar
static void WarnBusyHotkeys(Win* w, UINT busy)
{
    const struct { UINT bit; WORD hk; } keys[3] = {
        { HKID_MODE, g_settings.hkMode }, { HKID_ENABLE, g_settings.hkEnable },
        { HKID_CONVERT, g_settings.hkConvert },
    };
    wchar_t msg[768] = L"";
    int len = 0;
    for (int i = 0; i < 3; ++i) {
        if (!(busy & keys[i].bit)) continue;
        wchar_t key[64], line[256];
        Hotkey_Format(keys[i].hk, key, 64);
        wsprintfW(line, T(S_HK_BUSY), key);
        int ll = lstrlenW(line);
        if (len + ll + 2 >= 768) break;
        if (len) msg[len++] = L'\n';
        lstrcpyW(msg + len, line);
        len += ll;
    }
    if (len) MessageBoxW(w->hwnd, msg, T(S_SET_TITLE), MB_OK | MB_ICONWARNING);
}

// Qiymatlarni g_settings ga yozib saqlaydi; false — saqlash to'xtatildi (oyna ochiq qoladi)
static bool SaveAll(Win* w)
{
    if (HotkeysConflict(w)) return false;

    // maydonda yozilib, "Qöşiş" bosilmagan nom yo'qolmasin
    wchar_t raw[MAX_PATH];
    GetWindowTextW(w->edit, raw, MAX_PATH);
    if (raw[0]) { ExAdd(w, raw); SetWindowTextW(w->edit, L""); }

    // g_settings.lang jonli o'zgargan (SetLang)
    g_settings.apo    = w->apo;
    g_settings.notify = w->notify;

    if (w->startup != Startup_IsEnabled()) Startup_Set(w->startup);
    g_settings.runAtStartup = w->startup;

    g_settings.mode      = w->mode;
    g_settings.triggerVk = w->trig;
    g_settings.autoRules = w->rules;

    g_settings.hkMode    = w->hk[0];
    g_settings.hkEnable  = w->hk[1];
    g_settings.hkConvert = w->hk[2];

    Settings_SetExcludes(w->ex);

    Settings_Save();
    Hook_ResetTriggerState();   // eski triggerning "bosilgan" holati qolmasin

    w->hkApplied = true;
    UINT busy = Hotkeys_Apply();
    if (busy) WarnBusyHotkeys(w, busy);     // saqlash baribir bo'lgan
    return true;
}

static void DoSave(Win* w)
{
    if (SaveAll(w)) w->done = true;
}

static void DoCancel(Win* w)
{
    g_settings.lang = w->origLang;      // jonli o'zgargan til qaytariladi
    w->done = true;
}

// ---------------------------------------------------------------------
//  Kiritish: sichqoncha va klaviatura
// ---------------------------------------------------------------------

static int HitTest(const Win* w, int x, int y)
{
    POINT p = { x, y };
    for (int i = w->nit - 1; i >= 0; --i) {
        const Item& it = w->it[i];
        if (!IsInteractive(it) || !it.enabled) continue;
        if (PtInRect(&it.rc, p)) return i;
    }
    return -1;
}

// Ro'yxatdagi (qator, "×" ustidami) — sichqoncha koordinatalaridan
static void ListHit(const Win* w, const Item& list, int x, int y, int* row, bool* onX)
{
    *row = -1; *onX = false;
    int step = S(w, LROW_H) + S(w, LROW_GAP);
    int rel = y - list.rc.top + w->exScroll;
    if (rel < 0) return;
    int i = rel / step;
    if (i >= ExCount(w->ex) || rel % step >= S(w, LROW_H)) return;
    *row = i;
    bool bar = ListContentH(w) > list.rc.bottom - list.rc.top;
    int right = list.rc.right - (bar ? S(w, 12) : 0);
    *onX = x >= right - S(w, 40);
}

static void SetHot(Win* w, int id)
{
    if (w->hot == id) return;
    if (w->hot >= 0) AnimTo(w, AN_HOV + w->hot, 0.f, 160);
    w->hot = id;
    if (id >= 0) AnimTo(w, AN_HOV + id, 1.f, 120);
    Redraw(w);
}

static void OnMouseMove(Win* w, int x, int y)
{
    if (!w->tracking) {
        TRACKMOUSEEVENT te = { sizeof(te), TME_LEAVE, w->hwnd, 0 };
        TrackMouseEvent(&te);
        w->tracking = true;
    }
    int hi = HitTest(w, x, y);
    SetHot(w, hi >= 0 ? w->it[hi].id : -1);

    int row = -1; bool onX = false;
    if (hi >= 0 && w->it[hi].id == ID_LIST) ListHit(w, w->it[hi], x, y, &row, &onX);
    if (row != w->exHot || onX != w->exHotX) {
        w->exHot = row; w->exHotX = onX;
        Redraw(w);
    }
}

// ---- Sinov maydoni mantig'i ----

// Yangi chip: mavjudlari bir joy o'ngga suriladi, 4-sidan oshgani so'nadi
static void AddChip(Win* w, int k)
{
    wchar_t text[28];
    if (g_settings.mode == MODE_AUTO) {
        lstrcpynW(text, kDigraph[k], 28);
    } else {
        wchar_t cap[24];
        TriggerKeyCap(g_settings.triggerVk, cap, 24);
        wsprintfW(text, L"%s + %c → %c", cap, kBase[k], kSpecial[k]);
    }
    int free_ = -1, oldest = 0;
    for (int i = 0; i < CHIP_MAX; ++i) {
        Chip& c = w->chips[i];
        if (!c.used) { if (free_ < 0) free_ = i; continue; }
        if (c.slot > w->chips[oldest].slot || !w->chips[oldest].used) oldest = i;
        if (c.slot < 4) ++c.slot;
        if (c.slot >= 4 && !c.dying) { c.dying = true; Ui_AnimTo(c.in, 0.f, 320); }
        Ui_AnimTo(c.pos, (float)c.slot, 240);
        if (c.dying && !c.in.running) c.used = false;       // animatsiya o'chiq bo'lsa darhol
    }
    int n = free_ >= 0 ? free_ : oldest;
    Chip& c = w->chips[n];
    ZeroMemory(&c, sizeof(c));
    c.used = true;
    lstrcpyW(c.text, text);
    Ui_AnimTo(c.in, 1.f, 260);
    Ui_AnimTo(c.sp, 1.f, 420);
    Kick(w);
}

// Matn o'zgardi: maxsus harflar farqi -> chip, hisoblagich, kichik sinovlar
static void OnPgChange(Win* w)
{
    wchar_t buf[4097];
    int n = GetWindowTextW(w->pgEdit, buf, 4097);
    int cnt[8] = {}, total = 0;
    for (int i = 0; i < n; ++i)
        for (int k = 0; k < 8; ++k)
            if (buf[i] == kSpecial[k]) { ++cnt[k]; ++total; break; }
    for (int k = 0; k < 8; ++k)
        if (cnt[k] > w->pgCount[k]) AddChip(w, k);
    if (total > w->pgTotal) { SetAnim(w->an[AN_BUMP], 1.f); AnimTo(w, AN_BUMP, 0.f, 380); }
    for (int k = 0; k < 8; ++k) w->pgCount[k] = cnt[k];
    w->pgTotal = total;

    // kichik sinovlar: katta-kichik harfsiz, bajarilgani qaytmaydi (Tozalashgacha)
    CharLowerBuffW(buf, n);
    bool all = true;
    for (int i = 0; i < 4; ++i) {
        if (!w->chDone[i] && wcsstr(buf, kChall[i].target)) {
            w->chDone[i] = true;
            SetAnim(w->an[AN_CH + i], 0.f);
            AnimTo(w, AN_CH + i, 1.f, 380);
        }
        if (!w->chDone[i]) all = false;
    }
    if (all && !w->allDone) { w->allDone = true; AnimTo(w, AN_DONE, 1.f, 320); }

    // placeholder (bo'sh va fokussiz) holati o'zgardi: EDIT ni to'liq qayta chizish
    bool empty = (n == 0);
    if (empty != w->pgEmpty) { w->pgEmpty = empty; InvalidateRect(w->pgEdit, NULL, TRUE); }
    Redraw(w);
}

// Tozalash: maydon, chiplar, sinovlar va hisoblagich
static void PgClear(Win* w)
{
    SetWindowTextW(w->pgEdit, L"");
    ZeroMemory(w->pgCount, sizeof(w->pgCount));
    w->pgTotal = 0;
    ZeroMemory(w->chips, sizeof(w->chips));
    ZeroMemory(w->chDone, sizeof(w->chDone));
    w->allDone = false;
    for (int i = 0; i < 4; ++i) SetAnim(w->an[AN_CH + i], 0.f);
    SetAnim(w->an[AN_DONE], 0.f);
    SetAnim(w->an[AN_BUMP], 0.f);
    w->pgEmpty = true;
    InvalidateRect(w->pgEdit, NULL, TRUE);
    SetFocusId(w, ID_PGEDIT, false);
}

// Sinov maydonining EDIT'i: standart chizishdan keyin placeholder (bo'sh va fokussiz bo'lsa)
static LRESULT CALLBACK PgEditProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    Win* w = (Win*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (!w) return DefWindowProcW(h, msg, wp, lp);
    LRESULT r = CallWindowProcW(w->pgOrig, h, msg, wp, lp);
    if (msg == WM_PAINT && GetWindowTextLengthW(h) == 0 && GetFocus() != h) {
        HDC dc = GetDC(h);
        RECT rc;
        GetClientRect(h, &rc);
        rc.left += 2;
        Ui_Text(dc, T(S_PG_PLACEHOLDER), rc, w->fPg, Ui_Theme().textDim, DT_WORDBREAK | DT_NOPREFIX);
        ReleaseDC(h, dc);
    } else if (msg == WM_SETFOCUS || msg == WM_KILLFOCUS) {
        InvalidateRect(h, NULL, TRUE);
    }
    return r;
}

// Elementni faollashtiradi (sichqoncha: x,y — bosilgan nuqta; klaviatura: markaz)
static void Activate(Win* w, int id, int x, int y)
{
    switch (id) {
    case ID_NAV0: case ID_NAV1: case ID_NAV2: case ID_NAV3: case ID_NAV4: case ID_NAV5:
        GoPage(w, kNavPage[id - ID_NAV0]);
        break;
    case ID_PGEDIT:  SetFocusId(w, ID_PGEDIT, false); break;
    case ID_PGCLEAR: PgClear(w); break;
    case ID_CANCEL: DoCancel(w); break;
    case ID_SAVE:   DoSave(w);   break;
    case ID_LANG: {
        int ii = ItemIndex(w, ID_LANG);
        if (ii < 0) break;
        const RECT& r = w->it[ii].rc;
        int pad = S(w, 3), sw = (r.right - r.left - 2 * pad) / 3;
        int l = (x - r.left - pad) / sw;
        if (l < 0) l = 0;
        if (l > 2) l = 2;
        SetLang(w, l);
        break;
    }
    case ID_APO: case ID_TRIG: OpenDrop(w, id); break;
    case ID_STARTUP: case ID_NOTIFY:
    case ID_RULE_O: case ID_RULE_G: case ID_RULE_SH: case ID_RULE_CH:
        ToggleItem(w, id);
        break;
    case ID_CARD_TRIG: SetMode(w, MODE_TRIGGER); break;
    case ID_CARD_AUTO: SetMode(w, MODE_AUTO);    break;
    case ID_HK0: case ID_HK1: case ID_HK2: StartRecord(w, id - ID_HK0); break;
    case ID_HKX0: case ID_HKX1: case ID_HKX2:
        w->hk[id - ID_HKX0] = 0;
        Relayout(w);
        break;
    case ID_EDIT: SetFocusId(w, ID_EDIT, false); break;
    case ID_ADD:  AddFromEdit(w); break;
    case ID_BROWSE: Browse(w); break;
    case ID_LIST: {
        int ii = ItemIndex(w, ID_LIST);
        if (ii < 0) break;
        int row; bool onX;
        ListHit(w, w->it[ii], x, y, &row, &onX);
        if (row < 0) break;
        if (onX) ListRemove(w, row);
        else { w->exSel = row; ScrollToSel(w); Redraw(w); }
        break;
    }
    case ID_GITHUB:   ShellExecuteW(w->hwnd, L"open", URL_GITHUB, NULL, NULL, SW_SHOWNORMAL); break;
    case ID_LINKEDIN: ShellExecuteW(w->hwnd, L"open", L"https://www.linkedin.com/in/ascoder/", NULL, NULL, SW_SHOWNORMAL); break;
    case ID_LICENSE:  ShowLicense(w->hwnd); break;
    }
}

// Tab / Shift+Tab: fokusga loyiq elementlar orasida aylanadi
static void FocusMove(Win* w, int dir)
{
    int order[MAX_ITEMS], n = 0, cur = -1;
    for (int i = 0; i < w->nit; ++i)
        if (IsFocusable(w, w->it[i])) {
            if (w->it[i].id == w->focus) cur = n;
            order[n++] = w->it[i].id;
        }
    if (!n) return;
    int next = cur < 0 ? (dir > 0 ? 0 : n - 1) : (cur + dir + n) % n;
    SetFocusId(w, order[next], true);
}

// Fokusdagi elementda Enter bosilsa shu element ishlaydimi (aks holda — Saqlash)
static bool EnterActivates(const Win* w)
{
    int i = ItemIndex(w, w->focus);
    if (i < 0) return false;
    switch (w->it[i].kind) {
    case K_BTN: case K_DROP: case K_HK: case K_CARD: return true;
    }
    return false;
}

static void ActivateFocused(Win* w)
{
    int i = ItemIndex(w, w->focus);
    if (i < 0 || w->it[i].kind == K_SEG) return;     // segmentda faqat strelkalar ishlaydi
    const RECT& r = w->it[i].rc;
    Activate(w, w->focus, (r.left + r.right) / 2, (r.top + r.bottom) / 2);
}

// Klaviatura xabarlarini oyna tsikli darajasida ushlaydi. true — xabar sarflandi.
static bool OnKey(Win* w, const MSG& m)
{
    bool down = (m.message == WM_KEYDOWN || m.message == WM_SYSKEYDOWN);
    bool up   = (m.message == WM_KEYUP   || m.message == WM_SYSKEYUP);
    bool chr  = (m.message == WM_CHAR    || m.message == WM_SYSCHAR);
    if (!down && !up && !chr) return false;

    // hotkey yozib olish: hamma tugma (Alt, F10, Esc ... ) shu yerda sarflanadi
    if (w->rec >= 0) {
        if (down) RecordKey(w, (UINT)m.wParam, m.lParam);
        return true;
    }
    if (!down) return false;

    UINT vk = (UINT)m.wParam;
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool inEx = (w->focus == ID_EDIT && GetFocus() == w->edit);
    bool inPg = (w->focus == ID_PGEDIT && GetFocus() == w->pgEdit);
    bool inEdit = inEx || inPg;

    switch (vk) {
    case VK_TAB:
        if (ctrl) GoPage(w, kNavPage[(NavOf(w->page) + (shift ? NAV_N - 1 : 1)) % NAV_N]);   // Ctrl+Tab: keyingi sahifa
        else      FocusMove(w, shift ? -1 : 1);
        return true;
    case VK_ESCAPE:
        if (inPg) {                                 // birinchi Esc — fokusni maydondan chiqaradi
            SetFocusId(w, ID_NAV0 + NavOf(w->page), true);
            return true;
        }
        DoCancel(w);
        return true;
    case 0x41:                                      // Ctrl+A: hammasini belgilash
        if (ctrl && inEdit) { SendMessageW(inPg ? w->pgEdit : w->edit, EM_SETSEL, 0, -1); return true; }
        return false;
    case VK_RETURN:
        if (inPg) return false;                     // sinov maydonida Enter — yangi qator
        if (inEx) { AddFromEdit(w); return true; }
        if (EnterActivates(w)) ActivateFocused(w);
        else DoSave(w);
        return true;
    case VK_SPACE:
        if (inEdit) return false;
        ActivateFocused(w);
        return true;
    case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT: {
        if (inEdit) return false;
        int d = (vk == VK_UP || vk == VK_LEFT) ? -1 : 1;
        int fi = ItemIndex(w, w->focus);
        int kind = fi >= 0 ? w->it[fi].kind : -1;
        if (kind == K_NAV && (vk == VK_UP || vk == VK_DOWN)) {
            int p = NavOf(w->page) + d;
            if (p >= 0 && p < NAV_N) { GoPage(w, kNavPage[p], false, false); SetFocusId(w, ID_NAV0 + p, true); }
        } else if (kind == K_SEG && (vk == VK_LEFT || vk == VK_RIGHT)) {
            SetLang(w, (int)g_settings.lang + d);
            w->kbd = true;
        } else if (kind == K_CARD) {
            SetMode(w, d < 0 ? MODE_TRIGGER : MODE_AUTO);
            SetFocusId(w, d < 0 ? ID_CARD_TRIG : ID_CARD_AUTO, true);
        } else if (kind == K_LIST && (vk == VK_UP || vk == VK_DOWN)) {
            int cnt = ExCount(w->ex);
            if (cnt) {
                int s = w->exSel < 0 ? (d > 0 ? 0 : cnt - 1) : w->exSel + d;
                if (s < 0) s = 0;
                if (s >= cnt) s = cnt - 1;
                w->exSel = s;
                ScrollToSel(w);
                w->kbd = true;
                Redraw(w);
            }
        } else {
            return false;
        }
        return true;
    }
    case VK_DELETE: {
        int fi = ItemIndex(w, w->focus);
        if (inEdit) return false;
        if (fi >= 0 && w->it[fi].kind == K_LIST) { ListRemove(w, w->exSel); return true; }
        if (fi >= 0 && w->it[fi].kind == K_HK) { w->hk[w->it[fi].id - ID_HK0] = 0; Relayout(w); return true; }
        return false;
    }
    case VK_BACK: {
        int fi = ItemIndex(w, w->focus);
        if (fi >= 0 && w->it[fi].kind == K_HK && !inEdit) { w->hk[w->it[fi].id - ID_HK0] = 0; Relayout(w); return true; }
        return false;
    }
    }
    return false;
}

// ---------------------------------------------------------------------
//  Oyna proseduri
// ---------------------------------------------------------------------

static void MakeFonts(Win* w)
{
    w->fTitle  = Ui_Font(w->dpi, 28, FW_SEMIBOLD);
    w->fHead   = Ui_Font(w->dpi, 15, FW_SEMIBOLD);
    w->fBody   = Ui_Font(w->dpi, 14, FW_NORMAL);
    w->fSmall  = Ui_Font(w->dpi, 13, FW_NORMAL);
    w->fCap    = Ui_Font(w->dpi, 13, FW_SEMIBOLD);
    w->fBrand  = Ui_Font(w->dpi, 18, FW_SEMIBOLD);
    w->fIcon   = Ui_IconFont(w->dpi, 18);
    w->fIconSm = Ui_IconFont(w->dpi, 12);
    w->fIconLg = Ui_IconFont(w->dpi, 40);
    w->fPg     = Ui_Font(w->dpi, 20, FW_NORMAL);       // sinov maydoni matni
    w->fRule   = Ui_Font(w->dpi, 15, FW_NORMAL);       // "o' → ö" qoida yozuvlari (kichik o'lchamda ö aniq o'qilsin)
    w->fResult = Ui_Font(w->dpi, 34, FW_SEMIBOLD);     // illyustratsiyadagi natija harfi (ö ikki nuqtasi aniq ko'rinsin)
}

static void FreeRes(Win* w)
{
    HFONT* fs[] = { &w->fTitle, &w->fHead, &w->fBody, &w->fSmall, &w->fCap, &w->fBrand,
                    &w->fIcon, &w->fIconSm, &w->fIconLg, &w->fPg, &w->fResult, &w->fRule };
    for (size_t i = 0; i < sizeof(fs) / sizeof(fs[0]); ++i)
        if (*fs[i]) { DeleteObject(*fs[i]); *fs[i] = NULL; }
    if (w->icon)    { DestroyIcon(w->icon);    w->icon = NULL; }
    if (w->iconBig) { DestroyIcon(w->iconBig); w->iconBig = NULL; }
    if (w->editBr)  { DeleteObject(w->editBr); w->editBr = NULL; }
    if (w->snap)    { DeleteObject(w->snap);   w->snap = NULL; }
    if (w->msimg)   { FreeLibrary(w->msimg);   w->msimg = NULL; w->alphaBlend = NULL; }
}

static void UpdateEditBrush(Win* w)
{
    const UiTheme& th = Ui_Theme();
    if (w->editBr) DeleteObject(w->editBr);
    w->editBrCol = th.surface;
    w->editBr = CreateSolidBrush(w->editBrCol);
}

// Animatsiya taymeri: hamma animatsiyani bir qadam suradi
static void OnTimer(Win* w)
{
    bool run = false;
    for (int i = 0; i < AN_COUNT; ++i)
        if (w->an[i].running && Ui_AnimStep(w->an[i])) run = true;
    for (int i = 0; i < CHIP_MAX; ++i) {
        Chip& c = w->chips[i];
        if (!c.used) continue;
        if (c.in.running && Ui_AnimStep(c.in)) run = true;
        if (c.pos.running && Ui_AnimStep(c.pos)) run = true;
        if (c.sp.running && Ui_AnimStep(c.sp)) run = true;
        if (c.dying && !c.in.running) c.used = false;       // so'nib bo'ldi
    }

    // sahifa almashuvi tugadi: surat o'chiriladi, EDIT ko'rsatiladi
    if (w->snap && !w->an[AN_PAGE].running) {
        DeleteObject(w->snap);
        w->snap = NULL;
        ShowPageEdits(w);
    }
    Redraw(w);
    if (!run && w->rec < 0) {
        KillTimer(w->hwnd, UI_ANIM_TIMER);
        w->timerOn = false;
    }
}

static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Win* w = (Win*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        w = (Win*)((CREATESTRUCTW*)lp)->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)w);
        w->hwnd = hwnd;
        Ui_Begin();
        w->dpi = Ui_Dpi(hwnd);
        RECT rc;
        GetClientRect(hwnd, &rc);
        w->cw = rc.right; w->ch = rc.bottom;

        MakeFonts(w);
        int isz = Ui_S(32, w->dpi), big = Ui_S(72, w->dpi);
        w->icon    = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, isz, isz, LR_DEFAULTCOLOR);
        w->iconBig = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, big, big, LR_DEFAULTCOLOR);

        // sahifa almashuvidagi fade uchun (faqat shu oyna ochiq turganda yuklanadi)
        w->msimg = LoadLibraryExW(L"msimg32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (w->msimg)
            w->alphaBlend = (BOOL (WINAPI*)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION))
                            GetProcAddress(w->msimg, "AlphaBlend");

        UpdateEditBrush(w);
        w->edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL,
                                  0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_EDIT, g_hInst, NULL);
        SendMessageW(w->edit, WM_SETFONT, (WPARAM)w->fBody, FALSE);
        SendMessageW(w->edit, EM_LIMITTEXT, MAX_PATH - 1, 0);
        SendMessageW(w->edit, 0x1501 /*EM_SETCUEBANNER*/, TRUE, (LPARAM)L"notepad.exe");

        // sinov maydoni: oddiy ko'p qatorli EDIT (hook unda boshqa ilovalardagidek ishlaydi)
        w->pgEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                                    0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_PGEDIT, g_hInst, NULL);
        SendMessageW(w->pgEdit, WM_SETFONT, (WPARAM)w->fPg, FALSE);
        SendMessageW(w->pgEdit, EM_LIMITTEXT, 4000, 0);
        SetWindowLongPtrW(w->pgEdit, GWLP_USERDATA, (LONG_PTR)w);
        w->pgOrig = (WNDPROC)SetWindowLongPtrW(w->pgEdit, GWLP_WNDPROC, (LONG_PTR)PgEditProc);
        w->pgEmpty = true;
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC mem = NULL;
        Ui_BeginBuffer(hwnd, &ps, &mem);
        if (mem && w) Paint(w, mem);
        Ui_EndBuffer(hwnd, &ps, mem);
        return 0;
    }

    case WM_TIMER:
        if (w && wp == UI_ANIM_TIMER) OnTimer(w);
        return 0;

    case WM_MOUSEMOVE:
        if (w) OnMouseMove(w, (short)LOWORD(lp), (short)HIWORD(lp));
        return 0;

    case WM_MOUSELEAVE:
        if (w) {
            w->tracking = false;
            SetHot(w, -1);
            if (w->exHot != -1) { w->exHot = -1; Redraw(w); }
        }
        return 0;

    case WM_SETCURSOR:
        if (w && LOWORD(lp) == HTCLIENT) {
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(hwnd, &p);
            int hi = HitTest(w, p.x, p.y);
            if (hi >= 0 && ChildWindowFromPoint(hwnd, p) == hwnd) {
                int k = w->it[hi].kind;
                SetCursor(LoadCursorW(NULL, (k == K_PGEDIT || k == K_EDIT) ? IDC_IBEAM : IDC_HAND));
                return TRUE;
            }
        }
        break;

    case WM_LBUTTONDOWN:
        if (w) {
            if (w->rec >= 0) StopRecord(w);
            SetFocus(hwnd);
            w->kbd = false;         // sichqoncha: fokus halqasi ko'rinmaydi
            int hi = HitTest(w, (short)LOWORD(lp), (short)HIWORD(lp));
            w->pressed = hi >= 0 ? w->it[hi].id : -1;
            if (hi >= 0 && IsFocusable(w, w->it[hi])) { w->focus = w->it[hi].id; w->kbd = false; }
            if (hi >= 0 && w->it[hi].id == ID_EDIT) SetFocus(w->edit);
            if (hi >= 0 && w->it[hi].id == ID_PGEDIT) SetFocus(w->pgEdit);
            SetCapture(hwnd);
            Redraw(w);
        }
        return 0;

    case WM_LBUTTONUP:
        if (w) {
            int x = (short)LOWORD(lp), y = (short)HIWORD(lp);
            int pressed = w->pressed;
            w->pressed = -1;
            if (GetCapture() == hwnd) ReleaseCapture();
            int hi = HitTest(w, x, y);
            if (pressed >= 0 && hi >= 0 && w->it[hi].id == pressed)
                Activate(w, pressed, x, y);
            else
                Redraw(w);
            if (w->done) return 0;
        }
        return 0;

    case WM_CAPTURECHANGED:
        if (w && w->pressed >= 0) { w->pressed = -1; Redraw(w); }
        return 0;

    case WM_MOUSEWHEEL:
        if (w && w->page == SP_EXCLUDE) {
            int d = GET_WHEEL_DELTA_WPARAM(wp);
            w->exScroll -= d * (S(w, LROW_H) + S(w, LROW_GAP)) / WHEEL_DELTA;
            ClampScroll(w);
            Redraw(w);
        }
        return 0;

    case WM_COMMAND:
        // native EDIT: fokus o'zgarganda ramka qayta chiziladi
        if (w && (HWND)lp == w->edit) {
            if (HIWORD(wp) == EN_SETFOCUS) { w->focus = ID_EDIT; Redraw(w); }
            else if (HIWORD(wp) == EN_KILLFOCUS) Redraw(w);
        }
        // sinov maydoni: o'zgarish, fokus (kartochka porlashi)
        if (w && (HWND)lp == w->pgEdit) {
            if (HIWORD(wp) == EN_CHANGE) OnPgChange(w);
            else if (HIWORD(wp) == EN_SETFOCUS) { w->focus = ID_PGEDIT; AnimTo(w, AN_PGFOC, 1.f, 180); Redraw(w); }
            else if (HIWORD(wp) == EN_KILLFOCUS) { AnimTo(w, AN_PGFOC, 0.f, 220); Redraw(w); }
        }
        return 0;

    case WM_CTLCOLOREDIT:
        if (w && ((HWND)lp == w->edit || (HWND)lp == w->pgEdit)) {
            const UiTheme& th = Ui_Theme();
            SetTextColor((HDC)wp, th.text);
            SetBkColor((HDC)wp, w->editBrCol);
            return (LRESULT)w->editBr;
        }
        break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        if (w) Redraw(w);
        return 0;

    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0;      // Alt menyu rejimiga kirmasin
        break;

    case WM_THEMECHANGED:
    case WM_SETTINGCHANGE:
        if (w && (msg == WM_THEMECHANGED ||
                  (lp && lstrcmpW((const wchar_t*)lp, L"ImmersiveColorSet") == 0))) {
            Ui_RefreshTheme();
            Ui_ApplyWindowStyle(hwnd);
            UpdateEditBrush(w);
            InvalidateRect(w->edit, NULL, TRUE);
            Redraw(w);
        }
        return 0;

    case WM_CLOSE:
        if (w) DoCancel(w);
        return 0;

    case WM_NCDESTROY:
        if (w) {
            KillTimer(hwnd, UI_ANIM_TIMER);
            FreeRes(w);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        s_hSettings = NULL;
        Ui_End();
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------
//  Ochish (modal)
// ---------------------------------------------------------------------

void ShowSettingsPage(HWND parent, int page)
{
    // Allaqachon ochiq: yangisi ochilmaydi, mavjudi oldinga chiqadi va sahifaga o'tadi
    if (s_hSettings && IsWindow(s_hSettings)) {
        Win* o = (Win*)GetWindowLongPtrW(s_hSettings, GWLP_USERDATA);
        if (IsIconic(s_hSettings)) ShowWindow(s_hSettings, SW_RESTORE);
        if (o) GoPage(o, page);
        SetForegroundWindow(s_hSettings);
        return;
    }

    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = SettingsWndProc;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        wc.hIcon         = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON));
        wc.lpszClassName = WND_CLASS;
        RegisterClassW(&wc);
        registered = true;
    }

    // Hotkey yozib olish uchun tizim hotkeylari vaqtincha bo'shatiladi (contracts 15)
    Hotkeys_Unregister();

    Win* w = new Win();
    w->origLang = g_settings.lang;
    w->apo      = g_settings.apo;
    w->startup  = Startup_IsEnabled();
    w->notify   = g_settings.notify;
    w->mode     = g_settings.mode;
    w->trig     = g_settings.triggerVk;
    w->rules    = g_settings.autoRules;
    w->hk[0] = g_settings.hkMode; w->hk[1] = g_settings.hkEnable; w->hk[2] = g_settings.hkConvert;
    ExLoad(w);
    w->exSel = -1; w->exHot = -1;
    w->hot = w->pressed = -1;
    w->rec = -1;
    if (page < 0 || page > SP_PLAYGROUND) page = SP_GENERAL;
    w->focus = ID_NAV0 + NavOf(page);

    // klient maydoni o'lchami: system DPI bo'yicha
    HDC sdc = GetDC(NULL);
    int dpi = sdc ? GetDeviceCaps(sdc, LOGPIXELSY) : 96;
    if (sdc) ReleaseDC(NULL, sdc);
    if (dpi <= 0) dpi = 96;

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = { 0, 0, Ui_S(BASE_W, dpi), Ui_S(BASE_H, dpi) };
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    int ww = rc.right - rc.left, wh = rc.bottom - rc.top;
    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int sx = wa.left + ((wa.right - wa.left) - ww) / 2;
    int sy = wa.top  + ((wa.bottom - wa.top) - wh) / 2;

    HWND hwnd = CreateWindowExW(0, WND_CLASS, T(S_SET_TITLE), style, sx, sy, ww, wh,
                                parent, NULL, g_hInst, w);
    if (!hwnd) {
        delete w;
        Hotkeys_Apply();
        return;
    }
    s_hSettings = hwnd;
    Ui_ApplyWindowStyle(hwnd);

    // boshlang'ich holat (animatsiyasiz)
    w->page = page;
    SetAnim(w->an[AN_NAVY], (float)NavTop(w, NavOf(w->page)));
    SetAnim(w->an[AN_SEG], (float)g_settings.lang);
    SetAnim(w->an[AN_PAGE], 1.f);
    SetAnim(w->an[AN_TOG + ID_CARD_TRIG], w->mode == MODE_TRIGGER ? 1.f : 0.f);
    SetAnim(w->an[AN_TOG + ID_CARD_AUTO], w->mode == MODE_AUTO ? 1.f : 0.f);
    SetAnim(w->an[AN_TOG + ID_STARTUP], w->startup ? 1.f : 0.f);
    SetAnim(w->an[AN_TOG + ID_NOTIFY], w->notify ? 1.f : 0.f);
    for (int i = 0; i < 4; ++i)
        SetAnim(w->an[AN_TOG + ID_RULE_O + i], (w->rules >> i) & 1 ? 1.f : 0.f);
    Layout(w);
    ShowPageEdits(w);

    // modal: ota oyna o'chiriladi (task 01 qayta kirish himoyasi shunga tayanadi)
    bool parentWasEnabled = parent && IsWindowEnabled(parent);
    if (parentWasEnabled) EnableWindow(parent, FALSE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    if (w->page == SP_PLAYGROUND) SetFocusId(w, ID_PGEDIT, false);
    else SetFocus(hwnd);

    MSG msg;
    bool quit = false;
    while (!w->done) {
        BOOL r = GetMessageW(&msg, NULL, 0, 0);
        if (r <= 0) { quit = (r == 0); if (quit) PostQuitMessage((int)msg.wParam); break; }
        if ((msg.hwnd == hwnd || msg.hwnd == w->edit || msg.hwnd == w->pgEdit) && OnKey(w, msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // eski ota oyna fokusini qaytarish uchun avval yoqiladi
    if (parentWasEnabled) EnableWindow(parent, TRUE);
    bool applied = w->hkApplied;
    if (IsWindow(hwnd)) DestroyWindow(hwnd);
    delete w;

    // Bekor / X / xato — saqlashda allaqachon qayta ro'yxatdan o'tgan bo'lmasa
    if (!applied) Hotkeys_Apply();
    if (parentWasEnabled) SetForegroundWindow(parent);
}

void ShowSettings(HWND parent)
{
    ShowSettingsPage(parent, SP_GENERAL);
}
