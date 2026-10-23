// Dizayn tizimi (ui_gfx.h). GDI+ flat API gdiplus.dll dan dinamik olinadi —
// Gdiplus headerlari va gdiplus.lib kerak emas, oyna yopiq paytda DLL
// xotirada turmaydi (Ui_Begin/Ui_End refcount).
#include "app.h"

// =====================================================================
//  GDI+ flat API (o'z typedef'larimiz)
// =====================================================================
typedef int   GpStatus;              // 0 = Ok
typedef float REAL;
typedef DWORD ARGB;
struct GpGraphics; struct GpBrush; struct GpPen; struct GpPath;

struct GpStartupInput {
    UINT32 GdiplusVersion;
    void*  DebugEventCallback;
    BOOL   SuppressBackgroundThread;
    BOOL   SuppressExternalCodecs;
};

typedef GpStatus (WINAPI *FnStartup)(ULONG_PTR*, const GpStartupInput*, void*);
typedef void     (WINAPI *FnShutdown)(ULONG_PTR);
typedef GpStatus (WINAPI *FnCreateFromHDC)(HDC, GpGraphics**);
typedef GpStatus (WINAPI *FnDeleteGraphics)(GpGraphics*);
typedef GpStatus (WINAPI *FnSetSmoothing)(GpGraphics*, int);
typedef GpStatus (WINAPI *FnSetPixelOffset)(GpGraphics*, int);
typedef GpStatus (WINAPI *FnCreateSolidFill)(ARGB, GpBrush**);
typedef GpStatus (WINAPI *FnDeleteBrush)(GpBrush*);
typedef GpStatus (WINAPI *FnCreatePen1)(ARGB, REAL, int, GpPen**);
typedef GpStatus (WINAPI *FnDeletePen)(GpPen*);
typedef GpStatus (WINAPI *FnCreatePath)(int, GpPath**);
typedef GpStatus (WINAPI *FnDeletePath)(GpPath*);
typedef GpStatus (WINAPI *FnAddPathArc)(GpPath*, REAL, REAL, REAL, REAL, REAL, REAL);
typedef GpStatus (WINAPI *FnAddPathLine)(GpPath*, REAL, REAL, REAL, REAL);
typedef GpStatus (WINAPI *FnClosePathFigure)(GpPath*);
typedef GpStatus (WINAPI *FnFillPath)(GpGraphics*, GpBrush*, GpPath*);
typedef GpStatus (WINAPI *FnDrawPath)(GpGraphics*, GpPen*, GpPath*);
typedef GpStatus (WINAPI *FnFillEllipse)(GpGraphics*, GpBrush*, REAL, REAL, REAL, REAL);

static const int kSmoothingAntiAlias   = 4;
static const int kPixelOffsetHighQual  = 2;
static const int kUnitPixel            = 2;
static const int kFillModeAlternate    = 0;

static struct {
    HMODULE           dll;
    ULONG_PTR         token;
    FnStartup         Startup;
    FnShutdown        Shutdown;
    FnCreateFromHDC   CreateFromHDC;
    FnDeleteGraphics  DeleteGraphics;
    FnSetSmoothing    SetSmoothing;
    FnSetPixelOffset  SetPixelOffset;
    FnCreateSolidFill CreateSolidFill;
    FnDeleteBrush     DeleteBrush;
    FnCreatePen1      CreatePen1;
    FnDeletePen       DeletePen;
    FnCreatePath      CreatePath;
    FnDeletePath      DeletePath;
    FnAddPathArc      AddPathArc;
    FnAddPathLine     AddPathLine;
    FnClosePathFigure ClosePathFigure;
    FnFillPath        FillPath;
    FnDrawPath        DrawPath;
    FnFillEllipse     FillEllipse;
} g_gp;

static int  g_uiRefs = 0;     // Ui_Begin chaqiruvlari soni
static bool g_gpOk   = false; // GDI+ yuklangan va ishga tushgan

template <class F> static bool GpLoad(F& fn, const char* name)
{
    fn = (F)(void*)GetProcAddress(g_gp.dll, name);
    return fn != NULL;
}

