// Belgilangan matnni o'girish: nusxalash (Ctrl+C) -> o'girish -> qaytarish (Ctrl+V).
// Ish alohida thread'da bajariladi, shunda hook thread'i (klaviatura) bloklanmaydi.
#include "app.h"
#include <string>
#include <cstring>
#include <cwctype>

// Maxsus harflar (hex — encoding'ga bog'liq emas):
//   ö=00F6 Ö=00D6  ğ=011F Ğ=011E  ş=015F Ş=015E  ç=00E7 Ç=00C7

// Apostrof variantlarini tanish (teskari o'girishda hammasini qabul qilamiz)
static bool IsApo(wchar_t c)
{
    return c == L'\'' || c == L'`' ||
           c == (wchar_t)0x02BB || c == (wchar_t)0x2018 ||
           c == (wchar_t)0x2019 || c == (wchar_t)0x00B4 ||
           c == (wchar_t)0x2032;
}

// Katta harf konteksti (Ş/Ç dan keyin SH/CH yoki Sh/Ch tanlash uchun)
static bool IsUpperCtx(wchar_t c)
{
    if (c >= L'A' && c <= L'Z') return true;
    if (c == 0x00D6 || c == 0x011E || c == 0x015E || c == 0x00C7) return true;
    return iswupper(c) != 0;
}

// ö ğ ş ç  ->  o' g' sh ch
static std::wstring ToDigraph(const std::wstring& in, wchar_t apo)
{
    std::wstring o;
    o.reserve(in.size() + 8);
    for (size_t i = 0; i < in.size(); ++i) {
        wchar_t c  = in[i];
        wchar_t nx = (i + 1 < in.size()) ? in[i + 1] : 0;
        switch (c) {
        case 0x00F6: o += L'o'; o += apo; break;                 // ö
        case 0x00D6: o += L'O'; o += apo; break;                 // Ö
        case 0x011F: o += L'g'; o += apo; break;                 // ğ
        case 0x011E: o += L'G'; o += apo; break;                 // Ğ
        case 0x015F: o += L"sh"; break;                          // ş
        case 0x015E: o += (IsUpperCtx(nx) ? L"SH" : L"Sh"); break;// Ş
        case 0x00E7: o += L"ch"; break;                          // ç
        case 0x00C7: o += (IsUpperCtx(nx) ? L"CH" : L"Ch"); break;// Ç
        default:     o += c;
        }
    }
    return o;
}

// o' g' sh ch  ->  ö ğ ş ç   (apostrofning barcha variantlari tan olinadi)
static std::wstring ToSpecial(const std::wstring& in)
{
    std::wstring o;
    o.reserve(in.size());
    size_t n = in.size();
    for (size_t i = 0; i < n; ) {
        wchar_t c = in[i];
        wchar_t d = (i + 1 < n) ? in[i + 1] : 0;
        if ((c == L'o' || c == L'O') && IsApo(d)) {
            o += (c == L'O') ? (wchar_t)0x00D6 : (wchar_t)0x00F6; i += 2; continue; }
        if ((c == L'g' || c == L'G') && IsApo(d)) {
            o += (c == L'G') ? (wchar_t)0x011E : (wchar_t)0x011F; i += 2; continue; }
        if ((c == L's' || c == L'S') && (d == L'h' || d == L'H')) {
            o += (c == L'S') ? (wchar_t)0x015E : (wchar_t)0x015F; i += 2; continue; }
        if ((c == L'c' || c == L'C') && (d == L'h' || d == L'H')) {
            o += (c == L'C') ? (wchar_t)0x00C7 : (wchar_t)0x00E7; i += 2; continue; }
        o += c; ++i;
    }
    return o;
}

// --- Clipboard yordamchilari ---
static std::wstring ClipGet()
{
    std::wstring r;
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return r;
    if (!OpenClipboard(NULL)) return r;
    HANDLE hd = GetClipboardData(CF_UNICODETEXT);
    if (hd) {
        const wchar_t* p = (const wchar_t*)GlobalLock(hd);
        if (p) { r = p; GlobalUnlock(hd); }
    }
    CloseClipboard();
    return r;
}

static bool ClipSet(const std::wstring& s)
{
    if (!OpenClipboard(NULL)) return false;
    EmptyClipboard();
    size_t  bytes = (s.size() + 1) * sizeof(wchar_t);
    HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, bytes);
    bool    ok = false;
    if (g) {
        void* p = GlobalLock(g);
        if (p) { memcpy(p, s.c_str(), bytes); GlobalUnlock(g); }
        ok = SetClipboardData(CF_UNICODETEXT, g) != NULL;
    }
    CloseClipboard();
    return ok;
}

// Ctrl + <vk> yuborish (masalan Ctrl+C / Ctrl+V). Injected — hook o'tkazib yuboradi.
static void SendCtrlKey(WORD vk)
{
    INPUT in[4] = {};
    for (int i = 0; i < 4; ++i) in[i].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_CONTROL;
    in[1].ki.wVk = vk;
    in[2].ki.wVk = vk;         in[2].ki.dwFlags = KEYEVENTF_KEYUP;
    in[3].ki.wVk = VK_CONTROL; in[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, in, sizeof(INPUT));
}

static DWORD WINAPI ConvThread(LPVOID param)
{
    bool toDigraph = (param != NULL);

    Sleep(40);   // target oyna fokusni olib bo'lsin

    std::wstring saved = ClipGet();              // asl clipboardni saqlaymiz
    DWORD seq = GetClipboardSequenceNumber();

    SendCtrlKey('C');                            // belgilangan matnni nusxalash

    // clipboard yangilanishini kutamiz (~500 ms)
    bool changed = false;
    for (int i = 0; i < 50; ++i) {
        Sleep(10);
        if (GetClipboardSequenceNumber() != seq) { changed = true; break; }
    }
    if (!changed) return 0;                       // hech narsa belgilanmagan edi

    std::wstring sel = ClipGet();
    if (sel.empty()) { if (!saved.empty()) ClipSet(saved); return 0; }

    std::wstring out = toDigraph ? ToDigraph(sel, g_settings.apo) : ToSpecial(sel);
    if (out == sel) { if (!saved.empty()) ClipSet(saved); return 0; }  // o'zgarish yo'q

    ClipSet(out);
    Sleep(30);
    SendCtrlKey('V');                             // o'girilgan matnni joyiga qo'yish

    if (!saved.empty()) { Sleep(150); ClipSet(saved); }  // asl clipboardni tiklaymiz
    return 0;
}

void TextConvert_Run(bool toDigraph)
{
    HANDLE t = CreateThread(NULL, 0, ConvThread,
                            (LPVOID)(INT_PTR)(toDigraph ? 1 : 0), 0, NULL);
    if (t) CloseHandle(t);
}
