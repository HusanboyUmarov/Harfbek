#pragma once
#include <windows.h>

// =====================================================================
//  SOZLAMALAR / SETTINGS
//  Bu yerdan harflar jadvalini istagancha o'zgartirishingiz mumkin.
// =====================================================================
//
//  Ishlashi: TRIGGER (standart: AltGr = o'ng Alt) tugmasi bosib turilganda,
//  quyidagi klavishalar maxsus harflarga aylanadi.
//    AltGr + o            -> o-umlaut       AltGr + Shift + o  -> O-umlaut
//    AltGr + g            -> g-breve        AltGr + Shift + g  -> G-breve
//    AltGr + s            -> s-cedilla      AltGr + Shift + s  -> S-cedilla
//    AltGr + c            -> c-cedilla      AltGr + Shift + c  -> C-cedilla
//
//  Nega o'ng Alt: chap Alt + Shift — Windows'da klaviatura tilini almashtirish
//  yorlig'i. O'ng Alt (AltGr) unga tegmaydi, shuning uchun katta harf yozganda
//  (AltGr + Shift + harf) til almashib ketmaydi. Foydalanuvchi trigger tugmani
//  Sozlamalar oynasidan istalgan boshqa tugmaga o'zgartira oladi.
//
//  Belgilarni almashtirish uchun pastdagi Unicode kodlarini o'zgartiring.
//  (Escape shaklida yozilgan — kompilyator encoding'iga bog'liq emas.)
// ---------------------------------------------------------------------

struct KeyMap {
    WORD    vk;     // klavisha (virtual-key kodi). Harflar uchun katta lotin: 'O','G','S','C'
    wchar_t lower;  // AltGr + klavisha   -> kichik harf
    wchar_t upper;  // AltGr + Shift/Caps -> katta harf
};

// --- Hozirgi tanlov: turkcha uslub (o-umlaut, g-breve, s/c-cedilla) ---
// Belgilar hex Unicode kodlari bilan (encoding'ga bog'liq emas):
//   0x00F6 = o-umlaut (o..)   0x00D6 = O-umlaut
//   0x011F = g-breve          0x011E = G-breve
//   0x015F = s-cedilla        0x015E = S-cedilla
//   0x00E7 = c-cedilla        0x00C7 = C-cedilla
static const KeyMap kMaps[] = {
    { 'O', (wchar_t)0x00F6, (wchar_t)0x00D6 },  // o.. / O..   (O' -> ..)
    { 'G', (wchar_t)0x011F, (wchar_t)0x011E },  // g-breve     (G' -> ..)
    { 'S', (wchar_t)0x015F, (wchar_t)0x015E },  // s-cedilla   (Sh -> ..)
    { 'C', (wchar_t)0x00E7, (wchar_t)0x00C7 },  // c-cedilla   (Ch -> ..)
};

// --- Muqobil: 2021 rasmiy loyiha uslubi (macron) bo'lsa, shularni ishlating ---
//   { 'O', (wchar_t)0x014D, (wchar_t)0x014C },  // o-macron / O-macron
//   { 'G', (wchar_t)0x1E21, (wchar_t)0x1E20 },  // g-macron / G-macron
//   { 'S', (wchar_t)0x015F, (wchar_t)0x015E },  // s-cedilla
//   { 'C', (wchar_t)0x00E7, (wchar_t)0x00C7 },  // c-cedilla

static const int kMapCount = sizeof(kMaps) / sizeof(kMaps[0]);

// TRIGGER tugmasi (standart). Foydalanuvchi buni Sozlamalardan o'zgartira oladi.
//   - Faqat o'ng Alt (AltGr):  VK_RMENU   <- standart (til almashtirish bilan urishmaydi)
//   - Faqat chap Alt:          VK_LMENU
//   - Ikkala Alt ham:          VK_MENU
#define TRIGGER_VK  VK_RMENU
