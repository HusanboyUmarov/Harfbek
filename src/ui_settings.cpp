// Sozlamalar oynasi: tab control + 4 ta sahifa (Umumiy | Rejim | Tezkor tugmalar |
// Istisnolar). Trigger tugma nomlari uchun yordamchi funksiya (TriggerKeyCap) ham
// shu yerda.
#include "app.h"
#include <commctrl.h>
#include <wchar.h>

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

// Tab sahifalari (TCM_INSERTITEM tartibi bilan bir xil)
enum { PG_GENERAL, PG_MODE, PG_HOTKEYS, PG_EXCLUDE, PG_COUNT };

// Istisnolar ro'yxati uchun chegara (Settings_SetExcludes: maks 1023 belgi)
#define EXCLUDE_MAX  1024

// Dialog holati — DialogBoxParamW lParam orqali beriladi (static emas: qayta kirishga chidamli)
struct SetUi {
    HWND dlg;
    HWND tab;
    HWND pg[PG_COUNT];
    bool hkApplied;     // Hotkeys_Apply allaqachon chaqirilganmi
};

static HWND Item(SetUi* u, int page, int id) { return GetDlgItem(u->pg[page], id); }

// ---------------------------------------------------------------------
//  Tab sahifalarini boshqarish
// ---------------------------------------------------------------------

// Sahifa fonini tab mavzusiga moslaydi. uxtheme.dll faqat shu yerda yuklanadi
// va darhol bo'shatiladi (qo'shimcha lib ham, doimiy RAM ham kerak emas).
static void ThemeTabPages(SetUi* u)
{
    HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (!ux) return;
    typedef HRESULT (WINAPI *PFN_EnableThemeDialogTexture)(HWND, DWORD);
    PFN_EnableThemeDialogTexture fn =
        (PFN_EnableThemeDialogTexture)GetProcAddress(ux, "EnableThemeDialogTexture");
    if (fn)
        for (int i = 0; i < PG_COUNT; ++i)
            fn(u->pg[i], 0x6);      // ETDT_ENABLETAB
    FreeLibrary(ux);
}

