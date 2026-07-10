#pragma once
#include <windows.h>

// Qo'llab-quvvatlanadigan tillar
enum Lang { LANG_UZ = 0, LANG_RU = 1, LANG_EN = 2, LANG_COUNT = 3 };

// Dastur sozlamalari (%APPDATA%\Harfbek\settings.ini da saqlanadi)
struct Settings {
    Lang lang;          // interfeys tili
    UINT triggerVk;     // trigger tugma (virtual-key kodi), masalan VK_MENU
    bool firstRun;      // birinchi ishga tushishmi (til/qo'llanma ko'rsatish uchun)
    bool runAtStartup;  // Windows bilan birga ishga tushsinmi
    bool enabled;       // almashtirish yoqilganmi
    wchar_t apo;        // o'girishda o'g' /o'g' uchun apostrof belgisi (', `, ʻ ...)
};

extern Settings g_settings;

void Settings_Load();   // sozlamalarni fayldan o'qiydi (bo'lmasa firstRun = true)
void Settings_Save();   // sozlamalarni faylga yozadi

// Windows bilan ishga tushish (HKCU\...\Run)
bool Startup_IsEnabled();
void Startup_Set(bool enable);

// Ish stolida "Harfbek.lnk" yorlig'ini yaratadi (bo'lsa - tegmaydi)
void Shortcut_CreateDesktop();