bool Ui_Begin()
{
    if (g_uiRefs++ > 0) return g_gpOk;
    ZeroMemory(&g_gp, sizeof(g_gp));
    g_gp.dll = LoadLibraryW(L"gdiplus.dll");
    if (!g_gp.dll) return false;
    bool ok = GpLoad(g_gp.Startup, "GdiplusStartup") &&
              GpLoad(g_gp.Shutdown, "GdiplusShutdown") &&
              GpLoad(g_gp.CreateFromHDC, "GdipCreateFromHDC") &&
              GpLoad(g_gp.DeleteGraphics, "GdipDeleteGraphics") &&
              GpLoad(g_gp.SetSmoothing, "GdipSetSmoothingMode") &&
              GpLoad(g_gp.SetPixelOffset, "GdipSetPixelOffsetMode") &&
              GpLoad(g_gp.CreateSolidFill, "GdipCreateSolidFill") &&
              GpLoad(g_gp.DeleteBrush, "GdipDeleteBrush") &&
              GpLoad(g_gp.CreatePen1, "GdipCreatePen1") &&
              GpLoad(g_gp.DeletePen, "GdipDeletePen") &&
              GpLoad(g_gp.CreatePath, "GdipCreatePath") &&
              GpLoad(g_gp.DeletePath, "GdipDeletePath") &&
              GpLoad(g_gp.AddPathArc, "GdipAddPathArc") &&
              GpLoad(g_gp.AddPathLine, "GdipAddPathLine") &&
              GpLoad(g_gp.ClosePathFigure, "GdipClosePathFigure") &&
              GpLoad(g_gp.FillPath, "GdipFillPath") &&
              GpLoad(g_gp.DrawPath, "GdipDrawPath") &&
              GpLoad(g_gp.FillEllipse, "GdipFillEllipse");
    if (ok) {
        GpStartupInput in = { 1, NULL, FALSE, FALSE };
        ok = g_gp.Startup(&g_gp.token, &in, NULL) == 0;
    }
    if (!ok) {
        FreeLibrary(g_gp.dll);
        ZeroMemory(&g_gp, sizeof(g_gp));
        return false;
    }
    g_gpOk = true;
    return true;
}

// Oxirgi oyna yopilganda GDI+ to'xtatiladi va DLL bo'shatiladi — fonda
// (faqat tray) GDI+ xotirada qolmaydi.
void Ui_End()
{
    if (g_uiRefs <= 0 || --g_uiRefs > 0) return;
    if (g_gpOk) g_gp.Shutdown(g_gp.token);
    if (g_gp.dll) FreeLibrary(g_gp.dll);
    ZeroMemory(&g_gp, sizeof(g_gp));
    g_gpOk = false;
}

static inline ARGB ToArgb(COLORREF c, BYTE a)
{
    return ((ARGB)a << 24) | ((ARGB)GetRValue(c) << 16) | ((ARGB)GetGValue(c) << 8) | GetBValue(c);
}

// HDC uchun AA sozlangan Graphics (NULL — GDI fallback)
static GpGraphics* GpBegin(HDC hdc)
{
    if (!g_gpOk) return NULL;
    GpGraphics* g = NULL;
    if (g_gp.CreateFromHDC(hdc, &g) != 0) return NULL;
    g_gp.SetSmoothing(g, kSmoothingAntiAlias);
    g_gp.SetPixelOffset(g, kPixelOffsetHighQual);
    return g;
}

// Yumaloq to'rtburchak yo'li (x, y, w, h — haqiqiy son, r — radius)
static GpPath* GpRoundPath(REAL x, REAL y, REAL w, REAL h, REAL r)
{
    GpPath* p = NULL;
    if (g_gp.CreatePath(kFillModeAlternate, &p) != 0) return NULL;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r <= 0.5f) {
        g_gp.AddPathLine(p, x, y, x + w, y);
        g_gp.AddPathLine(p, x + w, y, x + w, y + h);
        g_gp.AddPathLine(p, x + w, y + h, x, y + h);
    } else {
        REAL d = r * 2;
        g_gp.AddPathArc(p, x,         y,         d, d, 180, 90);
        g_gp.AddPathArc(p, x + w - d, y,         d, d, 270, 90);
        g_gp.AddPathArc(p, x + w - d, y + h - d, d, d,   0, 90);
        g_gp.AddPathArc(p, x,         y + h - d, d, d,  90, 90);
    }
    g_gp.ClosePathFigure(p);
    return p;
}

// =====================================================================
//  Mavzu
// =====================================================================
static UiTheme g_theme;
static bool    g_themeInit = false;

static bool ReadAppsUseLight()
{
    DWORD v = 1, sz = sizeof(v);
    if (RegGetValueW(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &v, &sz) != ERROR_SUCCESS)
        return true;   // kalit yo'q (eski tizim) — yorug'
    return v != 0;
}