// Sahifalarni tabning ko'rinish maydoniga joylaydi (hozirgi sahifadan boshqasi yashirin)
static void LayoutPages(SetUi* u)
{
    RECT rc;
    GetClientRect(u->tab, &rc);
    SendMessageW(u->tab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rc);
    MapWindowPoints(u->tab, u->dlg, (POINT*)&rc, 2);
    int cur = TabCtrl_GetCurSel(u->tab);
    for (int i = 0; i < PG_COUNT; ++i)
        SetWindowPos(u->pg[i], HWND_TOP, rc.left, rc.top, rc.right - rc.left,
                     rc.bottom - rc.top,
                     SWP_NOACTIVATE | (i == cur ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

static void SelectPage(SetUi* u, int page)
{
    TabCtrl_SetCurSel(u->tab, page);
    for (int i = 0; i < PG_COUNT; ++i)
        ShowWindow(u->pg[i], i == page ? SW_SHOW : SW_HIDE);
}

// ---------------------------------------------------------------------
//  Tab "Umumiy"
// ---------------------------------------------------------------------

static void InitGeneral(SetUi* u)
{
    HWND p = u->pg[PG_GENERAL];
    SetDlgItemTextW(p, IDC_SET_LANG_LBL, T(S_SET_LANGUAGE));
    SetDlgItemTextW(p, IDC_SET_APO_LBL,  T(S_SET_APOSTROPHE));
    SetDlgItemTextW(p, IDC_SET_STARTUP,  T(S_SET_STARTUP));
    SetDlgItemTextW(p, IDC_SET_NOTIFY,   T(S_SET_NOTIFY));

    // Til combobox (tartib: UZ, RU, EN — Lang enum bilan bir xil)
    HWND lc = GetDlgItem(p, IDC_SET_LANG);
    SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"Özbekça");
    SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"Русский");
    SendMessageW(lc, CB_ADDSTRING, 0, (LPARAM)L"English");
    SendMessageW(lc, CB_SETCURSEL, (WPARAM)g_settings.lang, 0);

    // Apostrof combobox
    HWND ac = GetDlgItem(p, IDC_SET_APO);
    int apoSel = 0;
    for (int i = 0; i < kApoOptCount; ++i) {
        int idx = (int)SendMessageW(ac, CB_ADDSTRING, 0, (LPARAM)kApoOpts[i].label);
        SendMessageW(ac, CB_SETITEMDATA, idx, (LPARAM)kApoOpts[i].ch);
        if (kApoOpts[i].ch == g_settings.apo) apoSel = idx;
    }
    SendMessageW(ac, CB_SETCURSEL, apoSel, 0);

    // Startup checkbox — registrdagi haqiqiy holatdan
    CheckDlgButton(p, IDC_SET_STARTUP, Startup_IsEnabled() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(p, IDC_SET_NOTIFY,  g_settings.notify ? BST_CHECKED : BST_UNCHECKED);
}

// ---------------------------------------------------------------------
//  Tab "Rejim"
// ---------------------------------------------------------------------

static bool IsAutoChecked(SetUi* u)
{
    return IsDlgButtonChecked(u->pg[PG_MODE], IDC_SET_MODE_AUTO) == BST_CHECKED;
}

// Combobox'dagi trigger (tanlov bo'lmasa joriy qiymat)
static UINT SelectedTrigger(SetUi* u)
{
    HWND tc = Item(u, PG_MODE, IDC_SET_TRIG);
    int ti = (int)SendMessageW(tc, CB_GETCURSEL, 0, 0);
    return ti >= 0 ? (UINT)SendMessageW(tc, CB_GETITEMDATA, ti, 0) : g_settings.triggerVk;
}

// Pastdagi misol matni: rejimga va tanlangan triggerga qarab
static void UpdateExample(SetUi* u)
{
    wchar_t text[256];
    if (IsAutoChecked(u)) {
        lstrcpynW(text, T(S_SET_EXAMPLE_AUTO), 256);
    } else {
        wchar_t cap[24];
        TriggerKeyCap(SelectedTrigger(u), cap, 24);
        wsprintfW(text, T(S_SET_EXAMPLE_TRIGGER), cap);
    }
    SetDlgItemTextW(u->pg[PG_MODE], IDC_SET_EXAMPLE, text);
}

// Tanlangan rejimga qarab ikkinchisining boshqaruvlari o'chiriladi
static void UpdateModeUi(SetUi* u)
{
    HWND p = u->pg[PG_MODE];
    BOOL autoMode = IsAutoChecked(u);
    EnableWindow(GetDlgItem(p, IDC_SET_TRIG_LBL), !autoMode);
    EnableWindow(GetDlgItem(p, IDC_SET_TRIG),     !autoMode);
    EnableWindow(GetDlgItem(p, IDC_SET_RULES_LBL), autoMode);
    for (int id = IDC_SET_RULE_O; id <= IDC_SET_RULE_CH; ++id)
        EnableWindow(GetDlgItem(p, id), autoMode);
    UpdateExample(u);
}

static void InitMode(SetUi* u)
{
    HWND p = u->pg[PG_MODE];
    SetDlgItemTextW(p, IDC_SET_MODE_TRIGGER, T(S_MODE_TRIGGER));
    SetDlgItemTextW(p, IDC_SET_MODE_AUTO,    T(S_MODE_AUTO));
    SetDlgItemTextW(p, IDC_SET_TRIG_LBL,     T(S_SET_TRIGGER));
    SetDlgItemTextW(p, IDC_SET_RULES_LBL,    T(S_SET_RULES));
    // qoida yozuvlari — literal (tarjimasiz)
    SetDlgItemTextW(p, IDC_SET_RULE_O,  L"o' → ö");
    SetDlgItemTextW(p, IDC_SET_RULE_G,  L"g' → ğ");
    SetDlgItemTextW(p, IDC_SET_RULE_SH, L"sh → ş");
    SetDlgItemTextW(p, IDC_SET_RULE_CH, L"ch → ç");

    // Trigger combobox: faqat sinalgan tayyor variantlar
    HWND tc = GetDlgItem(p, IDC_SET_TRIG);
    int trigSel = -1;
    for (int i = 0; i < kTrigOptCount; ++i) {
        int idx = (int)SendMessageW(tc, CB_ADDSTRING, 0, (LPARAM)T(kTrigOpts[i].name));
        SendMessageW(tc, CB_SETITEMDATA, idx, (LPARAM)kTrigOpts[i].vk);
        if (kTrigOpts[i].vk == g_settings.triggerVk) trigSel = idx;
    }
    SendMessageW(tc, CB_SETCURSEL, trigSel, 0);

    CheckRadioButton(p, IDC_SET_MODE_TRIGGER, IDC_SET_MODE_AUTO,
                     g_settings.mode == MODE_AUTO ? IDC_SET_MODE_AUTO : IDC_SET_MODE_TRIGGER);

    CheckDlgButton(p, IDC_SET_RULE_O,  (g_settings.autoRules & AUTO_RULE_O)  ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(p, IDC_SET_RULE_G,  (g_settings.autoRules & AUTO_RULE_G)  ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(p, IDC_SET_RULE_SH, (g_settings.autoRules & AUTO_RULE_SH) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(p, IDC_SET_RULE_CH, (g_settings.autoRules & AUTO_RULE_CH) ? BST_CHECKED : BST_UNCHECKED);

    UpdateModeUi(u);
}

// ---------------------------------------------------------------------
//  Tab "Tezkor tugmalar"
// ---------------------------------------------------------------------

static const int kHkIds[3] = { IDC_SET_HK_MODE, IDC_SET_HK_ENABLE, IDC_SET_HK_CONVERT };

static void InitHotkeys(SetUi* u)
{
    HWND p = u->pg[PG_HOTKEYS];
    SetDlgItemTextW(p, IDC_SET_HK_MODE_LBL,    T(S_HK_MODE));
    SetDlgItemTextW(p, IDC_SET_HK_ENABLE_LBL,  T(S_HK_ENABLE));
    SetDlgItemTextW(p, IDC_SET_HK_CONVERT_LBL, T(S_HK_CONVERT));
    SetDlgItemTextW(p, IDC_SET_HK_HINT,        T(S_HK_HINT));

    const WORD cur[3] = { g_settings.hkMode, g_settings.hkEnable, g_settings.hkConvert };
    for (int i = 0; i < 3; ++i) {
        HWND hk = GetDlgItem(p, kHkIds[i]);
        // modifikatorsiz, faqat Shift va Ctrl+Alt (= AltGr) kombinatsiyalari taqiqlanadi —
        // oddiy yozishga va AltGr bilan harf kiritishga xalaqit beradi
        SendMessageW(hk, HKM_SETRULES, HKCOMB_NONE | HKCOMB_S | HKCOMB_CA, HOTKEYF_CONTROL | HOTKEYF_SHIFT);
        SendMessageW(hk, HKM_SETHOTKEY, cur[i], 0);
    }
}

static WORD GetHk(SetUi* u, int i)
{
    return (WORD)SendMessageW(Item(u, PG_HOTKEYS, kHkIds[i]), HKM_GETHOTKEY, 0, 0);
}

// Ikki maydonda bir xil kombinatsiya bo'lsa ogohlantiradi va tabni ko'rsatadi
static bool HotkeysConflict(SetUi* u)
{
    WORD hk[3] = { GetHk(u, 0), GetHk(u, 1), GetHk(u, 2) };
    for (int i = 0; i < 3; ++i)
        for (int j = i + 1; j < 3; ++j)
            if (hk[i] && hk[i] == hk[j]) {
                wchar_t key[64], msg[256];
                Hotkey_Format(hk[i], key, 64);
                wsprintfW(msg, T(S_HK_BUSY), key);
                SelectPage(u, PG_HOTKEYS);
                SetFocus(Item(u, PG_HOTKEYS, kHkIds[j]));
                MessageBoxW(u->dlg, msg, T(S_SET_TITLE), MB_OK | MB_ICONWARNING);
                return true;
            }
    return false;
}

// Hotkeys_Apply natijasidagi band tugmalar haqida bitta xabar
static void WarnBusyHotkeys(SetUi* u, UINT busy)
{
    const struct { UINT bit; WORD hk; } keys[3] = {
        { HKID_MODE, g_settings.hkMode }, { HKID_ENABLE, g_settings.hkEnable },
        { HKID_CONVERT, g_settings.hkConvert },
    };
    wchar_t msg[768] = L"";
    int len = 0;
    for (int i = 0; i < 3; ++i) {
        if (!(busy & keys[i].bit)) continue;
        wchar_t key[64], line[256];
        Hotkey_Format(keys[i].hk, key, 64);
        wsprintfW(line, T(S_HK_BUSY), key);
        int ll = lstrlenW(line);
        if (len + ll + 2 >= 768) break;
        if (len) msg[len++] = L'\n';
        lstrcpyW(msg + len, line);
        len += ll;
    }
    if (len) MessageBoxW(u->dlg, msg, T(S_SET_TITLE), MB_OK | MB_ICONWARNING);
}

// ---------------------------------------------------------------------
//  Tab "Istisnolar"
// ---------------------------------------------------------------------

// Ro'yxatni ";" bilan ajratilgan qatorga yig'adi; uzunligini qaytaradi
static int ExcludeJoin(SetUi* u, wchar_t* out, int n)
{
    HWND lb = Item(u, PG_EXCLUDE, IDC_SET_EX_LIST);
    int cnt = (int)SendMessageW(lb, LB_GETCOUNT, 0, 0);
    int len = 0;
    out[0] = 0;
    for (int i = 0; i < cnt; ++i) {
        wchar_t item[MAX_PATH];
        if (SendMessageW(lb, LB_GETTEXTLEN, i, 0) >= MAX_PATH) continue;
        SendMessageW(lb, LB_GETTEXT, i, (LPARAM)item);
        int il = lstrlenW(item);
        if (len + il + 2 > n) break;
        if (len) out[len++] = L';';
        lstrcpyW(out + len, item);
        len += il;
    }
    return len;
}

static void InitExclude(SetUi* u)
{
    HWND p = u->pg[PG_EXCLUDE];
    SetDlgItemTextW(p, IDC_SET_EX_HINT,   T(S_EX_HINT));
    SetDlgItemTextW(p, IDC_SET_EX_ADD,    T(S_EX_ADD));
    SetDlgItemTextW(p, IDC_SET_EX_BROWSE, T(S_EX_BROWSE));
    SetDlgItemTextW(p, IDC_SET_EX_REMOVE, T(S_EX_REMOVE));
    SendDlgItemMessageW(p, IDC_SET_EX_EDIT, EM_LIMITTEXT, MAX_PATH - 1, 0);

    wchar_t buf[EXCLUDE_MAX];
    Settings_GetExcludes(buf, EXCLUDE_MAX);
    HWND lb = GetDlgItem(p, IDC_SET_EX_LIST);
    wchar_t* s = buf;
    while (*s) {
        wchar_t* e = s;
        while (*e && *e != L';') ++e;
        wchar_t sep = *e;
        *e = 0;
        if (*s) SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)s);
        if (!sep) break;
        s = e + 1;
    }
}

// Nomni ro'yxatga qo'shadi: faqat fayl nomi, ".exe" bilan, kichik harfda, takrorsiz.
// Qo'shilgan (yoki allaqachon bor) bo'lsa true.
static bool ExcludeAdd(SetUi* u, const wchar_t* raw)
{
    // yo'l berilgan bo'lsa fayl nomi qismi olinadi
    const wchar_t* base = raw;
    for (const wchar_t* c = raw; *c; ++c)
        if (*c == L'\\' || *c == L'/') base = c + 1;

    wchar_t name[MAX_PATH];
    lstrcpynW(name, base, MAX_PATH);
    int len = lstrlenW(name);
    while (len && name[len - 1] == L' ') name[--len] = 0;
    int skip = 0;
    while (name[skip] == L' ') ++skip;
    if (skip) { MoveMemory(name, name + skip, (len - skip + 1) * sizeof(wchar_t)); len -= skip; }
    if (!len) return false;

    // ";" ro'yxat ajratgichi, qolganlari fayl nomida bo'lishi mumkin emas
    for (int i = 0; i < len; ++i)
        if (wcschr(L";:*?\"<>|", name[i])) { MessageBeep(MB_ICONWARNING); return false; }

    if (len < 4 || lstrcmpiW(name + len - 4, L".exe") != 0) {
        if (len + 4 >= MAX_PATH) return false;
        lstrcpyW(name + len, L".exe");
        len += 4;
    }
    if (len == 4) return false;     // faqat ".exe"
    CharLowerBuffW(name, len);

    HWND lb = Item(u, PG_EXCLUDE, IDC_SET_EX_LIST);
    if (SendMessageW(lb, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)name) != LB_ERR)
        return true;                // takror — qo'shilmaydi

    wchar_t cur[EXCLUDE_MAX];
    if (ExcludeJoin(u, cur, EXCLUDE_MAX) + 1 + len >= EXCLUDE_MAX) {    // saqlash chegarasi
        MessageBeep(MB_ICONWARNING);
        return false;
    }
    SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)name);
    return true;
}

