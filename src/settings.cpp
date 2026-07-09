#include "settings.h"
#include "config.h"   // TRIGGER_VK (standart trigger)

// Standart qiymatlar (apo = oddiy apostrof ')
Settings g_settings = { LANG_UZ, TRIGGER_VK, true, false, true, L'\'' };

static const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* kRunVal = L"Harfbek";

// %APPDATA%\Harfbek\settings.ini yo'lini tuzadi (papkani ham yaratadi)
static void GetIniPath(wchar_t* out, size_t n)
{
    wchar_t appdata[MAX_PATH] = {};
    if (!GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH))
        lstrcpyW(appdata, L".");
    wchar_t dir[MAX_PATH];
    wsprintfW(dir, L"%s\\Harfbek", appdata);
    CreateDirectoryW(dir, NULL);
    wsprintfW(out, L"%s\\settings.ini", dir);
    (void)n;
}

void Settings_Load()
{
    wchar_t ini[MAX_PATH];
    GetIniPath(ini, MAX_PATH);

    if (GetFileAttributesW(ini) == INVALID_FILE_ATTRIBUTES) {
        g_settings.firstRun = true;         // fayl yo'q - birinchi ishga tushish
        return;
    }
    g_settings.lang         = (Lang)GetPrivateProfileIntW(L"general", L"Language",     LANG_UZ,     ini);
    g_settings.triggerVk    =       GetPrivateProfileIntW(L"general", L"TriggerVK",    TRIGGER_VK,  ini);
    g_settings.firstRun     =       GetPrivateProfileIntW(L"general", L"FirstRun",     1,           ini) != 0;
    g_settings.runAtStartup =       GetPrivateProfileIntW(L"general", L"RunAtStartup", 0,           ini) != 0;
    g_settings.enabled      =       GetPrivateProfileIntW(L"general", L"Enabled",      1,           ini) != 0;
    g_settings.apo          = (wchar_t)GetPrivateProfileIntW(L"general", L"Apostrophe",  0x27,        ini);

    if (g_settings.lang < 0 || g_settings.lang >= LANG_COUNT) g_settings.lang = LANG_UZ;
    if (g_settings.triggerVk == 0) g_settings.triggerVk = TRIGGER_VK;
    if (g_settings.apo == 0) g_settings.apo = L'\'';
}

void Settings_Save()
{
    wchar_t ini[MAX_PATH];
    GetIniPath(ini, MAX_PATH);

    wchar_t buf[16];
    wsprintfW(buf, L"%d", (int)g_settings.lang);
    WritePrivateProfileStringW(L"general", L"Language", buf, ini);
    wsprintfW(buf, L"%u", g_settings.triggerVk);
    WritePrivateProfileStringW(L"general", L"TriggerVK", buf, ini);
    WritePrivateProfileStringW(L"general", L"FirstRun",     g_settings.firstRun     ? L"1" : L"0", ini);
    WritePrivateProfileStringW(L"general", L"RunAtStartup", g_settings.runAtStartup ? L"1" : L"0", ini);
    WritePrivateProfileStringW(L"general", L"Enabled",      g_settings.enabled      ? L"1" : L"0", ini);
    wsprintfW(buf, L"%u", (unsigned)g_settings.apo);
    WritePrivateProfileStringW(L"general", L"Apostrophe", buf, ini);
}

bool Startup_IsEnabled()
{
    HKEY k;
    bool ok = false;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &k) == ERROR_SUCCESS) {
        ok = (RegQueryValueExW(k, kRunVal, NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
        RegCloseKey(k);
    }
    return ok;
}

void Startup_Set(bool enable)
{
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t path[MAX_PATH];
            DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);
            // qo'shtirnoq ichida (yo'lda bo'sh joy bo'lsa)
            wchar_t quoted[MAX_PATH + 2];
            wsprintfW(quoted, L"\"%s\"", path);
            RegSetValueExW(k, kRunVal, 0, REG_SZ, (const BYTE*)quoted,
                           (lstrlenW(quoted) + 1) * sizeof(wchar_t));
            (void)len;
        } else {
            RegDeleteValueW(k, kRunVal);
        }
        RegCloseKey(k);
    }
}
