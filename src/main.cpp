// =====================================================================
//  Harfbek  -  o'zbek maxsus harflarini oson yozish (Windows)
//
//  Trigger (standart: AltGr = o'ng Alt) + harf:
//     AltGr+o -> ö   AltGr+g -> ğ   AltGr+s -> ş   AltGr+c -> ç   (Shift/Caps -> katta)
//
//  Interfeys: uz / ru / eng. Tray menyu orqali boshqariladi.
//  Xavfsizlik: hech narsa yozib olinmaydi/saqlanmaydi, internetga chiqmaydi.
// =====================================================================
#include "app.h"
#include <shellapi.h>

HINSTANCE g_hInst = NULL;
HWND      g_hWnd  = NULL;

static NOTIFYICONDATAW g_nid = {};
static HICON           g_iconBig   = NULL;
static HICON           g_iconSmall = NULL;

// Tray yozuvini (tooltip) joriy tilda yangilaydi
static void UpdateTrayTip()
{
    lstrcpynW(g_nid.szTip, T(S_TOOLTIP), 128);
    g_nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static void SetLanguage(Lang l)
{
    g_settings.lang = l;
    Settings_Save();
    UpdateTrayTip();
}

// Menyu buyrug'ini bajaradi
static void HandleCommand(HWND hWnd, int cmd)
{
    switch (cmd)
    {
    case CMD_TOGGLE_ENABLED:
        g_settings.enabled = !g_settings.enabled;
        Settings_Save();
        break;
    case CMD_SETTINGS:
        ShowSettings(hWnd);
        UpdateTrayTip();
        break;
    case CMD_TUTORIAL:
        ShowTutorial(hWnd);
        break;
    case CMD_ABOUT:
        ShowAbout(hWnd);
        break;
    case CMD_STARTUP:
        g_settings.runAtStartup = !g_settings.runAtStartup;
        Startup_Set(g_settings.runAtStartup);
        Settings_Save();
        break;
    case CMD_LANG_UZ: SetLanguage(LANG_UZ); break;
    case CMD_LANG_RU: SetLanguage(LANG_RU); break;
    case CMD_LANG_EN: SetLanguage(LANG_EN); break;
    case CMD_EXIT:
        PostMessageW(hWnd, WM_CLOSE, 0, 0);
        break;
    }
}

// Tray menyusini ochadi
static void ShowTrayMenu(HWND hWnd)
{
    POINT pt; GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | (g_settings.enabled ? MF_CHECKED : 0),
                CMD_TOGGLE_ENABLED, T(S_MENU_ENABLED));
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, CMD_SETTINGS, T(S_MENU_SETTINGS));

    HMENU langMenu = CreatePopupMenu();
    AppendMenuW(langMenu, MF_STRING | (g_settings.lang == LANG_UZ ? MF_CHECKED : 0), CMD_LANG_UZ, L"Özbekça");
    AppendMenuW(langMenu, MF_STRING | (g_settings.lang == LANG_RU ? MF_CHECKED : 0), CMD_LANG_RU, L"Русский");
    AppendMenuW(langMenu, MF_STRING | (g_settings.lang == LANG_EN ? MF_CHECKED : 0), CMD_LANG_EN, L"English");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)langMenu, T(S_MENU_LANGUAGE));

    AppendMenuW(menu, MF_STRING, CMD_TUTORIAL, T(S_MENU_TUTORIAL));
    AppendMenuW(menu, MF_STRING, CMD_ABOUT, T(S_MENU_ABOUT));
    AppendMenuW(menu, MF_STRING | (g_settings.runAtStartup ? MF_CHECKED : 0),
                CMD_STARTUP, T(S_MENU_STARTUP));
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, CMD_EXIT, T(S_MENU_EXIT));

    SetForegroundWindow(hWnd);
    int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(menu);
    PostMessageW(hWnd, WM_NULL, 0, 0);   // menyu to'g'ri yopilishi uchun
    if (cmd) HandleCommand(hWnd, cmd);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_TRAYICON:
        if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_LBUTTONUP)
            ShowTrayMenu(hWnd);
        return 0;

    case WM_APP_MODCHECK: {
        // Real modifikator qo'yib yuborilgach, u hali ham "bosilgan" ko'rinsa —
        // bu bizning qayta bosishimizdan osilib qolgan; majburan qo'yib yuboramiz.
        UINT vk = (UINT)wp;
        if (GetAsyncKeyState((int)vk) & 0x8000)
            Hook_ForceModUp(vk);
        return 0;
    }

    case WM_APP_CONVERTMENU: {
        // Ctrl+o'ng-tugma bosildi: belgilangan matnni o'girish menyusini ko'rsatamiz.
        int x = (int)(LONG)wp, y = (int)(LONG)lp;
        HWND target = GetForegroundWindow();     // matn shu oynada belgilangan
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, CMD_CONV_TODIGRAPH, T(S_CONV_TODIGRAPH));
        AppendMenuW(menu, MF_STRING, CMD_CONV_TOSPECIAL, T(S_CONV_TOSPECIAL));
        SetForegroundWindow(hWnd);
        int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, x, y, 0, hWnd, NULL);
        DestroyMenu(menu);
        PostMessageW(hWnd, WM_NULL, 0, 0);       // menyu to'g'ri yopilishi uchun
        if (cmd == CMD_CONV_TODIGRAPH || cmd == CMD_CONV_TOSPECIAL) {
            if (target) SetForegroundWindow(target);   // fokusni matn oynasiga qaytaramiz
            TextConvert_Run(cmd == CMD_CONV_TODIGRAPH);
        }
        return 0;
    }

    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    g_hInst = hInst;
    Settings_Load();

    // Ikki nusxa ishlab ketmasligi uchun
    HANDLE once = CreateMutexW(NULL, TRUE, L"HarfbekSingleInstance");
    if (once && GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, T(S_ALREADY_RUNNING), T(S_APP_TITLE), MB_ICONINFORMATION);
        return 0;
    }

    // Ikonkalar
    g_iconBig = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
                                  0, 0, LR_DEFAULTSIZE);
    g_iconSmall = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
                                    GetSystemMetrics(SM_CXSMICON),
                                    GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (!g_iconBig)   g_iconBig   = LoadIconW(NULL, IDI_APPLICATION);
    if (!g_iconSmall) g_iconSmall = g_iconBig;

    // Yashirin oyna
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hIcon         = g_iconBig;
    wc.lpszClassName = L"HarfbekWnd";
    RegisterClassW(&wc);
    g_hWnd = CreateWindowW(wc.lpszClassName, L"Harfbek",
                           WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hInst, NULL);

    // Tray ikonka
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = g_hWnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon            = g_iconSmall;
    lstrcpynW(g_nid.szTip, T(S_TOOLTIP), 128);
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Global hook
    if (!Hook_Install()) {
        MessageBoxW(NULL, T(S_HOOK_FAILED), T(S_ERROR), MB_ICONERROR);
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        return 1;
    }

    // Birinchi ishga tushish: til so'raladi + qo'llanma ko'rsatiladi
    if (g_settings.firstRun) {
        ShowLanguageDialog(g_hWnd);
        UpdateTrayTip();
        ShowLicense(g_hWnd);        // litsenziya/ogohlantirish (tanlangan tilda)
        ShowTutorial(g_hWnd);
        g_settings.firstRun = false;
        Settings_Save();
    }

    // Xabarlar sikli
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Hook_Uninstall();
    if (once) CloseHandle(once);
    return 0;
}