static void ExcludeAddFromEdit(SetUi* u)
{
    wchar_t raw[MAX_PATH];
    HWND ed = Item(u, PG_EXCLUDE, IDC_SET_EX_EDIT);
    GetWindowTextW(ed, raw, MAX_PATH);
    if (ExcludeAdd(u, raw)) SetWindowTextW(ed, L"");
    SetFocus(ed);
}

static void ExcludeRemove(SetUi* u)
{
    HWND lb = Item(u, PG_EXCLUDE, IDC_SET_EX_LIST);
    int sel = (int)SendMessageW(lb, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) return;
    SendMessageW(lb, LB_DELETESTRING, sel, 0);
    int cnt = (int)SendMessageW(lb, LB_GETCOUNT, 0, 0);
    if (cnt > 0) SendMessageW(lb, LB_SETCURSEL, sel < cnt ? sel : cnt - 1, 0);
}

// "Tanlash..." — fayl dialogi. comdlg32 link qilinmaydi: faqat shu tugma bosilganda
// yuklanadi va darhol bo'shatiladi (RAM budjeti).
static void ExcludeBrowse(SetUi* u)
{
    HMODULE dll = LoadLibraryW(L"comdlg32.dll");
    if (!dll) return;
    typedef BOOL (WINAPI *PFN_GetOpenFileNameW)(LPOPENFILENAMEW);
    PFN_GetOpenFileNameW fn = (PFN_GetOpenFileNameW)GetProcAddress(dll, "GetOpenFileNameW");
    if (fn) {
        // filtr: "<nom>\0*.exe\0\0"
        wchar_t filter[160] = L"";
        lstrcpynW(filter, T(S_EX_FILTER), 128);
        int fl = lstrlenW(filter);
        lstrcpyW(filter + fl + 1, L"*.exe");    // keyingi \0 lar massiv nollari bilan to'ldirilgan

        wchar_t file[MAX_PATH] = L"";
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = u->dlg;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile   = file;
        ofn.nMaxFile    = MAX_PATH;
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        if (fn(&ofn)) ExcludeAdd(u, file);
    }
    FreeLibrary(dll);
}

