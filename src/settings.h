#pragma once
#include <windows.h>

// Qo'llab-quvvatlanadigan tillar
enum Lang { LANG_UZ = 0, LANG_RU = 1, LANG_EN = 2, LANG_COUNT = 3 };

// Yozish rejimi: ikkalasidan biri ishlaydi
enum InputMode {
    MODE_TRIGGER = 0,   // trigger tugma + harf (AltGr+o -> ö)
    MODE_AUTO    = 1,   // eski yozuv avtomatik almashadi (o' g' sh ch -> ö ğ ş ç)
    MODE_COUNT   = 2
};

// Dastur sozlamalari (%APPDATA%\Harfbek\settings.ini da saqlanadi)
struct Settings {
    Lang lang;          // interfeys tili
    UINT triggerVk;     // trigger tugma (virtual-key kodi), masalan VK_MENU
    bool firstRun;      // birinchi ishga tushishmi (til/qo'llanma ko'rsatish uchun)
    bool runAtStartup;  // Windows bilan birga ishga tushsinmi
    bool enabled;       // almashtirish yoqilganmi
    wchar_t apo;        // o'girishda o'g' /o'g' uchun apostrof belgisi (', `, ʻ ...)
    InputMode mode;     // yozish rejimi (trigger yoki avto)
    // Tezkor tugmalar — HKM_GETHOTKEY formati: LOBYTE = vk, HIBYTE = HOTKEYF_*
    // (SHIFT 1, CONTROL 2, ALT 4, EXT 8); 0 = o'chiq
    WORD hkMode;        // rejimni almashtirish
    WORD hkEnable;      // yoqish/o'chirish
    WORD hkConvert;     // matnni o'girish menyusi
    bool notify;        // rejim/yoqish o'zgarganda ekranda bildirishnoma
    UINT autoRules;     // avto rejim qoidalari (AUTO_RULE_* bitlari)
};

// Avto rejim qoidalari (Settings::autoRules bitlari)
#define AUTO_RULE_O   1   // o' -> ö
#define AUTO_RULE_G   2   // g' -> ğ
#define AUTO_RULE_SH  4   // sh -> ş
#define AUTO_RULE_CH  8   // ch -> ç
#define AUTO_RULE_ALL 0xF

extern Settings g_settings;

void Settings_Load();   // sozlamalarni fayldan o'qiydi (bo'lmasa firstRun = true)
void Settings_Save();   // sozlamalarni faylga yozadi (istisnolar ro'yxati bilan)

// Istisno ilovalar (avto rejim bu dasturlarda ishlamaydi): ';' bilan ajratilgan
// exe nomlari ("code.exe;chrome.exe"), kichik harfda, maks 1023 belgi.
// Thread-xavfsiz — hook thread'i ham o'qiydi.
void Settings_GetExcludes(wchar_t* out, int n);
void Settings_SetExcludes(const wchar_t* list);
// Exe nomi (faqat fayl nomi, istalgan registrda) ro'yxatda bormi (hook thread'i uchun)
bool Settings_IsExcluded(const wchar_t* exeName);

// Windows bilan ishga tushish (HKCU\...\Run)
bool Startup_IsEnabled();
void Startup_Set(bool enable);
// Ishga tushganda chaqiriladi: g_settings.runAtStartup ni registrdagi haqiqiy
// holatga moslaydi; yoqilgan bo'lsa Run yo'lini joriy exe bilan yangilaydi.
void Startup_Sync();

// Ish stolida "Harfbek.lnk" yorlig'ini yaratadi (bo'lsa - tegmaydi)
void Shortcut_CreateDesktop();

// Dasturni o'chirish: barcha izlarni tozalaydi (sozlamalar papkasi,
// avtozapusk yozuvi, ish stolidagi yorliq). Exe faylning o'zi qoladi -
// qayta o'rnatish uchun shu faylni ishga tushirish kifoya.
void Uninstall_CleanData();