void Ui_RefreshTheme()
{
    UiTheme& t = g_theme;
    HIGHCONTRASTW hc = { sizeof(hc) };
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) &&
        (hc.dwFlags & HCF_HIGHCONTRASTON)) {
        // Yuqori kontrast: foydalanuvchi tanlagan tizim ranglari (qulaylik birinchi)
        COLORREF win = GetSysColor(COLOR_WINDOW), txt = GetSysColor(COLOR_WINDOWTEXT);
        t.dark          = (GetRValue(win) + GetGValue(win) + GetBValue(win)) < 384;
        t.bg            = win;
        t.surface       = win;
        t.surfaceHover  = win;
        t.border        = txt;
        t.text          = txt;
        t.textDim       = txt;
        t.accent        = GetSysColor(COLOR_HIGHLIGHT);
        t.accentHover   = t.accent;
        t.accentPressed = t.accent;
        t.onAccent      = GetSysColor(COLOR_HIGHLIGHTTEXT);
        t.danger        = t.accent;
        t.shadow        = txt;
    } else if (!ReadAppsUseLight()) {
        // Qorong'i: fon biroz binafsha tusli (ikonka indigo'si bilan uyg'un).
        // Kontrast (WCAG): text/surface ~15:1, textDim/surface ~6:1,
        // accent/bg ~6:1, onAccent/accent ~6:1.
        t.dark          = true;
        t.bg            = RGB(0x1C, 0x1B, 0x22);
        t.surface       = RGB(0x26, 0x25, 0x2E);
        t.surfaceHover  = RGB(0x30, 0x2F, 0x3A);
        t.border        = RGB(0x3A, 0x39, 0x46);
        t.text          = RGB(0xF4, 0xF4, 0xF8);
        t.textDim       = RGB(0xA0, 0x9F, 0xB0);
        t.accent        = RGB(0x81, 0x8C, 0xF8);
        t.accentHover   = RGB(0x9A, 0xA3, 0xFA);
        t.accentPressed = RGB(0x6D, 0x78, 0xE6);
        t.onAccent      = RGB(0x14, 0x13, 0x1A);
        t.danger        = RGB(0xF8, 0x71, 0x71);
        t.shadow        = RGB(0x00, 0x00, 0x00);
    } else {
        // Yorug': text/bg ~17:1, textDim/bg ~5:1, onAccent/accent ~6.3:1.
        t.dark          = false;
        t.bg            = RGB(0xF7, 0xF7, 0xFB);
        t.surface       = RGB(0xFF, 0xFF, 0xFF);
        t.surfaceHover  = RGB(0xF0, 0xF0, 0xF7);
        t.border        = RGB(0xE2, 0xE1, 0xEC);
        t.text          = RGB(0x17, 0x16, 0x1D);
        t.textDim       = RGB(0x6B, 0x6A, 0x7A);
        t.accent        = RGB(0x4F, 0x46, 0xE5);
        t.accentHover   = RGB(0x43, 0x38, 0xCA);
        t.accentPressed = RGB(0x37, 0x30, 0xA3);
        t.onAccent      = RGB(0xFF, 0xFF, 0xFF);
        t.danger        = RGB(0xDC, 0x26, 0x26);
        t.shadow        = RGB(0x1E, 0x1B, 0x4B);   // indigo tusli soya — kulrangdan yumshoqroq
    }
    g_themeInit = true;
}

const UiTheme& Ui_Theme()
{
    if (!g_themeInit) Ui_RefreshTheme();
    return g_theme;
}

COLORREF Ui_Mix(COLORREF a, COLORREF b, float t)
{
    if (t <= 0) return a;
    if (t >= 1) return b;
    // natija doim 0..255 — musbat sonni +0.5 bilan yaxlitlash yetarli
    int r  = (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t + 0.5f);
    int g  = (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t + 0.5f);
    int bl = (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t + 0.5f);
    return RGB(r, g, bl);
}

// =====================================================================
//  DPI / shrift
// =====================================================================
int Ui_Dpi(HWND h)
{
    typedef UINT (WINAPI *FnGetDpiForWindow)(HWND);
    static FnGetDpiForWindow fn = (FnGetDpiForWindow)(void*)
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (fn && h) {
        UINT d = fn(h);
        if (d) return (int)d;
    }
    HDC dc = GetDC(NULL);
    int d = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) ReleaseDC(NULL, dc);
    return d ? d : 96;
}