// ---------------------------------------------------------------------
//  Sahifa dialogi va asosiy dialog
// ---------------------------------------------------------------------

// Hamma sahifalar uchun umumiy proc: boshqaruv ID lari sahifalar bo'yicha takrorlanmaydi
static INT_PTR CALLBACK PageProc(HWND hp, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_INITDIALOG) {
        SetWindowLongPtrW(hp, DWLP_USER, lp);
        return FALSE;               // fokusni o'zgartirmaymiz
    }
    SetUi* u = (SetUi*)GetWindowLongPtrW(hp, DWLP_USER);
    if (msg != WM_COMMAND || !u) return FALSE;

    switch (LOWORD(wp))
    {
    case IDC_SET_MODE_TRIGGER:
    case IDC_SET_MODE_AUTO:
        if (HIWORD(wp) == BN_CLICKED) {
            // radiolar alohida guruhlarda — tanlovni o'zimiz yuritamiz
            CheckRadioButton(hp, IDC_SET_MODE_TRIGGER, IDC_SET_MODE_AUTO, LOWORD(wp));
            UpdateModeUi(u);
        }
        return TRUE;
    case IDC_SET_TRIG:
        if (HIWORD(wp) == CBN_SELCHANGE) UpdateExample(u);
        return TRUE;
    case IDC_SET_EX_ADD:
        if (HIWORD(wp) == BN_CLICKED) ExcludeAddFromEdit(u);
        return TRUE;
    case IDC_SET_EX_BROWSE:
        if (HIWORD(wp) == BN_CLICKED) ExcludeBrowse(u);
        return TRUE;
    case IDC_SET_EX_REMOVE:
        if (HIWORD(wp) == BN_CLICKED) ExcludeRemove(u);
        return TRUE;
    }
    return FALSE;
}

