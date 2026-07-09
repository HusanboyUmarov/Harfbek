#pragma once
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include "settings.h"
#include "i18n.h"
#include "resource.h"

// Tray ikonka xabari
#define WM_TRAYICON  (WM_APP + 1)
// Trigger tugma "bosib tanlash" tugagach hook Sozlamalar oynasiga yuboradi
#define WM_APP_CAPTURED  (WM_APP + 2)
// Modifikator "osilib qolgan"mi tekshirish (stuck-key himoyasi) — main thread'da
#define WM_APP_MODCHECK  (WM_APP + 3)
// Ctrl+o'ng-tugma bosilganda o'girish menyusini ko'rsatish (wp=x, lp=y)
#define WM_APP_CONVERTMENU  (WM_APP + 4)

// Menyu buyruq ID lari
enum {
    CMD_TOGGLE_ENABLED = 100,
    CMD_SETTINGS,
    CMD_TUTORIAL,
    CMD_ABOUT,
    CMD_STARTUP,
    CMD_EXIT,
    CMD_LANG_UZ = 200,
    CMD_LANG_RU,
    CMD_LANG_EN,
    // matnni o'girish menyusi (Ctrl+o'ng-tugma)
    CMD_CONV_TODIGRAPH = 300,   // ö ğ ş ç -> o' g' sh ch
    CMD_CONV_TOSPECIAL,         // o' g' sh ch -> ö ğ ş ç
};

// Umumiy global holat (main.cpp da aniqlanadi)
extern HINSTANCE g_hInst;
extern HWND      g_hWnd;

// Klaviatura hook (hook.cpp)
bool Hook_Install();
void Hook_Uninstall();
void Hook_ForceModUp(UINT vk);   // "osilib qolgan" modifikatorni majburan qo'yib yuboradi

// Trigger tugmani "bosib tanlash" rejimi (hook.cpp da aniqlanadi).
// Sozlamalar oynasi g_capturing ni yoqadi; hook birinchi bosilgan tugmani
// g_capturedVk ga yozib, g_captureNotifyWnd ga WM_APP_CAPTURED yuboradi.
extern volatile bool g_capturing;
extern volatile UINT g_capturedVk;
extern HWND          g_captureNotifyWnd;

// Trigger tugma nomlari (ui_settings.cpp da aniqlanadi)
const wchar_t* TriggerFullName(UINT vk);            // to'liq, joriy tilda: "O'ng Alt"
void           TriggerKeyCap(UINT vk, wchar_t* out, int n);  // qisqa keycap: "AltGr"

// Matnni o'girish (convert.cpp): belgilangan matnni nusxalab, o'girib, qaytaradi.
// toDigraph=true: ö ğ ş ç -> o' g' sh ch ;  false: teskari.
void TextConvert_Run(bool toDigraph);

// Interfeys oynalari
void ShowLanguageDialog(HWND parent);   // g_settings.lang ni yangilaydi
void ShowTutorial(HWND parent);
void ShowSettings(HWND parent);
void ShowAbout(HWND parent);
void ShowLicense(HWND parent);          // litsenziya / ogohlantirish
