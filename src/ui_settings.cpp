// Sozlamalar oynasi: til, rejim (trigger / avto), trigger tugma (tayyor variantlardan),
// apostrof belgisi, Windows bilan ishga tushish. Trigger tugma nomlari uchun yordamchi
// funksiya (TriggerKeyCap) ham shu yerda.
#include "app.h"

// ---------------------------------------------------------------------
//  Trigger tugma nomlari
// ---------------------------------------------------------------------

// Qisqa keycap nomi (qo'llanmada tugma rasmida, tray tooltip'da chiqadi).
// Har doim bo'sh bo'lmagan satr qaytaradi.
void TriggerKeyCap(UINT vk, wchar_t* out, int n)
{
    const wchar_t* s = NULL;
    switch (vk) {
    case VK_RMENU:    s = L"AltGr"; break;
    case VK_LMENU:
    case VK_MENU:     s = L"Alt";   break;
    case VK_RCONTROL: s = L"Ctrl";  break;
    case VK_RSHIFT:   s = L"R Shift"; break;
    case VK_CAPITAL:  s = L"Caps";  break;
    case VK_APPS:     s = L"Menu";  break;
    case VK_SCROLL:   s = L"ScrLk"; break;
    case VK_PAUSE:    s = L"Pause"; break;
    case VK_F12:      s = L"F12";   break;
    }
    if (s) {
        lstrcpynW(out, s, n);
    } else {
        // xavfsiz fallback: ruxsat etilmagan VK ham kod ko'rinishida chiqadi
        wchar_t b[16];   // "0x" + 8 hex raqam + NUL: istalgan UINT sig'adi
        wsprintfW(b, L"0x%02X", vk);
        lstrcpynW(out, b, n);
    }
}

// ---------------------------------------------------------------------
//  Sozlamalar dialogi
// ---------------------------------------------------------------------

// Trigger variantlari (combobox uchun) — FAQAT sinalgan tugmalar.
// Alt/Ctrl/Shift oilasi oddiy modifikator sifatida ishlaydi; qolganlari
// (Caps, Menyu, Scroll Lock, Pause, F12) hook tomonidan to'liq yutiladi,
// shuning uchun o'zining standart vazifasini bajarmaydi — bu ataylab shunday.
struct TrigOpt { StrId name; UINT vk; };
static const TrigOpt kTrigOpts[] = {
    { S_TRIG_RALT,    VK_RMENU    },
    { S_TRIG_LALT,    VK_LMENU    },
    { S_TRIG_BOTHALT, VK_MENU     },
    { S_TRIG_RCTRL,   VK_RCONTROL },
    { S_TRIG_RSHIFT,  VK_RSHIFT   },
    { S_TRIG_CAPS,    VK_CAPITAL  },
    { S_TRIG_APPS,    VK_APPS     },
    { S_TRIG_SCROLL,  VK_SCROLL   },
    { S_TRIG_PAUSE,   VK_PAUSE    },
    { S_TRIG_F12,     VK_F12      },
};
static const int kTrigOptCount = sizeof(kTrigOpts) / sizeof(kTrigOpts[0]);

// Berilgan VK sinalgan trigger ro'yxatida bormi (ini fayldan o'qishda
// tekshiriladi — eski/buzilgan qiymatlar standartga qaytadi).
bool Trigger_IsAllowed(UINT vk)
{
    for (int i = 0; i < kTrigOptCount; ++i)
        if (kTrigOpts[i].vk == vk) return true;
    return false;
}

// Apostrof variantlari (o' g' uchun chiqadigan belgi)
struct ApoOpt { const wchar_t* label; wchar_t ch; };
static const ApoOpt kApoOpts[] = {
    { L"ʻ  ( oʻzbek )", (wchar_t)0x02BB },   // rasmiy tutuq belgisi (standart)
    { L"'  ( ' )",   L'\''     },   // U+0027 oddiy apostrof
    { L"`  ( ` )",   L'`'      },   // U+0060 grave
    { L"’  ( ’ )", (wchar_t)0x2019 },        // egik apostrof
};
static const int kApoOptCount = sizeof(kApoOpts) / sizeof(kApoOpts[0]);

// Trigger combobox tanlovini berilgan VK ga moslaydi
static void SelectTrigInCombo(HWND h, UINT vk)
{
    HWND c = GetDlgItem(h, IDC_SET_TRIG);
    int sel = -1;
    for (int i = 0; i < kTrigOptCount; ++i)
        if (kTrigOpts[i].vk == vk) { sel = i; break; }
    SendMessageW(c, CB_SETCURSEL, sel, 0);
}

// Tanlangan rejim (combobox item data'dan); tanlov bo'lmasa joriy rejim
static InputMode SelectedMode(HWND h)
{
    HWND mc = GetDlgItem(h, IDC_SET_MODE);
    int mi = (int)SendMessageW(mc, CB_GETCURSEL, 0, 0);
    if (mi < 0) return g_settings.mode;
    return (InputMode)SendMessageW(mc, CB_GETITEMDATA, mi, 0);
}

// Avto rejimda trigger ishlatilmaydi — uning combobox'i va yorlig'i o'chiriladi
static void UpdateTriggerEnabled(HWND h)
{
    BOOL on = SelectedMode(h) != MODE_AUTO;
    EnableWindow(GetDlgItem(h, IDC_SET_TRIG_LBL), on);
    EnableWindow(GetDlgItem(h, IDC_SET_TRIG),     on);
}

