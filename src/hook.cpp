// Global low-level klaviatura hook: trigger + harf -> maxsus o'zbek harfi.
//
// Hook'lar ALOHIDA thread'da ishlaydi (Hook_Install). Sabab: low-level hook
// callback'i o'rnatgan thread'ning xabar siklida chaqiriladi — main thread
// modal dialog, menyu yoki ShellExecute bilan band bo'lsa, butun tizim
// klaviaturasi kechikadi, LowLevelHooksTimeout oshsa Windows hook'ni jimgina
// olib tashlaydi.
//
// Thread'lar orasidagi holat:
//  - Quyidagi g_* holat o'zgaruvchilari (g_trigHeld, g_fakeLCtrl,
//    g_repressTick, g_trigModHeld, g_mateHeld, g_rDownEaten) FAQAT hook
//    thread'ida o'qiladi/yoziladi; tashqaridan tozalash ham navbat orqali shu
//    thread'da bajariladi (Hook_ResetTriggerState). volatile — kompilyator
//    keshlamasligi uchun qoldirilgan; kichik skalyarlar (bool/DWORD) x86/x64 da
//    atomik o'qiladi/yoziladi, murakkab invariant yo'q — qulf kerak emas.
//  - g_settings ni main thread yozadi (Sozlamalar, tray menyu), hook thread
//    faqat o'qiydi. Maydonlar UINT/bool — atomik; bitta callback ichida eski va
//    yangi qiymat aralashib qolishi zararsiz (eng yomoni: o'sha bitta tugma eski
//    trigger bilan ishlanadi).
//  - g_hWnd hook thread yaratilishidan OLDIN o'rnatiladi va o'zgarmaydi.
//  - Avto rejim holati (g_auto, g_autoFg...) ham FAQAT hook thread'iniki:
//    klaviatura va sichqoncha callback'lari bir thread'da ketma-ket chaqiriladi,
//    tashqi tozalash esa WM_HOOK_RESET orqali keladi.
#include "app.h"
#include "config.h"
#include "autoconv.h"

static HHOOK           g_hook = NULL;       // klaviatura hook
static HHOOK           g_mouseHook = NULL;  // sichqoncha hook (Ctrl+o'ng-tugma)
static HANDLE          g_hookThread = NULL; // hook'lar ishlaydigan thread
static DWORD           g_hookTid = 0;
static const ULONG_PTR kInjectSig = 0x555A424B; // "UZBK" - o'zimiz yuborgan input belgisi

// Hook thread'iga: trigger kuzatuv holatini tozalash (thread xabari, hwnd = NULL)
#define WM_HOOK_RESET  (WM_APP + 10)

// Sinov rejimi: -DHARFBEK_TEST_SIG bilan qurilganda, quyidagi imzoli inyeksiya
// hodisalari "haqiqiy" (fizik) deb qabul qilinadi — sinov stendi (racetest)
// fizik klaviaturani taqlid qila olishi uchun. Oddiy build'ga ta'siri yo'q.
#ifdef HARFBEK_TEST_SIG
static const ULONG_PTR kTestSig = 0x54534554; // "TEST"
#endif
static inline bool IsInjected(const KBDLLHOOKSTRUCT* k)
{
    bool inj = (k->flags & LLKHF_INJECTED) || (k->dwExtraInfo == kInjectSig);
#ifdef HARFBEK_TEST_SIG
    if (k->dwExtraInfo == kTestSig) inj = false;
#endif
    return inj;
}

// Oxirgi marta modifikator "qayta bosilgan" vaqt (stuck-key tekshiruvi uchun).
static volatile DWORD g_repressTick = 0;

static inline bool IsDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

static void Hook_ResetGuardState();   // pastda (himoya-tap holati)
static void AutoResetAll();           // pastda (avto rejim holati)

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

static void ResetTriggerStateNow()
{
    g_trigHeld = false;
    Hook_ResetGuardState();
    AutoResetAll();
}