int Ui_S(int px96, int dpi) { return MulDiv(px96, dpi, 96); }

// HDC ning DPI'si (xotira DC ham ekran DPI'sini beradi — dastur system-aware)
static int DcDpi(HDC hdc)
{
    int d = GetDeviceCaps(hdc, LOGPIXELSY);
    return d > 0 ? d : 96;
}

static int CALLBACK FontExistsProc(const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM lp)
{
    *(bool*)lp = true;
    return 0;
}

static bool FontExists(const wchar_t* face)
{
    LOGFONTW lf = {};
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpynW(lf.lfFaceName, face, LF_FACESIZE);
    bool found = false;
    HDC dc = GetDC(NULL);
    if (dc) {
        EnumFontFamiliesExW(dc, &lf, FontExistsProc, (LPARAM)&found, 0);
        ReleaseDC(NULL, dc);
    }
    return found;
}

// Shrift nomi bir marta aniqlanadi (faqat bool kesh — shrift obyekti keshlanmaydi)
static const wchar_t* TextFace()
{
    static int has = -1;
    if (has < 0) has = FontExists(L"Segoe UI Variable Text") ? 1 : 0;
    return has ? L"Segoe UI Variable Text" : L"Segoe UI";
}

static const wchar_t* IconFace()
{
    static int has = -1;
    if (has < 0) has = FontExists(L"Segoe Fluent Icons") ? 1 : 0;
    return has ? L"Segoe Fluent Icons" : L"Segoe MDL2 Assets";
}

HFONT Ui_Font(int dpi, int sizePx96, int weight)
{
    return CreateFontW(-Ui_S(sizePx96, dpi), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, TextFace());
}

HFONT Ui_IconFont(int dpi, int sizePx96)
{
    return CreateFontW(-Ui_S(sizePx96, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH, IconFace());
}

// =====================================================================
//  Oyna
// =====================================================================
void Ui_ApplyWindowStyle(HWND h)
{
    typedef HRESULT (WINAPI *FnDwmSet)(HWND, DWORD, LPCVOID, DWORD);
    HMODULE dll = GetModuleHandleW(L"dwmapi.dll");
    bool    own = false;
    if (!dll) { dll = LoadLibraryW(L"dwmapi.dll"); own = (dll != NULL); }
    if (!dll) return;
    FnDwmSet set = (FnDwmSet)(void*)GetProcAddress(dll, "DwmSetWindowAttribute");
    if (set) {
        const UiTheme& t = Ui_Theme();
        BOOL dark = t.dark ? TRUE : FALSE;
        if (FAILED(set(h, 20, &dark, sizeof(dark))))     // DWMWA_USE_IMMERSIVE_DARK_MODE
            set(h, 19, &dark, sizeof(dark));             // Win10 1809–1909 dagi eski raqam
        DWORD corner = 2;                                // DWMWCP_ROUND (Win11)
        set(h, 33, &corner, sizeof(corner));
        COLORREF cap = t.bg, brd = t.border;
        set(h, 35, &cap, sizeof(cap));                   // DWMWA_CAPTION_COLOR
        set(h, 34, &brd, sizeof(brd));                   // DWMWA_BORDER_COLOR
    }
    if (own) FreeLibrary(dll);
}

bool Ui_AnimationsEnabled()
{
    BOOL on = TRUE;
    if (!SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &on, 0)) return true;
    return on != FALSE;
}

// =====================================================================
//  Chizish
// =====================================================================

// GDI fallback'da alpha yo'q — rangni mavzu foniga aralashtiramiz
static COLORREF FallbackColor(COLORREF c, BYTE alpha)
{
    return alpha >= 255 ? c : Ui_Mix(Ui_Theme().bg, c, alpha / 255.0f);
}

void Ui_FillRound(HDC hdc, const RECT& rc, int radius, COLORREF c, BYTE alpha)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top || alpha == 0) return;
    if (GpGraphics* g = GpBegin(hdc)) {
        GpBrush* b = NULL;
        GpPath*  p = GpRoundPath((REAL)rc.left, (REAL)rc.top, (REAL)(rc.right - rc.left),
                                 (REAL)(rc.bottom - rc.top), (REAL)radius);
        if (p && g_gp.CreateSolidFill(ToArgb(c, alpha), &b) == 0) {
            g_gp.FillPath(g, b, p);
            g_gp.DeleteBrush(b);
        }
        if (p) g_gp.DeletePath(p);
        g_gp.DeleteGraphics(g);
        return;
    }
    HBRUSH br  = CreateSolidBrush(FallbackColor(c, alpha));
    HGDIOBJ ob = SelectObject(hdc, br);
    HGDIOBJ op = SelectObject(hdc, GetStockObject(NULL_PEN));
    // NULL_PEN bilan RoundRect o'ng/past chetni 1 px kam chizadi
    RoundRect(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1, radius * 2, radius * 2);
    SelectObject(hdc, op);
    SelectObject(hdc, ob);
    DeleteObject(br);
}