// Hamma sahifalardagi qiymatlarni g_settings ga yozib, saqlaydi.
// false — saqlash to'xtatildi (dialog ochiq qoladi).
static bool SaveAll(SetUi* u)
{
    if (HotkeysConflict(u)) return false;

    // Tanlov bo'lmasa (cursel < 0) joriy qiymat saqlanadi
    HWND pg = u->pg[PG_GENERAL];
    int li = (int)SendMessageW(GetDlgItem(pg, IDC_SET_LANG), CB_GETCURSEL, 0, 0);
    if (li >= 0 && li < LANG_COUNT) g_settings.lang = (Lang)li;

    HWND ac = GetDlgItem(pg, IDC_SET_APO);
    int ai = (int)SendMessageW(ac, CB_GETCURSEL, 0, 0);
    if (ai >= 0) g_settings.apo = (wchar_t)SendMessageW(ac, CB_GETITEMDATA, ai, 0);

    g_settings.notify = IsDlgButtonChecked(pg, IDC_SET_NOTIFY) == BST_CHECKED;

    bool su = IsDlgButtonChecked(pg, IDC_SET_STARTUP) == BST_CHECKED;
    if (su != Startup_IsEnabled()) Startup_Set(su);
    g_settings.runAtStartup = su;

    HWND pm = u->pg[PG_MODE];
    g_settings.mode      = IsAutoChecked(u) ? MODE_AUTO : MODE_TRIGGER;
    g_settings.triggerVk = SelectedTrigger(u);
    UINT rules = 0;
    if (IsDlgButtonChecked(pm, IDC_SET_RULE_O)  == BST_CHECKED) rules |= AUTO_RULE_O;
    if (IsDlgButtonChecked(pm, IDC_SET_RULE_G)  == BST_CHECKED) rules |= AUTO_RULE_G;
    if (IsDlgButtonChecked(pm, IDC_SET_RULE_SH) == BST_CHECKED) rules |= AUTO_RULE_SH;
    if (IsDlgButtonChecked(pm, IDC_SET_RULE_CH) == BST_CHECKED) rules |= AUTO_RULE_CH;
    g_settings.autoRules = rules;

    g_settings.hkMode    = GetHk(u, 0);
    g_settings.hkEnable  = GetHk(u, 1);
    g_settings.hkConvert = GetHk(u, 2);

    wchar_t ex[EXCLUDE_MAX];
    ExcludeJoin(u, ex, EXCLUDE_MAX);
    Settings_SetExcludes(ex);

    Settings_Save();
    Hook_ResetTriggerState();   // eski triggerning "bosilgan" holati qolmasin

    u->hkApplied = true;
    UINT busy = Hotkeys_Apply();
    if (busy) WarnBusyHotkeys(u, busy);     // saqlash baribir bo'lgan
    return true;
}

