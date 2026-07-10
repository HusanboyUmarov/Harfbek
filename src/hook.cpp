// Global low-level klaviatura hook: trigger + harf -> maxsus o'zbek harfi.
#include "app.h"
#include "config.h"

static HHOOK           g_hook = NULL;       // klaviatura hook
static HHOOK           g_mouseHook = NULL;  // sichqoncha hook (Ctrl+o'ng-tugma)
static const ULONG_PTR kInjectSig = 0x555A424B; // "UZBK" - o'zimiz yuborgan input belgisi

// Oxirgi marta modifikator "qayta bosilgan" vaqt (stuck-key tekshiruvi uchun).
static volatile DWORD g_repressTick = 0;

static inline bool IsDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

// --- Modifikator bo'lmagan trigger (masalan VK_APPS, Caps Lock, F-tugmalar) ---
// Bunday tugmalar yolg'iz bosilganda ham o'z vazifasini bajaradi (VK_APPS
// kontekst menyu ochadi, Caps holatni almashtiradi...). Shuning uchun ularni
// hook'da TO'LIQ yutamiz. Yutilgan tugmani GetAsyncKeyState ko'rmaydi,
// shuning uchun bosib turilganini o'zimiz kuzatamiz.
static volatile bool g_trigHeld = false;  // modifikator bo'lmagan trigger bosib turilibdimi

// Alt/Ctrl/Shift oilasi: xavfsiz modifikatorlar — yutilmaydi,
// holati GetAsyncKeyState bilan tekshiriladi. (Sozlamalar oynasi ham
// ogohlantirish kerak-kerakmasligini shu bilan aniqlaydi.)
bool Trigger_IsModifier(UINT vk)
{
    switch (vk) {
    case VK_MENU:    case VK_LMENU:    case VK_RMENU:
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
    case VK_SHIFT:   case VK_LSHIFT:   case VK_RSHIFT:
        return true;
    }
    return false;
}

// Trigger o'zgarganda (Sozlamalar saqlanganda) eski kuzatuv holatini tozalaydi —
// aks holda oldingi tugmaning "bosib turilgan" holati yangi triggerga o'tib qoladi.
void Hook_ResetTriggerState()
{
    g_trigHeld = false;
}

