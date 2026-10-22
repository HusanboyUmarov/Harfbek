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
#include "hotkeys.h"
#include "ui_gfx.h"

// Tray ikonka xabari
#define WM_TRAYICON  (WM_APP + 1)
// Modifikator "osilib qolgan"mi tekshirish (stuck-key himoyasi) — main thread'da
#define WM_APP_MODCHECK  (WM_APP + 3)
// Stuck-key tekshiruv taymerlari: ID = MODCHECK_TIMER_BASE + vk
#define MODCHECK_TIMER_BASE  0x500
// Tray ikonkani qayta qo'shishga urinish taymeri (tray hali tayyor bo'lmasa)
#define TRAY_RETRY_TIMER     0x400
// Ctrl+o'ng-tugma bosilganda o'girish menyusini ko'rsatish (wp=x, lp=y)
#define WM_APP_CONVERTMENU  (WM_APP + 4)

// Menyu buyruq ID lari
enum {
    CMD_TOGGLE_ENABLED = 100,
    CMD_SETTINGS,
    CMD_TUTORIAL,
    CMD_ABOUT,
    CMD_STARTUP,
    CMD_UNINSTALL,
    CMD_EXIT,
    CMD_AUTOMODE,               // avto rejim (MODE_TRIGGER <-> MODE_AUTO)
    CMD_PLAYGROUND,             // Sozlamalar -> sinov maydoni (flyout)
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
bool Trigger_IsModifier(UINT vk);   // Alt/Ctrl/Shift oilasimi (yutilmaydigan trigger)
bool Trigger_IsAllowed(UINT vk);    // sinalgan trigger ro'yxatida bormi (ui_settings.cpp)
void Hook_ResetTriggerState();      // trigger o'zgarganda kuzatuv holatini tozalaydi
// Oyna terminalmi (cmd/PowerShell, Windows Terminal, mintty...; ro'yxat hook.cpp da).
// Terminalda Ctrl+o'ng-tugma, o'girish hotkey'i va avto rejim ishlamaydi.
// Xabar yubormaydi — istalgan thread'dan chaqirish mumkin.
bool Hook_IsTerminalWindow(HWND w);

// Trigger tugmaning qisqa keycap nomi, masalan "AltGr" (ui_settings.cpp da aniqlanadi;
// qo'llanma va tray tooltip uchun)
void TriggerKeyCap(UINT vk, wchar_t* out, int n);

// Matnni o'girish (convert.cpp): belgilangan matnni nusxalab, o'girib, qaytaradi.
// toDigraph=true: ö ğ ş ç -> o' g' sh ch ;  false: teskari.
void TextConvert_Run(bool toDigraph);

// Interfeys oynalari
void ShowLanguageDialog(HWND parent);   // g_settings.lang ni yangilaydi
void ShowTutorial(HWND parent);
void ShowSettings(HWND parent);
void ShowAbout(HWND parent);
void ShowLicense(HWND parent);          // litsenziya / ogohlantirish

// Sozlamalar oynasi kerakli sahifada (ui_settings.cpp). Oyna allaqachon ochiq
// bo'lsa — mavjudi oldinga chiqadi va sahifaga o'tadi.
// SP_PLAYGROUND — sinov maydoni (task 28; enum oxirida, sidebar'da "Rejim" dan keyin)
enum SettingsPage { SP_GENERAL, SP_MODE, SP_HOTKEYS, SP_EXCLUDE, SP_ABOUT, SP_PLAYGROUND };
void ShowSettingsPage(HWND parent, int page);

// Rejim / yoqish almashuvi (main.cpp): saqlash, hook holati, tooltip, OSD,
// ochiq flyout. Tray menyu, tezkor tugma va flyout shu ikkisidan o'tadi.
void ToggleMode();
void ToggleEnabled();

// Tray flyout (ui_flyout.cpp) — tray chap tugma. anchor — ikonka markazi.
void Flyout_Toggle(POINT anchor);
void Flyout_Refresh();   // holat tashqaridan (hotkey) o'zgardi — ochiq bo'lsa yangilanadi

// Ekrandagi qisqa bildirishnoma (ui_osd.cpp): fokus olmaydi, ~1.5 s dan keyin
// yo'qoladi; qayta chaqiruv matnni yangilaydi. Faqat main thread'dan.
void Osd_Show(const wchar_t* text);
