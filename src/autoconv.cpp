// Avto rejim holat mashinasi: o' g' sh ch -> ö ğ ş ç (autoconv.h).
// Maxsus harflar (hex — encoding'ga bog'liq emas):
//   ö=00F6 Ö=00D6  ğ=011F Ğ=011E  ş=015F Ş=015E  ç=00E7 Ç=00C7
#include "autoconv.h"
#include "settings.h"   // AUTO_RULE_* (faqat makrolar)

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
// O'chiq qoida (rules da biti yo'q) bo'yicha juftlik almashmaydi.
static wchar_t Combine(wchar_t prev, wchar_t ch, unsigned rules)
{
    if (Auto_IsApostrophe(ch)) {
        switch (prev) {
        case L'o': return (rules & AUTO_RULE_O) ? (wchar_t)0x00F6 : 0;
        case L'O': return (rules & AUTO_RULE_O) ? (wchar_t)0x00D6 : 0;
        case L'g': return (rules & AUTO_RULE_G) ? (wchar_t)0x011F : 0;
        case L'G': return (rules & AUTO_RULE_G) ? (wchar_t)0x011E : 0;
        }
        return 0;
    }
    if (ch == L'h' || ch == L'H') {
        switch (prev) {
        case L's': return (rules & AUTO_RULE_SH) ? (wchar_t)0x015F : 0;
        case L'S': return (rules & AUTO_RULE_SH) ? (wchar_t)0x015E : 0;
        case L'c': return (rules & AUTO_RULE_CH) ? (wchar_t)0x00E7 : 0;
        case L'C': return (rules & AUTO_RULE_CH) ? (wchar_t)0x00C7 : 0;
        }
    }
    return 0;
}

AutoAction Auto_OnChar(AutoState& s, wchar_t ch, unsigned rules)
{
    AutoAction a = { AUTO_NONE, { 0, 0 }, 0 };
    wchar_t sp = ch ? Combine(s.prev, ch, rules) : 0;
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
