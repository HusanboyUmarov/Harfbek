// "Dastur haqida" oynasi: qisqa ma'lumot + LinkedIn havolasi.
#include "app.h"
#include <shellapi.h>

static const wchar_t* kLinkedIn = L"https://www.linkedin.com/in/ascoder/";
static const wchar_t* kGitHub   = L"https://github.com/HusanboyUmarov/Harfbek";

static INT_PTR CALLBACK AboutProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowTextW(h, T(S_ABOUT_TITLE));
        SetDlgItemTextW(h, IDC_ABOUT_DESC,   T(S_ABOUT_DESC));
        SetDlgItemTextW(h, IDC_ABOUT_DEV,    T(S_ABOUT_DEV));
        SetDlgItemTextW(h, IDC_ABOUT_OPEN,    T(S_ABOUT_OPEN));
        SetDlgItemTextW(h, IDC_ABOUT_GITHUB,  T(S_ABOUT_GITHUB));
        SetDlgItemTextW(h, IDC_ABOUT_LICENSE, T(S_LIC_TITLE));
        SetDlgItemTextW(h, IDCANCEL,          T(S_ABOUT_CLOSE));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDC_ABOUT_OPEN:
            ShellExecuteW(h, L"open", kLinkedIn, NULL, NULL, SW_SHOWNORMAL);
            return TRUE;
        case IDC_ABOUT_GITHUB:
            ShellExecuteW(h, L"open", kGitHub, NULL, NULL, SW_SHOWNORMAL);
            return TRUE;
        case IDC_ABOUT_LICENSE:
            ShowLicense(h);
            return TRUE;
        case IDCANCEL:
            EndDialog(h, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowAbout(HWND parent)
{
    DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_ABOUT), parent, AboutProc);
}
