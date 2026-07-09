// Litsenziya / ogohlantirish oynasi. Birinchi ishga tushishda va "Dastur haqida"
// dan ochiladi. Matn joriy interfeys tilida (uz/ru/en) ko'rsatiladi.
#include "app.h"
#include <shellapi.h>

static const wchar_t* kGitHub = L"https://github.com/HusanboyUmarov/Harfbek";

static INT_PTR CALLBACK LicenseProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowTextW(h, T(S_LIC_TITLE));
        SetDlgItemTextW(h, IDC_LIC_TEXT,   T(S_LIC_BODY));
        SetDlgItemTextW(h, IDOK,           T(S_LIC_OK));
        SetDlgItemTextW(h, IDC_LIC_GITHUB, T(S_ABOUT_GITHUB));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDC_LIC_GITHUB:
            ShellExecuteW(h, L"open", kGitHub, NULL, NULL, SW_SHOWNORMAL);
            return TRUE;
        case IDOK:
        case IDCANCEL:
            EndDialog(h, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowLicense(HWND parent)
{
    DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_LICENSE), parent, LicenseProc);
}
