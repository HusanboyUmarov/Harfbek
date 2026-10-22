#pragma once
// Avto rejim holat mashinasi (sof mantiq — Win32 ga bog'liq emas, alohida
// test qilinadi). Hook har yozilgan belgini beradi, mashina nima qilishni
// aytadi: hech narsa / oldingi belgini almashtirish / oxirgi almashtirishni
// bekor qilish (undo).
//
//   o' g' sh ch  ->  ö ğ ş ç   (katta-kichiklik birinchi harf bo'yicha:
//                              O'->Ö, Sh/SH->Ş, sh/sH->ş)
//   almashtirishdan keyin DARHOL Backspace -> asl ikki belgi qaytadi.

enum AutoActKind {
    AUTO_NONE = 0,   // tugma o'z yo'liga o'tadi
    AUTO_REPLACE,    // tugma yutiladi; 1 belgi o'chiriladi (Backspace), text yoziladi
    AUTO_UNDO,       // Backspace yutiladi; 1 belgi o'chiriladi, text (asl 2 belgi) yoziladi
};

struct AutoAction {
    AutoActKind kind;
    wchar_t     text[2];   // yoziladigan belgilar
    int         len;       // text dagi belgilar soni (REPLACE: 1, UNDO: 2)
};

struct AutoState {
    wchar_t prev;       // oxirgi yozilgan belgi (0 — noma'lum / almashtirib bo'lmaydi)
    wchar_t undo[2];    // oxirgi almashtirishning asl 2 belgisi
    bool    canUndo;    // oxirgi tugma almashtirish edimi (undo mumkinmi)
};

// Holatni tozalaydi (navigatsiya, sichqoncha, fokus almashishi, Ctrl/Alt/Win...)
void Auto_Reset(AutoState& s);

// Apostrof belgisimi: '  `  ʻ(02BB)  ’(2019)  ‘(2018)
bool Auto_IsApostrophe(wchar_t c);

// Belgili tugma yozildi. ch — yozilgan belgi (harf registri bilan, apostrof);
// boshqa yoki noma'lum belgi uchun 0.
AutoAction Auto_OnChar(AutoState& s, wchar_t ch);

// Backspace bosildi: almashtirishdan darhol keyin bo'lsa — undo, aks holda
// holat tozalanadi va Backspace o'z yo'liga o'tadi.
AutoAction Auto_OnBackspace(AutoState& s);
