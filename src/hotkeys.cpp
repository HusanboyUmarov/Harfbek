// Tezkor tugmalar: RegisterHotKey orqali g_hWnd ga WM_HOTKEY keladi (main.cpp).
// Qiymatlar HKM_GETHOTKEY formatida (settings.h): LOBYTE = vk, HIBYTE = HOTKEYF_*.
#include "app.h"
#include <commctrl.h>   // HOTKEYF_* (faqat makrolar)

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000   // Win7+; eski MinGW headerlarida bo'lmasligi mumkin
#endif

static UINT g_hkRegistered = 0;   // hozir ro'yxatdan o'tgan HKID_* bitlari

static WORD HotkeyOf(int id)
{
    switch (id) {
    case HKID_MODE:    return g_settings.hkMode;
    case HKID_ENABLE:  return g_settings.hkEnable;
    case HKID_CONVERT: return g_settings.hkConvert;
    }
    return 0;
}

// HOTKEYF_* -> MOD_* (HOTKEYF_EXT RegisterHotKey'da yo'q — e'tiborsiz)
static UINT HotkeyMods(BYTE f)
{
    UINT m = 0;
    if (f & HOTKEYF_CONTROL) m |= MOD_CONTROL;
    if (f & HOTKEYF_SHIFT)   m |= MOD_SHIFT;
    if (f & HOTKEYF_ALT)     m |= MOD_ALT;
    return m;
}

void Hotkeys_Unregister()
{
    for (int id = HKID_MODE; id <= HKID_CONVERT; id <<= 1)
        if (g_hkRegistered & id) UnregisterHotKey(g_hWnd, id);
    g_hkRegistered = 0;
}

UINT Hotkeys_Apply()
{
    Hotkeys_Unregister();
    UINT failed = 0;
    for (int id = HKID_MODE; id <= HKID_CONVERT; id <<= 1) {
        WORD hk = HotkeyOf(id);
        UINT vk = LOBYTE(hk);
        if (!vk) continue;                       // o'chiq
        UINT mods = HotkeyMods(HIBYTE(hk));
        BOOL ok = g_hWnd && RegisterHotKey(g_hWnd, id, mods | MOD_NOREPEAT, vk);
        // MOD_NOREPEAT ni bilmaydigan tizim — bayroqsiz qayta urinamiz
        if (!ok && g_hWnd && GetLastError() == ERROR_INVALID_PARAMETER)
            ok = RegisterHotKey(g_hWnd, id, mods, vk);
        if (ok) g_hkRegistered |= id;
        else    failed |= id;
    }
    return failed;
}

// Tugma nomi: ko'p ishlatiladiganlari o'z jadvalimizdan (tizim tilidan qat'i
// nazar bir xil), qolganlari GetKeyNameTextW (OEM belgilar joriy layoutda).
static void KeyName(UINT vk, bool ext, wchar_t* out, int n)
{
    if (vk >= VK_F1 && vk <= VK_F24) { wsprintfW(out, L"F%u", vk - VK_F1 + 1); return; }
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        out[0] = (wchar_t)vk; out[1] = 0; return;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) { wsprintfW(out, L"Num %u", vk - VK_NUMPAD0); return; }

    const wchar_t* s = NULL;
    switch (vk) {
    case VK_SPACE:    s = L"Space";     break;
    case VK_RETURN:   s = L"Enter";     break;
    case VK_TAB:      s = L"Tab";       break;
    case VK_BACK:     s = L"Backspace"; break;
    case VK_ESCAPE:   s = L"Esc";       break;
    case VK_INSERT:   s = L"Insert";    break;
    case VK_DELETE:   s = L"Delete";    break;
    case VK_HOME:     s = L"Home";      break;
    case VK_END:      s = L"End";       break;
    case VK_PRIOR:    s = L"PgUp";      break;
    case VK_NEXT:     s = L"PgDn";      break;
    case VK_LEFT:     s = L"Left";      break;
    case VK_RIGHT:    s = L"Right";     break;
    case VK_UP:       s = L"Up";        break;
    case VK_DOWN:     s = L"Down";      break;
    case VK_PAUSE:    s = L"Pause";     break;
    case VK_SNAPSHOT: s = L"PrtSc";     break;
    case VK_SCROLL:   s = L"ScrollLock"; break;
    case VK_MULTIPLY: s = L"Num *";     break;
    case VK_ADD:      s = L"Num +";     break;
    case VK_SUBTRACT: s = L"Num -";     break;
    case VK_DECIMAL:  s = L"Num .";     break;
    case VK_DIVIDE:   s = L"Num /";     break;
    }
    if (s) { lstrcpynW(out, s, n); return; }

    UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    if (sc && GetKeyNameTextW((LONG)((sc << 16) | (ext ? (1u << 24) : 0)), out, n) > 0)
        return;
    wsprintfW(out, L"0x%02X", vk);
}

void Hotkey_Format(WORD hk, wchar_t* out, int n)
{
    if (!out || n <= 0) return;
    out[0] = 0;
    UINT vk = LOBYTE(hk);
    if (!vk) return;
    BYTE f = HIBYTE(hk);

    wchar_t key[64];
    KeyName(vk, (f & HOTKEYF_EXT) != 0, key, 64);
    wchar_t buf[128];
    wsprintfW(buf, L"%s%s%s%s",
              (f & HOTKEYF_CONTROL) ? L"Ctrl+"  : L"",
              (f & HOTKEYF_ALT)     ? L"Alt+"   : L"",
              (f & HOTKEYF_SHIFT)   ? L"Shift+" : L"",
              key);
    lstrcpynW(out, buf, n);
}
