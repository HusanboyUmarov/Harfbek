#include "app.h"      // Trigger_IsAllowed
#include "config.h"   // TRIGGER_VK (standart trigger)
#include <shlobj.h>   // SHGetFolderPathW, IShellLinkW (yorliq yaratish)

// Standart qiymatlar (apo = rasmiy tutuq belgisi ʻ U+02BB)
Settings g_settings = { LANG_UZ, TRIGGER_VK, true, false, true, (wchar_t)0x02BB, MODE_TRIGGER };

static const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* kRunVal = L"Harfbek";

// a + b ni out (n belgi, oxirgi 0 bilan) ga yig'adi. Sig'masa — out bo'sh, false.
static bool JoinPath(wchar_t* out, size_t n, const wchar_t* a, const wchar_t* b)
{
    size_t la = (size_t)lstrlenW(a), lb = (size_t)lstrlenW(b);
    if (la + lb + 1 > n) { if (n) out[0] = 0; return false; }
    lstrcpyW(out, a);
    lstrcatW(out, b);
    return true;
}

// %APPDATA%\Harfbek\settings.ini yo'lini tuzadi (papkani ham yaratadi).
// APPDATA yo'q yoki juda uzun bo'lsa (to'liq yo'l n ga sig'masa) — joriy papka.
static void GetIniPath(wchar_t* out, size_t n)
{
    wchar_t appdata[MAX_PATH] = {};
    DWORD   len = GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);
    wchar_t dir[MAX_PATH];
    if (len == 0 || len >= MAX_PATH ||
        !JoinPath(dir, MAX_PATH, appdata, L"\\Harfbek") ||
        !JoinPath(out, n, dir, L"\\settings.ini"))
    {
        lstrcpyW(dir, L".\\Harfbek");
        JoinPath(out, n, dir, L"\\settings.ini");
    }
    CreateDirectoryW(dir, NULL);
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
    g_settings.apo          = (wchar_t)GetPrivateProfileIntW(L"general", L"Apostrophe",  0x2BB,       ini);
    UINT mode               =       GetPrivateProfileIntW(L"general", L"Mode",         MODE_TRIGGER, ini);

    if (g_settings.lang < 0 || g_settings.lang >= LANG_COUNT) g_settings.lang = LANG_UZ;
    // Noma'lum/buzilgan rejim qiymati — standart (trigger) rejim
    g_settings.mode = (mode < MODE_COUNT) ? (InputMode)mode : MODE_TRIGGER;
    // Faqat sinalgan trigger tugmalari qabul qilinadi. Eski versiyada saqlangan
    // istalgan tugma yoki buzilgan/chala yozilgan ini (masalan, tok o'chib
    // qolganda) standartga qaytadi.
    if (!Trigger_IsAllowed(g_settings.triggerVk))
        g_settings.triggerVk = TRIGGER_VK;
    if (g_settings.apo == 0) g_settings.apo = (wchar_t)0x02BB;
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
    wsprintfW(buf, L"%d", (int)g_settings.mode);
    WritePrivateProfileStringW(L"general", L"Mode", buf, ini);
}

// Ish stolida "Harfbek.lnk" yorlig'ini yaratadi. Yorliq allaqachon bo'lsa,
// foydalanuvchi uni o'zgartirgan/ko'chirgan bo'lishi mumkin - tegmaymiz.
void Shortcut_CreateDesktop()
{
    wchar_t desktop[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktop)))
        return;

    wchar_t lnk[MAX_PATH];
    if (!JoinPath(lnk, MAX_PATH, desktop, L"\\Harfbek.lnk"))
        return;
    if (GetFileAttributesW(lnk) != INVALID_FILE_ATTRIBUTES)
        return;

    wchar_t exe[MAX_PATH];
    DWORD   len = GetModuleFileNameW(NULL, exe, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)   // xato yoki yo'l kesilgan
        return;

    // Ish papkasi - exe turgan papka
    wchar_t dir[MAX_PATH];
    lstrcpyW(dir, exe);
    for (int i = lstrlenW(dir) - 1; i >= 0; i--) {
        if (dir[i] == L'\\') { dir[i] = 0; break; }
    }

    bool comInit = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
    IShellLinkW* sl = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW, (void**)&sl))) {
        sl->SetPath(exe);
        sl->SetWorkingDirectory(dir);
        sl->SetIconLocation(exe, 0);

        IPersistFile* pf = NULL;
        if (SUCCEEDED(sl->QueryInterface(IID_IPersistFile, (void**)&pf))) {
            pf->Save(lnk, TRUE);
            pf->Release();
        }
        sl->Release();
    }
    if (comInit) CoUninitialize();
}