// Modifikator uchun to'g'ri scancode (chap/o'ng farqi extended bayroqda).
static WORD ModScan(WORD vk)
{
    switch (vk) {
    case VK_LMENU:    case VK_RMENU:    return 0x38;
    case VK_LCONTROL: case VK_RCONTROL: return 0x1D;
    default: return (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    }
}

// Bitta INPUT ni modifikator bosish/qo'yib yuborish uchun to'ldiradi.
static void FillMod(INPUT& in, WORD vk, bool up)
{
    in = INPUT{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.wScan = ModScan(vk);
    in.ki.dwFlags = (up ? KEYEVENTF_KEYUP : 0);
    if (vk == VK_RMENU || vk == VK_RCONTROL)   // o'ng modifikatorlar "extended"
        in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    in.ki.dwExtraInfo = kInjectSig;
}

// Trigger tugma bosilganmi. VK_MENU/VK_CONTROL/VK_SHIFT uchun chap va o'ng
// tomonlarni ALOHIDA tekshiramiz (ba'zi tizimlarda umumiy VK chap tomonni
// ishonchli qaytarmaydi).
static bool TriggerDown()
{
    switch (g_settings.triggerVk) {
    case VK_MENU:    return IsDown(VK_LMENU)    || IsDown(VK_RMENU);
    case VK_CONTROL: return IsDown(VK_LCONTROL) || IsDown(VK_RCONTROL);
    case VK_SHIFT:   return IsDown(VK_LSHIFT)   || IsDown(VK_RSHIFT);
    default:
        if (Trigger_IsModifier(g_settings.triggerVk))
            return IsDown(g_settings.triggerVk);
        return g_trigHeld;   // yutilgan tugma — holatini o'zimiz kuzatamiz
    }
}

// "Niqob" tugma (vk 0xE8 — Windows'da tayinlanmagan, hech narsa qilmaydi):
// Alt bosilib-qo'yilgani orasida boshqa tugma "bosilgan"dek ko'rsatadi.
// Busiz Windows bizning Alt ko'tarish/qayta-bosishimizni "Alt yolg'iz
// bosildi" deb tushunib, oynaning menyu qatorini faollashtiradi (klaviatura
// "yozmay qoladi") yoki Alt+Shift / Ctrl+Shift til almashtirishni ishga
// tushiradi. Diqqat: SendInput wVk uchun 1..254 oraliqni talab qiladi —
// 0xFF ISHLATMANG, butun paket rad etiladi.
static void FillMask(INPUT& in, bool up)
{
    in = INPUT{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = 0xE8;
    in.ki.dwFlags = (up ? KEYEVENTF_KEYUP : 0);
    in.ki.dwExtraInfo = kInjectSig;
}

// Maxsus harfni (Unicode) klaviaturaga yuborish.
// Muhim: Alt/Ctrl bosilgan bo'lsa, ular menyu yoki yorliq (shortcut) rejimini
// yoqib harfni "yutib" yubormasligi uchun ularni vaqtincha qo'yib turamiz,
// harfni yuboramiz, so'ng qayta bosamiz (shunda async holat saqlanadi va
// keyingi harflar ham ishlaydi).
static void SendUnicode(wchar_t ch)
{
    const WORD mods[] = { VK_LMENU, VK_RMENU, VK_LCONTROL, VK_RCONTROL };
    WORD down[4]; int n = 0;
    for (int i = 0; i < 4; ++i) if (IsDown(mods[i])) down[n++] = mods[i];

    // Butun ketma-ketlikni BITTA SendInput'da yuboramiz (atomik): bu tartib
    // buzilishi va hook'ka ko'p qayta kirishning oldini oladi.
    INPUT seq[2 + 4 + 2 + 4 + 2];
    int m = 0;

    // 0) niqob: Alt ko'tarilishidan oldin "boshqa tugma bosildi" belgisi
    if (n > 0) { FillMask(seq[m++], false); FillMask(seq[m++], true); }

    // 1) bosilgan modifikatorlarni ko'taramiz
    for (int i = 0; i < n; ++i) FillMod(seq[m++], down[i], true);

    // 2) harfni (Unicode) yuboramiz (wVk = 0)
    seq[m] = INPUT{}; seq[m].type = INPUT_KEYBOARD;
    seq[m].ki.wScan = ch; seq[m].ki.dwFlags = KEYEVENTF_UNICODE;
    seq[m].ki.dwExtraInfo = kInjectSig; m++;
    seq[m] = INPUT{}; seq[m].type = INPUT_KEYBOARD;
    seq[m].ki.wScan = ch; seq[m].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    seq[m].ki.dwExtraInfo = kInjectSig; m++;

    // 3) modifikatorlarni qayta bosamiz (teskari tartibda)
    for (int i = n - 1; i >= 0; --i) FillMod(seq[m++], down[i], false);

    // 4) niqob: qayta bosilgan Alt keyin fizik qo'yib yuborilganda ham
    //    "yolg'iz Alt" deb hisoblanmasligi uchun
    if (n > 0) { FillMask(seq[m++], false); FillMask(seq[m++], true); }

    // Modifikatorni qayta bosamiz — haqiqiy keyup bizning qayta-bosishdan
    // OLDIN kelib qolsa (tez qo'yib yuborilganda), stuck-tekshiruv (pastdagi
    // WM_APP_MODCHECK) buni tuzatishi uchun vaqtni SendInput'dan oldin belgilaymiz.
    if (n > 0) g_repressTick = GetTickCount();

    SendInput(m, seq, sizeof(INPUT));
}

// "Osilib qolgan" modifikatorni majburan qo'yib yuboradi (main thread'dan chaqiriladi).
void Hook_ForceModUp(UINT vk)
{
    INPUT in;
    FillMod(in, (WORD)vk, true);
    SendInput(1, &in, sizeof(in));
}

static inline bool IsMod(UINT vk)
{
    return vk == VK_LMENU || vk == VK_RMENU ||
           vk == VK_LCONTROL || vk == VK_RCONTROL;
}

static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION &&
        (wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
    {
        // Stuck-key himoyasi: real modifikator qo'yib yuborilganda, agar
        // yaqinda harf yuborgan bo'lsak, u "osilib qolmaganini" tekshiramiz.
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
        bool injected = (k->flags & LLKHF_INJECTED) || (k->dwExtraInfo == kInjectSig);
        if (!injected && IsMod(k->vkCode) &&
            (GetTickCount() - g_repressTick) < 1000 && g_hWnd)
        {
            PostMessageW(g_hWnd, WM_APP_MODCHECK, (WPARAM)k->vkCode, 0);
        }

        // Modifikator bo'lmagan trigger qo'yib yuborildi: bosilishini yutgan
        // bo'lsak, qo'yib yuborilishini ham yutamiz.
        if (!injected && !Trigger_IsModifier(g_settings.triggerVk) &&
            k->vkCode == g_settings.triggerVk)
        {
            bool wasHeld = g_trigHeld;
            g_trigHeld = false;
            if (wasHeld) return 1;
        }
    }

    if (nCode == HC_ACTION &&
        (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
        bool injected = (k->flags & LLKHF_INJECTED) || (k->dwExtraInfo == kInjectSig);

        // Modifikator bo'lmagan trigger tugmasining O'ZINI yutamiz: masalan,
        // VK_APPS (menyu tugmasi) trigger bo'lsa, har bosilganda kontekst
        // menyu ochilib ketmasligi kerak. Holatini g_trigHeld da kuzatamiz.
        if (g_settings.enabled && !injected &&
            !Trigger_IsModifier(g_settings.triggerVk) &&
            k->vkCode == g_settings.triggerVk)
        {
            g_trigHeld = true;
            return 1;
        }

        if (g_settings.enabled && !injected && TriggerDown())
        {
            for (int i = 0; i < kMapCount; ++i)
            {
                if (kMaps[i].vk == k->vkCode)
                {
                    // Trigger Shift oilasidan bo'lsa, uni katta harf belgisi deb
                    // hisoblamaymiz (aks holda hamma harf katta chiqadi) —
                    // faqat QARSHI tomondagi Shift katta harf beradi.
                    bool shift;
                    switch (g_settings.triggerVk) {
                    case VK_SHIFT:  shift = false; break;
                    case VK_LSHIFT: shift = IsDown(VK_RSHIFT); break;
                    case VK_RSHIFT: shift = IsDown(VK_LSHIFT); break;
                    default:        shift = IsDown(VK_SHIFT);  break;
                    }
                    bool caps  = (GetKeyState(VK_CAPITAL) & 1) != 0;
                    bool upper = shift ^ caps;
                    SendUnicode(upper ? kMaps[i].upper : kMaps[i].lower);
                    return 1;   // asl klavishani bloklaymiz
                }
            }
        }
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

// Sichqoncha hook: Ctrl + o'ng-tugma bosilganda dastur o'girish menyusini
// ko'rsatadi (haqiqiy o'ng-tugma menyusini "yutamiz").
static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && g_settings.enabled &&
        (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP))
    {
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        // AltGr'li layoutlarda o'ng Alt bosilganda Windows soxta LCtrl ham
        // bosadi — bu holda foydalanuvchi Ctrl bosgani yo'q, e'tiborga olmaymiz.
        if (ctrl && IsDown(VK_RMENU)) ctrl = false;
        if (ctrl) {
            if (wParam == WM_RBUTTONUP && g_hWnd) {
                MSLLHOOKSTRUCT* m = (MSLLHOOKSTRUCT*)lParam;
                PostMessageW(g_hWnd, WM_APP_CONVERTMENU,
                             (WPARAM)m->pt.x, (LPARAM)m->pt.y);
            }
            return 1;   // Ctrl+o'ng-tugmani bloklaymiz (o'zimizning menyu uchun)
        }
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

bool Hook_Install()
{
    if (!g_hook)
        g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, g_hInst, 0);
    if (!g_mouseHook)
        g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseProc, g_hInst, 0);
    return g_hook != NULL;
}

void Hook_Uninstall()
{
    if (g_hook)      { UnhookWindowsHookEx(g_hook);      g_hook = NULL; }
    if (g_mouseHook) { UnhookWindowsHookEx(g_mouseHook); g_mouseHook = NULL; }
}