static INT_PTR CALLBACK SettingsProc(HWND h, UINT msg, WPARAM wp, LPARAM /*lp*/)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        SetWindowTextW(h, T(S_SET_TITLE));
        SetDlgItemTextW(h, IDC_SET_LANG_LBL, T(S_SET_LANGUAGE));
        SetDlgItemTextW(h, IDC_SET_MODE_LBL, T(S_SET_MODE));
        SetDlgItemTextW(h, IDC_SET_TRIG_LBL, T(S_SET_TRIGGER));
        SetDlgItemTextW(h, IDC_SET_APO_LBL,  T(S_SET_APOSTROPHE));
        SetDlgItemTextW(h, IDC_SET_STARTUP,  T(S_SET_STARTUP));
        SetDlgItemTextW(h, IDOK,             T(S_SET_SAVE));
        SetDlgItemTextW(h, IDCANCEL,         T(S_SET_CANCEL));

        // Til combobox (tartib: UZ, RU, EN — Lang enum bilan bir xil)
        HWND lc = GetDlgItem(h, IDC_SET_LANG);
        SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"Özbekça");
        SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"Русский");
        SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessageW(lc, CB_SETCURSEL, (WPARAM)g_settings.lang, 0);

        // Rejim combobox (item data = InputMode)
        HWND mc = GetDlgItem(h, IDC_SET_MODE);
        const struct { StrId name; InputMode mode; } modeOpts[] = {
            { S_MODE_TRIGGER, MODE_TRIGGER },
            { S_MODE_AUTO,    MODE_AUTO    },
        };
        int modeSel = 0;
        for (int i = 0; i < MODE_COUNT; ++i) {
            int idx = (int)SendMessageW(mc, CB_ADDSTRING, 0, (LPARAM)T(modeOpts[i].name));
            SendMessageW(mc, CB_SETITEMDATA, idx, (LPARAM)modeOpts[i].mode);
            if (modeOpts[i].mode == g_settings.mode) modeSel = idx;
        }
        SendMessageW(mc, CB_SETCURSEL, modeSel, 0);

        // Trigger combobox: faqat sinalgan tayyor variantlar
        HWND tc = GetDlgItem(h, IDC_SET_TRIG);
        for (int i = 0; i < kTrigOptCount; ++i) {
            int idx = (int)SendMessageW(tc, CB_ADDSTRING, 0, (LPARAM)T(kTrigOpts[i].name));
            SendMessageW(tc, CB_SETITEMDATA, idx, (LPARAM)kTrigOpts[i].vk);
        }
        SelectTrigInCombo(h, g_settings.triggerVk);

        // Apostrof combobox
        HWND ac = GetDlgItem(h, IDC_SET_APO);
        int apoSel = 0;
        for (int i = 0; i < kApoOptCount; ++i) {
            int idx = (int)SendMessageW(ac, CB_ADDSTRING, 0, (LPARAM)kApoOpts[i].label);
            SendMessageW(ac, CB_SETITEMDATA, idx, (LPARAM)kApoOpts[i].ch);
            if (kApoOpts[i].ch == g_settings.apo) apoSel = idx;
        }
        SendMessageW(ac, CB_SETCURSEL, apoSel, 0);

        // Startup checkbox — registrdagi haqiqiy holatdan
        CheckDlgButton(h, IDC_SET_STARTUP,
                       Startup_IsEnabled() ? BST_CHECKED : BST_UNCHECKED);

        UpdateTriggerEnabled(h);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDC_SET_MODE:
            if (HIWORD(wp) == CBN_SELCHANGE) UpdateTriggerEnabled(h);
            return TRUE;

        case IDOK:
        {
            // Tanlov bo'lmasa (cursel < 0) joriy qiymat saqlanadi
            Lang    lang = g_settings.lang;
            InputMode mode = SelectedMode(h);
            UINT    trig = g_settings.triggerVk;
            wchar_t apo  = g_settings.apo;

            int li = (int)SendMessageW(GetDlgItem(h, IDC_SET_LANG), CB_GETCURSEL, 0, 0);
            if (li >= 0 && li < LANG_COUNT) lang = (Lang)li;

            HWND tc = GetDlgItem(h, IDC_SET_TRIG);
            int ti = (int)SendMessageW(tc, CB_GETCURSEL, 0, 0);
            if (ti >= 0) trig = (UINT)SendMessageW(tc, CB_GETITEMDATA, ti, 0);

            HWND ac = GetDlgItem(h, IDC_SET_APO);
            int ai = (int)SendMessageW(ac, CB_GETCURSEL, 0, 0);
            if (ai >= 0) apo = (wchar_t)SendMessageW(ac, CB_GETITEMDATA, ai, 0);

            g_settings.lang      = lang;
            g_settings.mode      = mode;
            g_settings.triggerVk = trig;
            g_settings.apo       = apo;

            bool su = IsDlgButtonChecked(h, IDC_SET_STARTUP) == BST_CHECKED;
            if (su != Startup_IsEnabled()) Startup_Set(su);
            g_settings.runAtStartup = su;

            Settings_Save();
            Hook_ResetTriggerState();   // eski triggerning "bosilgan" holati qolmasin
            EndDialog(h, 1);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(h, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowSettings(HWND parent)
{
    DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_SETTINGS), parent, SettingsProc);
}
