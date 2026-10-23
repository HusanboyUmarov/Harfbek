#pragma once
#include <windows.h>

// Tezkor tugmalar (RegisterHotKey, g_hWnd ga WM_HOTKEY keladi).
// ID lar bir vaqtda bitmask ham — Hotkeys_Apply natijasi shu bitlardan iborat.
#define HKID_MODE     1   // rejimni almashtirish (g_settings.hkMode)
#define HKID_ENABLE   2   // yoqish/o'chirish (g_settings.hkEnable)
#define HKID_CONVERT  4   // matnni o'girish menyusi (g_settings.hkConvert)

// Barcha tezkor tugmalarni qayta ro'yxatdan o'tkazadi. Ro'yxatdan o'tmaganlar
// (band) bitmaskini qaytaradi; 0 = hammasi OK yoki o'chiq. Faqat main thread'dan.
UINT Hotkeys_Apply();
void Hotkeys_Unregister();
// "Ctrl+Shift+F9" kabi matn; hk == 0 bo'lsa bo'sh satr
void Hotkey_Format(WORD hk, wchar_t* out, int n);