static INT_PTR CALLBACK SettingsProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    SetUi* u = (SetUi*)GetWindowLongPtrW(h, DWLP_USER);

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        u = (SetUi*)lp;
        u->dlg = h;
        SetWindowLongPtrW(h, DWLP_USER, lp);

        SetWindowTextW(h, T(S_SET_TITLE));
        SetDlgItemTextW(h, IDOK,     T(S_SET_SAVE));
        SetDlgItemTextW(h, IDCANCEL, T(S_SET_CANCEL));

        // tab sarlavhalari
        u->tab = GetDlgItem(h, IDC_SET_TAB);
        const StrId tabNames[PG_COUNT] = { S_TAB_GENERAL, S_TAB_MODE, S_TAB_HOTKEYS, S_TAB_EXCLUDE };
        for (int i = 0; i < PG_COUNT; ++i) {
            TCITEMW ti = {};
            ti.mask    = TCIF_TEXT;
            ti.pszText = (LPWSTR)T(tabNames[i]);
            SendMessageW(u->tab, TCM_INSERTITEMW, i, (LPARAM)&ti);
        }

        // sahifalar (tab'ning ustida turadi; faqat tanlangani ko'rinadi)
        const int pageIds[PG_COUNT] = { IDD_SET_GENERAL, IDD_SET_MODE, IDD_SET_HOTKEYS, IDD_SET_EXCLUDE };
        for (int i = 0; i < PG_COUNT; ++i) {
            u->pg[i] = CreateDialogParamW(g_hInst, MAKEINTRESOURCEW(pageIds[i]), h, PageProc, (LPARAM)u);
            if (!u->pg[i]) { EndDialog(h, 0); return TRUE; }
        }
        ThemeTabPages(u);

        InitGeneral(u);
        InitMode(u);
        InitHotkeys(u);
        InitExclude(u);

        TabCtrl_SetCurSel(u->tab, PG_GENERAL);
        LayoutPages(u);
        SetFocus(u->tab);
        return FALSE;               // fokusni o'zimiz qo'ydik
    }

    case WM_NOTIFY:
        if (u && ((NMHDR*)lp)->idFrom == IDC_SET_TAB && ((NMHDR*)lp)->code == TCN_SELCHANGE)
            SelectPage(u, TabCtrl_GetCurSel(u->tab));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDOK:
            if (!u) return TRUE;
            // Istisnolar maydonida Enter = "Qo'shish" (dialogni yopmaydi)
            if (GetFocus() == Item(u, PG_EXCLUDE, IDC_SET_EX_EDIT)) {
                ExcludeAddFromEdit(u);
                return TRUE;
            }
            if (SaveAll(u)) EndDialog(h, 1);
            return TRUE;

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
    SetUi ui = {};
    // Hotkey control kombinatsiyani ushlay olishi uchun tizim hotkeylari vaqtincha bo'shatiladi
    Hotkeys_Unregister();
    DialogBoxParamW(g_hInst, MAKEINTRESOURCEW(IDD_SETTINGS), parent, SettingsProc, (LPARAM)&ui);
    // Bekor / X / xato — saqlashda allaqachon qayta ro'yxatdan o'tgan bo'lmasa
    if (!ui.hkApplied) Hotkeys_Apply();
}
