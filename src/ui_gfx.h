#pragma once
// Dizayn tizimi: mavzu ranglari, DPI/shrift, AA chizish (GDI+ dinamik),
// double-buffer, animatsiya va umumiy elementlar (tugma, toggle, kartochka...).
//
// GDI+ — gdiplus.dll dan LoadLibrary/GetProcAddress (header va .lib yo'q).
// Faqat UI oynalari ochiq paytda yuklanadi: har oyna WM_CREATE da Ui_Begin,
// WM_NCDESTROY da Ui_End (refcount). GDI+ topilmasa — hamma funksiya oddiy
// GDI bilan chizadi (AA'siz). Hamma funksiya faqat main (UI) thread'dan.
// Elementlar faqat chizadi; hit-test/hover/fokus mantiqi oynaning o'zida.
// Radius/qalinlik ichkarida HDC ning DPI'si bo'yicha masshtablanadi (RECT
// esa chaqiruvchi tomonidan allaqachon masshtablangan).
#include <windows.h>

// ---- Hayot sikli ----
bool Ui_Begin();   // true — GDI+ mavjud (AA); false bo'lsa ham chizish ishlaydi (GDI)
void Ui_End();

// ---- Mavzu ----
struct UiTheme {
    bool     dark;
    COLORREF bg, surface, surfaceHover, border, text, textDim,
             accent, accentHover, accentPressed, onAccent, danger, shadow;
};
const UiTheme& Ui_Theme();
void Ui_RefreshTheme();   // AppsUseLightTheme (va yuqori kontrast) ni qayta o'qiydi

// ---- DPI / shrift ----
int   Ui_Dpi(HWND);
int   Ui_S(int px96, int dpi);
HFONT Ui_Font(int dpi, int sizePx96, int weight);   // chaqiruvchi DeleteObject qiladi
HFONT Ui_IconFont(int dpi, int sizePx96);           // Segoe Fluent Icons / MDL2 Assets

// ---- Oyna ----
void Ui_ApplyWindowStyle(HWND);   // DWM: dark title bar, yumaloq burchak, caption/border rangi
bool Ui_AnimationsEnabled();      // SPI_GETCLIENTAREAANIMATION

// ---- Chizish (AA) ----
void     Ui_FillRound(HDC, const RECT&, int radius, COLORREF, BYTE alpha = 255);
void     Ui_StrokeRound(HDC, const RECT&, int radius, COLORREF, int widthPx);
void     Ui_FillCircle(HDC, int cx, int cy, int r, COLORREF, BYTE alpha = 255);
void     Ui_Shadow(HDC, const RECT&, int radius, int spreadPx, BYTE alpha);
void     Ui_Text(HDC, const wchar_t*, RECT, HFONT, COLORREF, UINT dtFlags);
void     Ui_Glyph(HDC, const wchar_t* glyph, RECT, HFONT iconFont, COLORREF);
COLORREF Ui_Mix(COLORREF a, COLORREF b, float t);   // t=0 -> a, t=1 -> b

// ---- Bufer (miltillamaslik uchun) ----
// BeginPaint + client o'lchamidagi xotira DC (mavzu foni bilan to'ldirilgan).
// Qaytariladigan HDC va *outMem — bir xil xotira DC: hamma chizish shunga.
// Ui_EndBuffer uni oynaga ko'chiradi, bo'shatadi va EndPaint qiladi.
HDC  Ui_BeginBuffer(HWND, PAINTSTRUCT*, HDC* outMem);
void Ui_EndBuffer(HWND, PAINTSTRUCT*, HDC mem);

// ---- Animatsiya ----
struct UiAnim { float from, to, value; DWORD start, dur; bool running; };
void Ui_AnimTo(UiAnim&, float to, DWORD ms);   // animatsiya o'chiq bo'lsa darhol value = to
bool Ui_AnimStep(UiAnim&);                     // ease-out-cubic; hali ketyaptimi
#define UI_ANIM_TIMER 0x7A11                   // SetTimer(hwnd, UI_ANIM_TIMER, 15, ...) faqat animatsiya paytida

// ---- Umumiy elementlar ----
enum UiBtnKind { UI_BTN_PRIMARY, UI_BTN_SECONDARY, UI_BTN_GHOST, UI_BTN_DANGER };
void Ui_DrawButton(HDC, const RECT&, const wchar_t* text, HFONT, UiBtnKind,
                   float hover, float press, bool enabled, bool focused);
void Ui_DrawToggle(HDC, const RECT&, float pos /*0..1*/, float hover, bool enabled, bool focused);
// Vizual tanlov holati (accent ramka/fon) — select (0..1) dan; selected — o'ng yuqori
// burchakdagi belgi (✓). Animatsiyasiz chaqiruvchi select = selected ? 1 : 0 beradi.
void Ui_DrawCard(HDC, const RECT&, bool selected, float hover, float select /*0..1*/);
void Ui_DrawKeycap(HDC, const RECT&, const wchar_t* text, HFONT, bool accent);
void Ui_DrawFocusRing(HDC, const RECT&, int radius);
void Ui_DrawInputFrame(HDC, const RECT&, bool focused, float hover);   // native EDIT atrofidagi ramka