// Dasturning barcha izlarini tozalaydi: sozlamalar papkasi, avtozapusk
// yozuvi va ish stolidagi yorliq. Exe faylning o'zi qoladi - qayta
// o'rnatish kerak bo'lsa, shu fayl ishga tushiriladi (birinchi ishga
// tushirish kabi: til tanlash, qo'llanma, yorliq).
void Uninstall_CleanData()
{
    // Avtozapusk yozuvi (HKCU\...\Run)
    Startup_Set(false);

    // %APPDATA%\Harfbek papkasi (settings.ini bilan birga)
    wchar_t ini[MAX_PATH];
    GetIniPath(ini, MAX_PATH);
    DeleteFileW(ini);
    wchar_t dir[MAX_PATH];
    lstrcpyW(dir, ini);
    for (int i = lstrlenW(dir) - 1; i >= 0; i--) {
        if (dir[i] == L'\\') { dir[i] = 0; break; }
    }
    RemoveDirectoryW(dir);   // faqat bo'sh bo'lsa o'chadi - boshqa fayl bo'lsa tegmaydi

    // Ish stolidagi yorliq
    wchar_t desktop[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktop))) {
        wchar_t lnk[MAX_PATH];
        if (JoinPath(lnk, MAX_PATH, desktop, L"\\Harfbek.lnk"))
            DeleteFileW(lnk);
    }
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

// Joriy exe yo'li qo'shtirnoq ichida (yo'lda bo'sh joy bo'lsa) — Run qiymati
// shu ko'rinishda yoziladi. out kamida MAX_PATH + 2 belgi.
static bool GetQuotedExePath(wchar_t* out)
{
    wchar_t path[MAX_PATH];
    DWORD   len = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)   // xato yoki yo'l kesilgan — yozmaymiz
        return false;
    wsprintfW(out, L"\"%s\"", path);
    return true;
}

void Startup_Set(bool enable)
{
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t quoted[MAX_PATH + 2];
            if (GetQuotedExePath(quoted))
                RegSetValueExW(k, kRunVal, 0, REG_SZ, (const BYTE*)quoted,
                               (lstrlenW(quoted) + 1) * sizeof(wchar_t));
        } else {
            RegDeleteValueW(k, kRunVal);
        }
        RegCloseKey(k);
    }
}

// Ishga tushganda: registrdagi haqiqiy holat asosiy manba. Yoqilgan bo'lsa,
// lekin Run qiymati boshqa yo'lni ko'rsatsa (exe ko'chirilgan), joriy yo'l
// bilan qayta yoziladi. g_settings.runAtStartup shu holatga moslanadi.
void Startup_Sync()
{
    bool on = Startup_IsEnabled();
    if (on) {
        wchar_t cur[MAX_PATH + 2] = {};
        DWORD   bytes = sizeof(cur) - sizeof(wchar_t);   // oxirgi 0 uchun joy qoladi
        DWORD   type = 0;
        HKEY    k;
        bool    same = false;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &k) == ERROR_SUCCESS) {
            wchar_t want[MAX_PATH + 2];
            if (RegQueryValueExW(k, kRunVal, NULL, &type, (BYTE*)cur, &bytes) == ERROR_SUCCESS &&
                type == REG_SZ && GetQuotedExePath(want))
                same = (lstrcmpiW(cur, want) == 0);
            RegCloseKey(k);
        }
        if (!same) Startup_Set(true);
    }
    if (g_settings.runAtStartup != on) {
        g_settings.runAtStartup = on;
        if (!g_settings.firstRun) Settings_Save();   // birinchi ishga tushishda keyin saqlanadi
    }
}
