// Til tanlash dialogi (birinchi ishga tushishda ko'rsatiladi)
#include "app.h"

static INT_PTR CALLBACK LangProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowTextW(h, T(S_LANG_TITLE));
        SetDlgItemTextW(h, IDC_LANG_PROMPT, T(S_LANG_PROMPT));
        // Til nomlari - o'z tilida (neytral)
        SetDlgItemTextW(h, IDC_LANG_UZ, L"Özbekça");
        SetDlgItemTextW(h, IDC_LANG_RU, L"Русский");
        SetDlgItemTextW(h, IDC_LANG_EN, L"English");
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDC_LANG_UZ: EndDialog(h, LANG_UZ); return TRUE;
        case IDC_LANG_RU: EndDialog(h, LANG_RU); return TRUE;
        case IDC_LANG_EN: EndDialog(h, LANG_EN); return TRUE;
        case IDCANCEL:    EndDialog(h, -1);      return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowLanguageDialog(HWND parent)
{
    INT_PTR r = DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_LANG), parent, LangProc);
    if (r >= 0 && r < LANG_COUNT) {
        g_settings.lang = (Lang)r;
        Settings_Save();
    }
}
