// Sozlamalar oynasi: til, trigger tugma (tayyor variant yoki "bosib tanlash"),
// Windows bilan ishga tushish. Trigger tugma nomlari uchun yordamchi
// funksiyalar (TriggerFullName / TriggerKeyCap) ham shu yerda.
#include "app.h"
#include <windowsx.h>

// ---------------------------------------------------------------------
//  Trigger tugma nomlari
// ---------------------------------------------------------------------

// To'liq nom, joriy tilda (Sozlamalardagi "Joriy: ..." uchun).
const wchar_t* TriggerFullName(UINT vk)
{
    switch (vk) {
    case VK_RMENU:    return T(S_TRIG_RALT);
    case VK_LMENU:    return T(S_TRIG_LALT);
    case VK_MENU:     return T(S_TRIG_BOTHALT);
    case VK_RCONTROL: return T(S_TRIG_RCTRL);
    case VK_RSHIFT:   return T(S_TRIG_RSHIFT);
    case VK_CAPITAL:  return T(S_TRIG_CAPS);
    case VK_APPS:     return T(S_TRIG_APPS);
    case VK_SCROLL:   return T(S_TRIG_SCROLL);
    case VK_PAUSE:    return T(S_TRIG_PAUSE);
    case VK_F12:      return T(S_TRIG_F12);
    default: {
        static wchar_t buf[48];
        UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        // extended tugmalar (o'ng Ctrl/Alt, strelka...) uchun bayroq
        LONG lp = (LONG)(sc << 16);
        switch (vk) {
        case VK_RCONTROL: case VK_RMENU: case VK_INSERT: case VK_DELETE:
        case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
            lp |= (1 << 24);
        }
        if (sc && GetKeyNameTextW(lp, buf, 48) > 0) return buf;
        wsprintfW(buf, L"VK 0x%02X", vk);
        return buf;
    }
    }
}

// Qisqa keycap nomi (qo'llanmada tugma rasmida chiqadi).
void TriggerKeyCap(UINT vk, wchar_t* out, int n)
{
    const wchar_t* s = NULL;
    switch (vk) {
    case VK_RMENU:    s = L"AltGr"; break;
    case VK_LMENU:
    case VK_MENU:     s = L"Alt";   break;
    case VK_RCONTROL: s = L"Ctrl";  break;
    case VK_RSHIFT:   s = L"Shift"; break;
    case VK_CAPITAL:  s = L"Caps";  break;
    case VK_APPS:     s = L"Menu";  break;
    case VK_SCROLL:   s = L"ScrLk"; break;
    case VK_PAUSE:    s = L"Pause"; break;
    case VK_F12:      s = L"F12";   break;
    default: {
        UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        if (sc && GetKeyNameTextW((LONG)(sc << 16), out, n) > 0) return;
        wsprintfW(out, L"0x%02X", vk);
        return;
    }
    }
    lstrcpynW(out, s, n);
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
    { L"'  ( ' )",   L'\''     },   // U+0027 oddiy apostrof
    { L"ʻ  ( oʻzbek )", (wchar_t)0x02BB },   // rasmiy tutuq belgisi
    { L"`  ( ` )",   L'`'      },   // U+0060 grave
    { L"’  ( ’ )", (wchar_t)0x2019 },        // egik apostrof
};
static const int kApoOptCount = sizeof(kApoOpts) / sizeof(kApoOpts[0]);

// Dialog davomida vaqtinchalik tanlovlar
static Lang    s_lang;
static UINT    s_trig;
static wchar_t s_apo;

// "Joriy: <tugma>" matnini yangilaydi
static void UpdateCurrentText(HWND h)
{
    wchar_t b[160];
    wsprintfW(b, L"%s  %s", T(S_SET_CURRENT), TriggerFullName(s_trig));
    SetDlgItemTextW(h, IDC_SET_CURRENT, b);
}

