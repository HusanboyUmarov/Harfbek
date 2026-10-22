// Avto rejim holat mashinasi: o' g' sh ch -> ö ğ ş ç (autoconv.h).
// Maxsus harflar (hex — encoding'ga bog'liq emas):
//   ö=00F6 Ö=00D6  ğ=011F Ğ=011E  ş=015F Ş=015E  ç=00E7 Ç=00C7
#include "autoconv.h"

void Auto_Reset(AutoState& s)
{
    s.prev    = 0;
    s.undo[0] = s.undo[1] = 0;
    s.canUndo = false;
}

bool Auto_IsApostrophe(wchar_t c)
{
    return c == L'\'' || c == L'`' ||
           c == (wchar_t)0x02BB || c == (wchar_t)0x2019 || c == (wchar_t)0x2018;
}

// prev + ch juftligi maxsus harf beradimi. Registr — birinchi harfniki.
static wchar_t Combine(wchar_t prev, wchar_t ch)
{
    if (Auto_IsApostrophe(ch)) {
        switch (prev) {
        case L'o': return (wchar_t)0x00F6;
        case L'O': return (wchar_t)0x00D6;
        case L'g': return (wchar_t)0x011F;
        case L'G': return (wchar_t)0x011E;
        }
        return 0;
    }
    if (ch == L'h' || ch == L'H') {
        switch (prev) {
        case L's': return (wchar_t)0x015F;
        case L'S': return (wchar_t)0x015E;
        case L'c': return (wchar_t)0x00E7;
        case L'C': return (wchar_t)0x00C7;
        }
    }
    return 0;
}

AutoAction Auto_OnChar(AutoState& s, wchar_t ch)
{
    AutoAction a = { AUTO_NONE, { 0, 0 }, 0 };
    wchar_t sp = ch ? Combine(s.prev, ch) : 0;
    if (!sp) {
        s.prev    = ch;      // 0 bo'lsa — keyingi belgi bilan juftlashmaydi
        s.canUndo = false;
        return a;
    }
    a.kind    = AUTO_REPLACE;
    a.text[0] = sp;
    a.len     = 1;
    s.undo[0] = s.prev;
    s.undo[1] = ch;
    s.canUndo = true;
    s.prev    = sp;          // maxsus harf hech narsa bilan juftlashmaydi (shh -> şh, o'' -> ö')
    return a;
}

AutoAction Auto_OnBackspace(AutoState& s)
{
    AutoAction a = { AUTO_NONE, { 0, 0 }, 0 };
    if (s.canUndo) {
        a.kind    = AUTO_UNDO;
        a.text[0] = s.undo[0];
        a.text[1] = s.undo[1];
        a.len     = 2;
    }
    // Undo'dan keyin ham tozalaymiz: tiklangan "sh" qayta almashmasin
    // (yana "h" -> "shh"), ikkinchi Backspace esa oddiy o'chiradi.
    Auto_Reset(s);
    return a;
}