// Trigger yoki rejim o'zgarganda (Sozlamalar saqlanganda, tray'dan rejim
// almashganda) eski kuzatuv holatini tozalaydi — aks holda oldingi tugmaning
// "bosib turilgan" holati yangi triggerga, avto buferdagi belgi esa yangi
// rejimga o'tib qoladi.
// Holat hook thread'iniki: tozalashni o'sha thread'ga navbat orqali beramiz,
// shunda u hook callback'lari bilan poygaga kirmaydi.
void Hook_ResetTriggerState()
{
    if (g_hookTid && PostThreadMessageW(g_hookTid, WM_HOOK_RESET, 0, 0))
        return;
    ResetTriggerStateNow();   // hook ishlamayapti — to'g'ridan-to'g'ri
}

// Modifikator uchun to'g'ri scancode (chap/o'ng farqi extended bayroqda).
// Faqat IsMod() tugmalari (L/R Alt, L/R Ctrl) uchun chaqiriladi.
static WORD ModScan(WORD vk)
{
    return (vk == VK_LMENU || vk == VK_RMENU) ? 0x38 : 0x1D;   // Alt : Ctrl
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

// --- Soxta AltGr-LCtrl kuzatuvi ---
// AltGr'li layoutlarda (masalan US-International) o'ng Alt bosilganda tizim
// u bilan birga "soxta" LCtrl ham yuboradi (scancode 0x21D) va uni O'ZI
// boshqaradi: bizning RMENU ko'tarishimizda o'zi ko'taradi, qayta
// bosishimizda o'zi qayta bosadi. SendUnicode bu Ctrl'ga TEGMASLIGI shart:
// biz uni o'zimiz qayta bossak (oddiy scancode 0x1D bilan), tizim "egalikni"
// yo'qotadi — foydalanuvchi AltGr'ni qo'yib yuborganda LCtrl ko'tarilmay
// ABADIY bosiq qolib qoladi (har harf Ctrl+harf bo'lib: Ctrl+W oynalarni
// yopadi, Ctrl+Shift tilni almashtiradi...). Jonli sinovda isbotlangan.
static volatile bool g_fakeLCtrl = false;

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
    for (int i = 0; i < 4; ++i) {
        // Soxta AltGr-LCtrl'ni tizimning o'ziga qoldiramiz (yuqoridagi izoh)
        if (mods[i] == VK_LCONTROL && g_fakeLCtrl) continue;
        if (IsDown(mods[i])) down[n++] = mods[i];
    }

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
// vk — WM_APP_MODCHECK dan keladi, u faqat IsMod() tugmalari uchun yuboriladi.
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

// =====================================================================
//  Til-almashtirish (Alt+Shift / Ctrl+Shift) va "yolg'iz Alt" menyu-rejimga
//  qarshi himoya.
//
//  Jonli tajribada isbotlangan faktlar (2026-07):
//   - Windows'da O'NG Alt + Shift ham tilni almashtiradi (sozlamada
//     "chap Alt+Shift" deyilsa ham) — RMENU trigger bundan qochib qutulmaydi.
//   - Hotkey modifikator QO'YIB YUBORILGANDA otiladi; kombinatsiya bosiq
//     paytida boshqa istalgan tugma bosilsa — bekor bo'ladi va autorepeat
//     uni qayta "qurollantirmaydi". Demak bitta niqob-tap yetarli.
//   - Xuddi shu qoida "yolg'iz Alt" menyu-rejimiga ham tegishli.
//
//  Muammo: biz trigger+harf ni yutganimizda Windows harfni KO'RMAYDI —
//  unga "toza" Alt(+Shift) bosilib-qo'yilgandek ko'rinadi. Natijada til
//  almashib ketadi yoki menyu qatori faollashib klaviatura "yozmay qoladi"
//  (S = Size/Свернуть kabi mnemonikalar oynalarni yashiradi). SendUnicode
//  ichidagi niqoblar kech qolishi mumkin: foydalanuvchi modifikatorni juda
//  tez qo'yib yuborsa, haqiqiy keyup bizning inyeksiyamizdan OLDIN navbatga
//  tushadi. Shuning uchun xavfli kombinatsiya SHAKLLANGAN zahoti (qo'yib
//  yuborishni kutmasdan) niqob-tap yuboramiz.
// =====================================================================

// Trigger oilasiga tegishli vk mi (umumiy VK uchun chap/o'ng variantlari bilan)
static bool IsTriggerVk(UINT vk)
{
    switch (g_settings.triggerVk) {
    case VK_MENU:    return vk == VK_LMENU    || vk == VK_RMENU;
    case VK_CONTROL: return vk == VK_LCONTROL || vk == VK_RCONTROL;
    case VK_SHIFT:   return vk == VK_LSHIFT   || vk == VK_RSHIFT;
    default:         return vk == (UINT)g_settings.triggerVk;
    }
}

static inline bool IsShiftVk(UINT vk)
{
    return vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT;
}

// Trigger bilan birga til-almashtirish kombinatsiyasini hosil qiladigan
// "sherik" tugma: Alt/Ctrl triggerlar uchun Shift, Shift trigger uchun Alt/Ctrl.
static bool IsArmMate(UINT vk)
{
    if (IsShiftVk(g_settings.triggerVk)) return IsMod(vk);
    return IsShiftVk(vk);
}

// Tap'ni har autorepeat'da qaytarmaslik uchun bosiqlik holati
static volatile bool g_trigModHeld = false;
static volatile bool g_mateHeld    = false;

static void Hook_ResetGuardState()
{
    g_trigModHeld = false;
    g_mateHeld    = false;
}

static void SendMaskTap()
{
    INPUT s[2];
    FillMask(s[0], false);
    FillMask(s[1], true);
    SendInput(2, s, sizeof(INPUT));
}

// =====================================================================
//  Avto rejim: eski yozuv (o' g' sh ch) yozilishi bilan ö ğ ş ç ga almashadi.
//  Qaror mantig'i — autoconv.cpp (sof, test qilinadi). Bu yerda faqat
//  kirish (tugma -> belgi) va chiqish (inyeksiya) bog'lanadi. Trigger
//  mantiqiga (niqob-tap, yutish, soxta LCtrl) umuman tegmaydi.
//
//  Belgini aniqlash: ToUnicode ISHLATILMAYDI — LL hook ichida u foreground
//  ilovaning dead-key holatini buzadi. O'rniga foreground thread layout'i
//  bilan MapVirtualKeyExW(MAPVK_VK_TO_CHAR): harf tugmasi uchun katta lotin
//  harf qaytaradi (kirill layout'da kirill — demak avto rejim ishlamaydi,
//  bu to'g'ri), registrni Shift ^ Caps dan o'zimiz hisoblaymiz. Xabar
//  yubormaydi va tez — yuqori prioritetli hook thread'i uchun mos.
// =====================================================================
static AutoState g_auto       = {};     // oxirgi belgi va undo ma'lumoti
static HWND      g_autoFg     = NULL;   // oxirgi foreground oyna (almashsa holat tozalanadi)
static DWORD     g_autoFgTid  = 0;      // uning thread'i (klaviatura layout'i shu thread'niki)
static bool      g_autoFgTerm = false;  // u terminalmi (har tugmada GetClassName shart emas)

static bool IsTerminalWindow(HWND w);   // pastda (Ctrl+o'ng-tugma bilan umumiy ro'yxat)

static void AutoResetAll()
{
    Auto_Reset(g_auto);
    g_autoFg = NULL;                    // keyingi tugmada foreground qayta tekshiriladi
}

// Kursor joyini o'zgartiradigan / tahrir tugmalari — oldingi belgi endi
// kursor oldida emas, holat tozalanadi.
static bool IsNavKey(UINT vk)
{
    switch (vk) {
    case VK_LEFT: case VK_RIGHT: case VK_UP:    case VK_DOWN:
    case VK_HOME: case VK_END:   case VK_PRIOR: case VK_NEXT:
    case VK_RETURN: case VK_TAB: case VK_ESCAPE:
    case VK_DELETE: case VK_INSERT:
        return true;
    }
    return false;
}

// Bitta Backspace (oldingi belgini o'chiradi) + text (1-2 belgi) ni BITTA
// SendInput'da yuboradi (atomik; o'zimizning imzo bilan — hook ularni qayta
// ishlamaydi).
static void AutoSend(const wchar_t* text, int len)
{
    INPUT seq[2 + 2 * 2];
    int m = 0;
    for (int up = 0; up < 2; ++up) {
        seq[m] = INPUT{}; seq[m].type = INPUT_KEYBOARD;
        seq[m].ki.wVk = VK_BACK; seq[m].ki.wScan = 0x0E;
        seq[m].ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
        seq[m].ki.dwExtraInfo = kInjectSig; m++;
    }
    for (int i = 0; i < len && i < 2; ++i) {
        for (int up = 0; up < 2; ++up) {
            seq[m] = INPUT{}; seq[m].type = INPUT_KEYBOARD;
            seq[m].ki.wScan = text[i];
            seq[m].ki.dwFlags = KEYEVENTF_UNICODE | (up ? KEYEVENTF_KEYUP : 0);
            seq[m].ki.dwExtraInfo = kInjectSig; m++;
        }
    }
    SendInput(m, seq, sizeof(INPUT));
}

// Avto rejimda tugma bosilishi. true — tugma yutiladi (o'rniga inyeksiya qilindi).
static bool AutoKeyDown(const KBDLLHOOKSTRUCT* k)
{
    UINT vk = k->vkCode;

    // O'zimizning inyeksiyamiz (almashtirish, undo) — holatga tegmaydi
    if (k->dwExtraInfo == kInjectSig) return false;
    // Boshqa dastur yozgan — hujjatdagi matn endi noma'lum
    if (IsInjected(k)) { Auto_Reset(g_auto); return false; }

    // Shift / Caps faqat registrni belgilaydi — holatni buzmaydi (SH yozish uchun)
    if (IsShiftVk(vk) || vk == VK_CAPITAL) return false;

    // Ctrl/Alt/Win (AltGr ham — u soxta LCtrl + RAlt) — bu yozuv emas, yorliq
    if (IsDown(VK_CONTROL) || IsDown(VK_MENU) || IsDown(VK_LWIN) || IsDown(VK_RWIN)) {
        Auto_Reset(g_auto);
        return false;
    }

    // Foreground oyna almashdi — holat tozalanadi, terminal/thread qayta keshlanadi
    HWND fg = GetForegroundWindow();
    if (fg != g_autoFg) {
        Auto_Reset(g_auto);
        g_autoFg     = fg;
        g_autoFgTid  = fg ? GetWindowThreadProcessId(fg, NULL) : 0;
        g_autoFgTerm = IsTerminalWindow(fg);
    }
    if (!fg || g_autoFgTerm) return false;   // terminalda avto rejim ishlamaydi

    if (vk == VK_BACK) {
        AutoAction a = Auto_OnBackspace(g_auto);
        if (a.kind != AUTO_UNDO) return false;   // oddiy Backspace
        AutoSend(a.text, a.len);                 // ş -> sh
        return true;
    }
    if (IsNavKey(vk)) { Auto_Reset(g_auto); return false; }

    // Shift'siz belgi; yuqori bit — dead-key belgisi, olib tashlaymiz.
    // 0 — belgisiz tugma (F1, media...) -> holat tozalanadi.
    UINT    r = MapVirtualKeyExW(vk, MAPVK_VK_TO_CHAR, GetKeyboardLayout(g_autoFgTid));
    wchar_t c = (wchar_t)(r & 0x7FFFFFFF);
    bool shift = IsDown(VK_SHIFT);
    wchar_t ch = 0;
    if (c >= L'a' && c <= L'z') c = (wchar_t)(c - L'a' + L'A');
    if (c >= L'A' && c <= L'Z') {
        // Registr: Shift ^ Caps (GetKeyState toggle biti — trigger rejimi ham shu
        // usulda ishlaydi, hook thread'ida ham tizim toggle holatini beradi).
        bool caps = (GetKeyState(VK_CAPITAL) & 1) != 0;
        ch = (shift ^ caps) ? c : (wchar_t)(c - L'A' + L'a');
    } else if (!shift && Auto_IsApostrophe(c)) {
        ch = c;                                 // Shift bilan (US: ") apostrof emas
    }

    AutoAction a = Auto_OnChar(g_auto, ch);
    if (a.kind != AUTO_REPLACE) return false;
    AutoSend(a.text, a.len);                    // oldingi harfni o'chirib, maxsus harfni yozamiz
    return true;                                // apostrof / h tugmasining o'zi yutiladi
}

static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION &&
        (wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
    {
        // Stuck-key himoyasi: real modifikator qo'yib yuborilganda, agar
        // yaqinda harf yuborgan bo'lsak, u "osilib qolmaganini" tekshiramiz.
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
        bool injected = IsInjected(k);

        // Soxta AltGr-LCtrl ko'tarildi (inyeksiya bo'lsa ham kuzatamiz)
        if (k->vkCode == VK_LCONTROL)
            g_fakeLCtrl = false;

        if (!injected && IsMod(k->vkCode) &&
            (GetTickCount() - g_repressTick) < 1000 && g_hWnd)
        {
            PostMessageW(g_hWnd, WM_APP_MODCHECK, (WPARAM)k->vkCode, 0);
        }

        // Quyidagilar faqat trigger rejimiga tegishli (avto rejimda trigger
        // mantiqiga kirmaymiz; rejim almashganda holat baribir tozalanadi).
        if (g_settings.mode == MODE_TRIGGER) {
            // Himoya-tap dedup holatini yangilaymiz (bosiqlik tugadi)
            if (!injected) {
                if (IsTriggerVk(k->vkCode)) g_trigModHeld = false;
                if (IsArmMate(k->vkCode))   g_mateHeld    = false;
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
    }

    if (nCode == HC_ACTION &&
        (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
        bool injected = IsInjected(k);

        // Soxta AltGr-LCtrl bosildi? (scancode 0x21D — tizim sintez qilgan;
        // oddiy 0x1D bo'lsa bu haqiqiy/oddiy Ctrl.) Inyeksiya bo'lsa ham kuzatamiz.
        if (k->vkCode == VK_LCONTROL)
            g_fakeLCtrl = (k->scanCode == 0x21D);

        // Avto rejim: trigger+harf, trigger yutish va himoya-tap ishlamaydi.
        if (g_settings.mode == MODE_AUTO) {
            if (!g_settings.enabled) Auto_Reset(g_auto);   // o'chiq paytdagi matn noma'lum
            else if (AutoKeyDown(k)) return 1;
            return CallNextHookEx(g_hook, nCode, wParam, lParam);
        }

        // Himoya-tap: trigger-modifikator bosilganda va trigger bosiq turganda
        // "sherik" (Shift) bosilganda darhol niqob yuboramiz — til-almashtirish
        // hotkey'i va "yolg'iz Alt" menyu-rejimi shu zahoti zararsizlanadi.
        // (Tap bizning inyeksiyamiz, keyingi hodisalarga xalaqit bermaydi.)
        if (g_settings.enabled && !injected &&
            Trigger_IsModifier(g_settings.triggerVk))
        {
            if (IsTriggerVk(k->vkCode)) {
                if (!g_trigModHeld) { g_trigModHeld = true; SendMaskTap(); }
            } else if (IsArmMate(k->vkCode) && TriggerDown()) {
                if (!g_mateHeld)    { g_mateHeld = true;    SendMaskTap(); }
            }
        }

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
                    // Trigger o'ng Shift bo'lsa, uni katta harf belgisi deb
                    // hisoblamaymiz (aks holda hamma harf katta chiqadi) —
                    // faqat chap Shift katta harf beradi. (Shift oilasidan
                    // boshqa variant ruxsat etilgan triggerlar ro'yxatida yo'q.)
                    bool shift = (g_settings.triggerVk == VK_RSHIFT)
                                     ? IsDown(VK_LSHIFT) : IsDown(VK_SHIFT);
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

// Ctrl+o'ng-tugma USHLANMAYDIGAN terminal oynalari (oyna sinf nomlari).
// Terminalda bizning Ctrl+C inyeksiyamiz nusxalash emas, SIGINT (ishlab
// turgan buyruqni to'xtatish) bo'lib qoladi; terminalning o'z Ctrl+o'ng-tugma
// xatti-harakati ham saqlansin; avto rejim ham terminalda ishlamaydi.
// Ro'yxat faqat shu yerda. Alacritty ("Window Class") va VS Code terminali
// (Chrome_WidgetWin_1) umumiy sinf nomlari — ularni qo'shib bo'lmaydi
// (oddiy ilovalar ham ushlanmay qoladi).
static const wchar_t* const kTerminalClasses[] = {
    L"ConsoleWindowClass",              // cmd / PowerShell (conhost)
    L"CASCADIA_HOSTING_WINDOW_CLASS",   // Windows Terminal
    L"mintty",                          // Git Bash, MSYS2, Cygwin
    L"VirtualConsoleClass",             // ConEmu / Cmder
    L"PuTTY",                           // PuTTY
    L"KiTTY",                           // KiTTY
    L"org.wezfurlong.wezterm",          // WezTerm (standart sinf)
};

// GetClassNameW xabar yubormaydi — hook ichida chaqirish xavfsiz (bloklanmaydi).
static bool IsTerminalWindow(HWND w)
{
    wchar_t cls[64];
    if (!w || !GetClassNameW(w, cls, 64)) return false;
    for (int i = 0; i < (int)(sizeof(kTerminalClasses) / sizeof(kTerminalClasses[0])); ++i)
        if (lstrcmpiW(cls, kTerminalClasses[i]) == 0) return true;
    return false;
}

// Oxirgi o'ng-tugma DOWN ni yutdikmi. UP faqat shunga qarab yutiladi —
// ilovalar hech qachon juftsiz DOWN yoki UP olmaydi.
static volatile bool g_rDownEaten = false;

// Sichqoncha hook: Ctrl + o'ng-tugma bosilganda dastur o'girish menyusini
// ko'rsatadi (haqiqiy o'ng-tugma menyusini "yutamiz").
static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Avto rejim: istalgan tugma bosilishi kursorni boshqa joyga qo'yishi
    // mumkin — oldingi belgi endi kursor oldida emas. (Shu thread'da — poyga yo'q.)
    if (nCode == HC_ACTION &&
        (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN ||
         wParam == WM_MBUTTONDOWN || wParam == WM_XBUTTONDOWN))
        Auto_Reset(g_auto);

    if (nCode == HC_ACTION &&
        (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP))
    {
        MSLLHOOKSTRUCT* m = (MSLLHOOKSTRUCT*)lParam;
        if (!(m->flags & LLMHF_INJECTED)) {        // boshqa dasturlar inyeksiyasiga tegmaymiz
            if (wParam == WM_RBUTTONDOWN) {
                bool ctrl = IsDown(VK_CONTROL);
                // AltGr'li layoutlarda o'ng Alt bosilganda Windows soxta LCtrl ham
                // bosadi — bu holda foydalanuvchi Ctrl bosgani yo'q, e'tiborga olmaymiz.
                if (ctrl && IsDown(VK_RMENU)) ctrl = false;
                // Modal oynamiz ochiq bo'lsa (g_hWnd disabled) menyu ochilmaydi —
                // bosishni ilovaning o'ziga qoldiramiz.
                g_rDownEaten = ctrl && g_settings.enabled && g_hWnd &&
                               IsWindowEnabled(g_hWnd) &&
                               !IsTerminalWindow(GetForegroundWindow());
                if (g_rDownEaten) return 1;   // o'zimizning menyu uchun bloklaymiz
            } else if (g_rDownEaten) {
                // DOWN yutilgan edi — UP ham yutiladi va menyu shu yerda ochiladi
                // (Ctrl allaqachon qo'yib yuborilgan bo'lsa ham).
                g_rDownEaten = false;
                PostMessageW(g_hWnd, WM_APP_CONVERTMENU,
                             (WPARAM)m->pt.x, (LPARAM)m->pt.y);
                return 1;
            }
        }
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

// Hook thread'ini ishga tushirishda o'rnatish natijasini qaytarish uchun
struct HookStart {
    HANDLE ready;   // thread hook'larni o'rnatib bo'lgach signal beradi
    bool   ok;      // klaviatura hook'i o'rnatildimi
};

static DWORD WINAPI HookThreadProc(LPVOID param)
{
    HookStart* hs = (HookStart*)param;
    MSG msg;

    // Xabar navbatini oldindan yaratamiz — Hook_Uninstall/Hook_ResetTriggerState
    // dagi PostThreadMessageW yo'qolib ketmasin.
    PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // HIGHEST: callback qisqa (GetAsyncKeyState, SendInput, PostMessage —
    // hech narsa kutmaydi) va thread vaqtining deyarli hammasini GetMessage'da
    // uxlab o'tkazadi. Yuqori prioritet CPU to'la band bo'lganda ham tugmani
    // darhol ishlashga va LowLevelHooksTimeout (Windows hook'ni jimgina olib
    // tashlaydi) oshmasligiga yetarli. TIME_CRITICAL tavsiya etilmaydi:
    // tizimni muzlatish va priority inversion xavfi bor (scout hisoboti 04/5).
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    g_hook      = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, g_hInst, 0);
    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL,    MouseProc,    g_hInst, 0);
    bool ok = (g_hook != NULL);   // sichqoncha hook'i ixtiyoriy (faqat o'girish menyusi)
    hs->ok = ok;
    SetEvent(hs->ready);          // shundan keyin hs ga murojaat qilinmaydi

    if (ok) {
        while (GetMessageW(&msg, NULL, 0, 0) > 0) {
            if (msg.hwnd == NULL && msg.message == WM_HOOK_RESET)
                ResetTriggerStateNow();
        }
    }

    if (g_hook)      { UnhookWindowsHookEx(g_hook);      g_hook = NULL; }
    if (g_mouseHook) { UnhookWindowsHookEx(g_mouseHook); g_mouseHook = NULL; }
    return 0;
}

// Hook thread'ini yaratadi va hook'lar o'rnatilishini kutadi.
bool Hook_Install()
{
    if (g_hookThread) return true;

    HookStart hs = { CreateEventW(NULL, TRUE, FALSE, NULL), false };
    if (!hs.ready) return false;

    DWORD  tid = 0;
    HANDLE t = CreateThread(NULL, 0, HookThreadProc, &hs, 0, &tid);
    if (t) {
        // thread kutilmaganda tugab qolsa ham osilib qolmaymiz
        HANDLE w[2] = { hs.ready, t };
        WaitForMultipleObjects(2, w, FALSE, INFINITE);
    }
    CloseHandle(hs.ready);

    if (t && hs.ok) {
        g_hookThread = t;
        g_hookTid    = tid;
        return true;
    }
    if (t) { WaitForSingleObject(t, INFINITE); CloseHandle(t); }
    return false;
}

// Hook thread'ini to'xtatadi: thread o'zi hook'larni yechib chiqadi.
void Hook_Uninstall()
{
    if (!g_hookThread) return;
    PostThreadMessageW(g_hookTid, WM_QUIT, 0, 0);
    // Callback hech narsa kutmaydi — thread darhol tugaydi. Baribir cheksiz
    // kutmaymiz: jarayon tugashi bilan tizim hook'larni o'zi yechadi.
    WaitForSingleObject(g_hookThread, 2000);
    CloseHandle(g_hookThread);
    g_hookThread = NULL;
    g_hookTid    = 0;
}