// Combobox tanlovini s_trig ga moslaydi (tayyor variantlardan biri bo'lsa)
static void SelectTrigInCombo(HWND h)
{
    HWND c = GetDlgItem(h, IDC_SET_TRIG);
    int sel = -1;
    for (int i = 0; i < kTrigOptCount; ++i)
        if (kTrigOpts[i].vk == s_trig) { sel = i; break; }
    SendMessageW(c, CB_SETCURSEL, sel, 0);
}

static INT_PTR CALLBACK SettingsProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        s_lang = g_settings.lang;
        s_trig = g_settings.triggerVk;
        s_apo  = g_settings.apo;

        SetWindowTextW(h, T(S_SET_TITLE));
        SetDlgItemTextW(h, IDC_SET_LANG_LBL, T(S_SET_LANGUAGE));
        SetDlgItemTextW(h, IDC_SET_TRIG_LBL, T(S_SET_TRIGGER));
        SetDlgItemTextW(h, IDC_SET_APO_LBL,  T(S_SET_APOSTROPHE));
        SetDlgItemTextW(h, IDC_SET_STARTUP,  T(S_SET_STARTUP));
        SetDlgItemTextW(h, IDOK,             T(S_SET_SAVE));
        SetDlgItemTextW(h, IDCANCEL,         T(S_SET_CANCEL));

        // Til combobox (tartib: UZ, RU, EN — Lang enum bilan bir xil)
        HWND lc = GetDlgItem(h, IDC_SET_LANG);
        SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"Oʻzbekcha");
        SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"Русский");
        SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessageW(lc, CB_SETCURSEL, (WPARAM)s_lang, 0);

        // Trigger combobox: faqat sinalgan tayyor variantlar
        HWND tc = GetDlgItem(h, IDC_SET_TRIG);
        for (int i = 0; i < kTrigOptCount; ++i) {
            int idx = (int)SendMessageW(tc, CB_ADDSTRING, 0, (LPARAM)T(kTrigOpts[i].name));
            SendMessageW(tc, CB_SETITEMDATA, idx, (LPARAM)kTrigOpts[i].vk);
        }
        SelectTrigInCombo(h);

        // Apostrof combobox
        HWND ac = GetDlgItem(h, IDC_SET_APO);
        int apoSel = 0;
        for (int i = 0; i < kApoOptCount; ++i) {
            int idx = (int)SendMessageW(ac, CB_ADDSTRING, 0, (LPARAM)kApoOpts[i].label);
            SendMessageW(ac, CB_SETITEMDATA, idx, (LPARAM)kApoOpts[i].ch);
            if (kApoOpts[i].ch == s_apo) apoSel = idx;
        }
        SendMessageW(ac, CB_SETCURSEL, apoSel, 0);

        // Startup checkbox — registrdagi haqiqiy holatdan
        CheckDlgButton(h, IDC_SET_STARTUP,
                       Startup_IsEnabled() ? BST_CHECKED : BST_UNCHECKED);

        UpdateCurrentText(h);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDC_SET_TRIG:
            if (HIWORD(wp) == CBN_SELCHANGE) {
                HWND tc = (HWND)lp;
                int sel = (int)SendMessageW(tc, CB_GETCURSEL, 0, 0);
                if (sel < 0) return TRUE;
                s_trig = (UINT)SendMessageW(tc, CB_GETITEMDATA, sel, 0);
                UpdateCurrentText(h);
            }
            return TRUE;

        case IDOK:
        {
            HWND lc = GetDlgItem(h, IDC_SET_LANG);
            int li = (int)SendMessageW(lc, CB_GETCURSEL, 0, 0);
            if (li >= 0 && li < LANG_COUNT) s_lang = (Lang)li;

            HWND ac = GetDlgItem(h, IDC_SET_APO);
            int ai = (int)SendMessageW(ac, CB_GETCURSEL, 0, 0);
            if (ai >= 0) s_apo = (wchar_t)SendMessageW(ac, CB_GETITEMDATA, ai, 0);

            g_settings.lang      = s_lang;
            g_settings.triggerVk = s_trig;
            g_settings.apo       = s_apo;

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