void Ui_StrokeRound(HDC hdc, const RECT& rc, int radius, COLORREF c, int widthPx)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top || widthPx <= 0) return;
    if (GpGraphics* g = GpBegin(hdc)) {
        // Chiziq rc ichida qolsin: yo'l yarim qalinlikka ichkariga suriladi
        REAL h = widthPx / 2.0f;
        GpPen*  pen = NULL;
        GpPath* p = GpRoundPath(rc.left + h, rc.top + h, (REAL)(rc.right - rc.left) - widthPx,
                                (REAL)(rc.bottom - rc.top) - widthPx, radius - h);
        if (p && g_gp.CreatePen1(ToArgb(c, 255), (REAL)widthPx, kUnitPixel, &pen) == 0) {
            g_gp.DrawPath(g, pen, p);
            g_gp.DeletePen(pen);
        }
        if (p) g_gp.DeletePath(p);
        g_gp.DeleteGraphics(g);
        return;
    }
    HPEN    pen = CreatePen(PS_INSIDEFRAME, widthPx, c);
    HGDIOBJ op  = SelectObject(hdc, pen);
    HGDIOBJ ob  = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius * 2, radius * 2);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(pen);
}

void Ui_FillCircle(HDC hdc, int cx, int cy, int r, COLORREF c, BYTE alpha)
{
    if (r <= 0 || alpha == 0) return;
    if (GpGraphics* g = GpBegin(hdc)) {
        GpBrush* b = NULL;
        if (g_gp.CreateSolidFill(ToArgb(c, alpha), &b) == 0) {
            g_gp.FillEllipse(g, b, (REAL)(cx - r), (REAL)(cy - r), (REAL)(2 * r), (REAL)(2 * r));
            g_gp.DeleteBrush(b);
        }
        g_gp.DeleteGraphics(g);
        return;
    }
    HBRUSH  br = CreateSolidBrush(FallbackColor(c, alpha));
    HGDIOBJ ob = SelectObject(hdc, br);
    HGDIOBJ op = SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, cx - r, cy - r, cx + r + 1, cy + r + 1);
    SelectObject(hdc, op);
    SelectObject(hdc, ob);
    DeleteObject(br);
}

// Yumshoq soya: spreadPx qalinlikdagi bir necha (≤ 8) qatlam, har biri past
// alpha'li yumaloq to'rtburchak, biroz pastga surilgan. Ichki qatlamlar
// ustma-ust tushib quyuqroq bo'ladi — oddiy gradient hissi. GDI'da chizilmaydi.
void Ui_Shadow(HDC hdc, const RECT& rc, int radius, int spreadPx, BYTE alpha)
{
    if (!g_gpOk || spreadPx <= 0 || alpha == 0) return;
    int layers = spreadPx < 8 ? spreadPx : 8;
    COLORREF c = Ui_Theme().shadow;
    BYTE a = (BYTE)((alpha + layers - 1) / layers);
    if (a == 0) a = 1;
    for (int i = layers; i >= 1; --i) {
        int grow = MulDiv(spreadPx, i, layers);
        RECT r = { rc.left - grow, rc.top - grow + grow / 2, rc.right + grow, rc.bottom + grow + grow / 2 };
        Ui_FillRound(hdc, r, radius + grow, c, a);
    }
}

void Ui_Text(HDC hdc, const wchar_t* s, RECT rc, HFONT f, COLORREF c, UINT dtFlags)
{
    if (!s) return;
    HGDIOBJ of = f ? SelectObject(hdc, f) : NULL;
    int     om = SetBkMode(hdc, TRANSPARENT);
    COLORREF oc = SetTextColor(hdc, c);
    DrawTextW(hdc, s, -1, &rc, dtFlags);
    SetTextColor(hdc, oc);
    SetBkMode(hdc, om);
    if (of) SelectObject(hdc, of);
}

void Ui_Glyph(HDC hdc, const wchar_t* glyph, RECT rc, HFONT iconFont, COLORREF c)
{
    Ui_Text(hdc, glyph, rc, iconFont, c, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

// =====================================================================
//  Bufer
// =====================================================================
HDC Ui_BeginBuffer(HWND h, PAINTSTRUCT* ps, HDC* outMem)
{
    HDC dc = BeginPaint(h, ps);
    RECT cr; GetClientRect(h, &cr);
    HDC mem = (cr.right > 0 && cr.bottom > 0) ? CreateCompatibleDC(dc) : NULL;
    HBITMAP bmp = mem ? CreateCompatibleBitmap(dc, cr.right, cr.bottom) : NULL;
    if (!bmp) {                                   // xotira yetmadi — to'g'ridan-to'g'ri chizamiz
        if (mem) DeleteDC(mem);
        *outMem = dc;
        return dc;
    }
    SelectObject(mem, bmp);
    HBRUSH bg = CreateSolidBrush(Ui_Theme().bg);
    FillRect(mem, &cr, bg);
    DeleteObject(bg);
    *outMem = mem;
    return mem;
}

void Ui_EndBuffer(HWND h, PAINTSTRUCT* ps, HDC mem)
{
    if (mem && mem != ps->hdc) {
        const RECT& r = ps->rcPaint;
        BitBlt(ps->hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, mem, r.left, r.top, SRCCOPY);
        HGDIOBJ bmp = GetCurrentObject(mem, OBJ_BITMAP);
        DeleteDC(mem);                            // DC o'chgach bitmap tanlovdan bo'shaydi
        DeleteObject(bmp);
    }
    EndPaint(h, ps);
}

// =====================================================================
//  Animatsiya
// =====================================================================
void Ui_AnimTo(UiAnim& a, float to, DWORD ms)
{
    if (a.to == to && (a.running || a.value == to)) return;   // allaqachon shu tomonga
    if (ms == 0 || !Ui_AnimationsEnabled()) {
        a.from = a.to = a.value = to;
        a.running = false;
        return;
    }
    a.from    = a.value;
    a.to      = to;
    a.start   = GetTickCount();
    a.dur     = ms;
    a.running = true;
}

bool Ui_AnimStep(UiAnim& a)
{
    if (!a.running) return false;
    DWORD el = GetTickCount() - a.start;
    if (el >= a.dur) {
        a.value   = a.to;
        a.running = false;
        return false;
    }
    float t = 1.0f - (float)el / (float)a.dur;
    float e = 1.0f - t * t * t;                   // ease-out-cubic
    a.value = a.from + (a.to - a.from) * e;
    return true;
}

// =====================================================================
//  Elementlar
// =====================================================================
static float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

void Ui_DrawFocusRing(HDC hdc, const RECT& rc, int radius)
{
    // Win11 uslubi: matn rangidagi 2 px halqa — har ikkala mavzuda yaqqol ko'rinadi
    Ui_StrokeRound(hdc, rc, radius, Ui_Theme().text, Ui_S(2, DcDpi(hdc)));
}

void Ui_DrawButton(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, UiBtnKind kind,
                   float hover, float press, bool enabled, bool focused)
{
    const UiTheme& t = Ui_Theme();
    int dpi = DcDpi(hdc);
    int rad = Ui_S(8, dpi);
    hover = Clamp01(hover); press = Clamp01(press);
    if (!enabled) hover = press = 0;

    COLORREF fill = t.surface, txt = t.text, brd = t.border;
    BYTE     fillA = 255;
    bool     border = false;
    switch (kind) {
    case UI_BTN_PRIMARY:
        fill = Ui_Mix(Ui_Mix(t.accent, t.accentHover, hover), t.accentPressed, press);
        txt  = t.onAccent;
        break;
    case UI_BTN_DANGER:
        fill = Ui_Mix(Ui_Mix(t.danger, t.text, 0.12f * hover), t.text, 0.2f * press);
        txt  = t.dark ? t.onAccent : RGB(0xFF, 0xFF, 0xFF);
        break;
    case UI_BTN_SECONDARY:
        fill   = Ui_Mix(Ui_Mix(t.surface, t.surfaceHover, hover), t.border, 0.6f * press);
        border = true;
        break;
    case UI_BTN_GHOST:
        // Tinch holatda fon yo'q — hover/press'da yumshoq paydo bo'ladi
        fill  = Ui_Mix(t.surfaceHover, t.border, press);
        fillA = (BYTE)(255 * Clamp01(hover + press));
        break;
    }
    if (!enabled) {
        fill = Ui_Mix(fill, t.bg, 0.55f);
        txt  = t.textDim;
        brd  = Ui_Mix(brd, t.bg, 0.5f);
    }
    if (fillA) Ui_FillRound(hdc, rc, rad, fill, fillA);
    if (border) Ui_StrokeRound(hdc, rc, rad, Ui_Mix(brd, t.textDim, 0.35f * hover), Ui_S(1, dpi) > 0 ? Ui_S(1, dpi) : 1);

    RECT tr = rc;
    if (press > 0) OffsetRect(&tr, 0, (int)(press * Ui_S(1, dpi) + 0.5f));   // bosilganda biroz pastga
    Ui_Text(hdc, text, tr, font, txt, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    if (focused) {
        RECT fr = rc;
        int  g = Ui_S(3, dpi);
        InflateRect(&fr, g, g);
        Ui_DrawFocusRing(hdc, fr, rad + g);
    }
}

void Ui_DrawToggle(HDC hdc, const RECT& rc, float pos, float hover, bool enabled, bool focused)
{
    const UiTheme& t = Ui_Theme();
    int dpi = DcDpi(hdc);
    pos = Clamp01(pos); hover = enabled ? Clamp01(hover) : 0;

    // Trek 40x20 (96 DPI), rc ichida markazda
    int tw = Ui_S(40, dpi), th = Ui_S(20, dpi);
    if (tw > rc.right - rc.left) tw = rc.right - rc.left;
    if (th > rc.bottom - rc.top) th = rc.bottom - rc.top;
    RECT tr;
    tr.left   = rc.left + ((rc.right - rc.left) - tw) / 2;
    tr.top    = rc.top + ((rc.bottom - rc.top) - th) / 2;
    tr.right  = tr.left + tw;
    tr.bottom = tr.top + th;
    int rad = th / 2;

    COLORREF offFill = Ui_Mix(t.surface, t.surfaceHover, hover);
    COLORREF onFill  = Ui_Mix(t.accent, t.accentHover, hover);
    COLORREF fill    = Ui_Mix(offFill, onFill, pos);
    COLORREF brd     = Ui_Mix(t.textDim, onFill, pos);
    COLORREF knob    = Ui_Mix(t.textDim, t.onAccent, pos);
    if (!enabled) {
        fill = Ui_Mix(fill, t.bg, 0.5f);
        brd  = Ui_Mix(brd, t.bg, 0.5f);
        knob = Ui_Mix(knob, t.bg, 0.5f);
    }
    Ui_FillRound(hdc, tr, rad, fill);
    if (pos < 1) Ui_StrokeRound(hdc, tr, rad, brd, Ui_S(1, dpi) > 0 ? Ui_S(1, dpi) : 1);

    // Knob: o'chiqda kichikroq (Win11), hover'da biroz kattalashadi
    float kr = (Ui_S(12, dpi) + (Ui_S(14, dpi) - Ui_S(12, dpi)) * pos) / 2.0f + hover * Ui_S(1, dpi) * 0.5f;
    int   x0 = tr.left + rad, x1 = tr.right - rad;
    int   cx = x0 + (int)((x1 - x0) * pos + 0.5f);
    Ui_FillCircle(hdc, cx, (tr.top + tr.bottom) / 2, (int)(kr + 0.5f), knob);

    if (focused) {
        RECT fr = tr;
        int  g = Ui_S(3, dpi);
        InflateRect(&fr, g, g);
        Ui_DrawFocusRing(hdc, fr, rad + g);
    }
}

void Ui_DrawCard(HDC hdc, const RECT& rc, bool selected, float hover, float select)
{
    const UiTheme& t = Ui_Theme();
    int dpi = DcDpi(hdc);
    int rad = Ui_S(8, dpi);
    hover = Clamp01(hover); select = Clamp01(select);

    COLORREF fill = Ui_Mix(t.surface, t.surfaceHover, hover);
    fill = Ui_Mix(fill, t.accent, (t.dark ? 0.10f : 0.06f) * select);   // yengil accent fon
    Ui_FillRound(hdc, rc, rad, fill);

    int w1 = Ui_S(1, dpi) > 0 ? Ui_S(1, dpi) : 1;
    int w2 = Ui_S(2, dpi) > 0 ? Ui_S(2, dpi) : 2;
    int bw = w1 + (int)((w2 - w1) * select + 0.5f);
    COLORREF brd = Ui_Mix(Ui_Mix(t.border, t.textDim, 0.4f * hover), t.accent, select);
    Ui_StrokeRound(hdc, rc, rad, brd, bw);

    if (selected) {
        // O'ng yuqori burchakda accent doira ichida ✓ (ikonka shriftisiz — chiziqlar)
        int r  = Ui_S(8, dpi);
        int cx = rc.right - Ui_S(12, dpi) - r / 2;
        int cy = rc.top + Ui_S(12, dpi) + r / 2;
        BYTE a = (BYTE)(255 * (select > 0 ? select : 1));
        Ui_FillCircle(hdc, cx, cy, r, t.accent, a);
        if (GpGraphics* g = GpBegin(hdc)) {
            GpPen*  pen = NULL;
            GpPath* p = NULL;
            if (g_gp.CreatePath(kFillModeAlternate, &p) == 0) {
                REAL s = (REAL)r;
                g_gp.AddPathLine(p, cx - s * 0.45f, cy + s * 0.02f, cx - s * 0.12f, cy + s * 0.35f);
                g_gp.AddPathLine(p, cx - s * 0.12f, cy + s * 0.35f, cx + s * 0.48f, cy - s * 0.32f);
                if (g_gp.CreatePen1(ToArgb(t.onAccent, a), (REAL)Ui_S(2, dpi), kUnitPixel, &pen) == 0) {
                    g_gp.DrawPath(g, pen, p);
                    g_gp.DeletePen(pen);
                }
                g_gp.DeletePath(p);
            }
            g_gp.DeleteGraphics(g);
        } else {
            HPEN    pen = CreatePen(PS_SOLID, Ui_S(2, dpi), t.onAccent);
            HGDIOBJ op  = SelectObject(hdc, pen);
            MoveToEx(hdc, cx - r * 45 / 100, cy, NULL);
            LineTo(hdc, cx - r * 12 / 100, cy + r * 35 / 100);
            LineTo(hdc, cx + r * 48 / 100, cy - r * 32 / 100);
            SelectObject(hdc, op);
            DeleteObject(pen);
        }
    }
}

void Ui_DrawKeycap(HDC hdc, const RECT& rc, const wchar_t* text, HFONT font, bool accent)
{
    const UiTheme& t = Ui_Theme();
    int dpi   = DcDpi(hdc);
    int rad   = Ui_S(6, dpi);
    int depth = Ui_S(2, dpi) > 0 ? Ui_S(2, dpi) : 2;   // pastki "qalinlik"

    COLORREF face  = accent ? t.accent : t.surface;
    COLORREF side  = accent ? t.accentPressed : Ui_Mix(t.border, t.textDim, t.dark ? 0.15f : 0.35f);
    COLORREF brd   = accent ? t.accentPressed : t.border;
    COLORREF txt   = accent ? t.onAccent : t.text;

    Ui_FillRound(hdc, rc, rad, side);                  // butun tugma — yon tomon rangi
    RECT fr = rc; fr.bottom -= depth;
    Ui_FillRound(hdc, fr, rad, face);                  // ustki yuz
    Ui_StrokeRound(hdc, rc, rad, brd, 1);
    Ui_Text(hdc, text, fr, font, txt, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void Ui_DrawInputFrame(HDC hdc, const RECT& rc, bool focused, float hover)
{
    const UiTheme& t = Ui_Theme();
    int dpi = DcDpi(hdc);
    int rad = Ui_S(6, dpi);
    hover = Clamp01(hover);

    COLORREF fill = focused ? t.surface : Ui_Mix(t.surface, t.surfaceHover, hover);
    Ui_FillRound(hdc, rc, rad, fill);
    COLORREF brd = focused ? Ui_Mix(t.border, t.accent, 0.5f) : Ui_Mix(t.border, t.textDim, 0.5f * hover);
    Ui_StrokeRound(hdc, rc, rad, brd, 1);
    if (focused) {
        // Win11 uslubi: pastda 2 px accent chiziq (yumaloq burchaklar ichida)
        int  w = Ui_S(2, dpi) > 0 ? Ui_S(2, dpi) : 2;
        RECT b = { rc.left + rad / 2, rc.bottom - w, rc.right - rad / 2, rc.bottom };
        Ui_FillRound(hdc, b, w / 2, t.accent);
    }
}
