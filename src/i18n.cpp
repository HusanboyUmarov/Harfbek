#include "i18n.h"

// Har bir qator: { o'zbekcha, ruscha, inglizcha }  -  tartib StrId enum bilan bir xil.
// O'zbekcha matnlar YANGI ALIFBODA (ş ç ğ ö) yozilgan.
static const wchar_t* STR[S_STR_COUNT][LANG_COUNT] = {
    // --- umumiy ---
    /*S_APP_TITLE*/       { L"Harfbek", L"Harfbek", L"Harfbek" },
    // Har tilda aynan bitta %s bor: u trigger keycap nomi bilan to'ldiriladi (wsprintfW).
    /*S_TOOLTIP*/         { L"Harfbek — %s bilan özbek harflari",
                            L"Harfbek — узбекские буквы через %s",
                            L"Harfbek — Uzbek letters via %s" },
    /*S_ERROR*/           { L"Xato", L"Ошибка", L"Error" },
    /*S_ALREADY_RUNNING*/ { L"Dastur allaqaçon işlab turibdi.",
                            L"Программа уже запущена.",
                            L"The program is already running." },
    /*S_HOOK_FAILED*/     { L"Klaviatura hook örnatilmadi.",
                            L"Не удалось установить клавиатурный перехват.",
                            L"Failed to install the keyboard hook." },

    // --- tray menyu ---
    /*S_MENU_ENABLED*/    { L"Yoqilgan", L"Включено", L"Enabled" },
    /*S_MENU_SETTINGS*/   { L"Sozlamalar...", L"Настройки...", L"Settings..." },
    /*S_MENU_LANGUAGE*/   { L"Til", L"Язык", L"Language" },
    /*S_MENU_TUTORIAL*/   { L"Tanişuv...", L"Знакомство...", L"Getting started..." },
    /*S_MENU_ABOUT*/      { L"Dastur haqida...", L"О программе...", L"About..." },
    /*S_MENU_STARTUP*/    { L"Windows bilan işga tuşsin",
                            L"Запускать вместе с Windows",
                            L"Run at Windows startup" },
    /*S_MENU_EXIT*/       { L"Çiqiş", L"Выход", L"Exit" },

    // --- matnni o'girish menyusi ---
    /*S_CONV_TODIGRAPH*/  { L"ö ğ ş ç  →  o' g' sh ch",
                            L"ö ğ ş ç  →  o' g' sh ch",
                            L"ö ğ ş ç  →  o' g' sh ch" },
    /*S_CONV_TOSPECIAL*/  { L"o' g' sh ch  →  ö ğ ş ç",
                            L"o' g' sh ch  →  ö ğ ş ç",
                            L"o' g' sh ch  →  ö ğ ş ç" },

    // --- dastur haqida ---
    /*S_ABOUT_TITLE*/     { L"Dastur haqida", L"О программе", L"About" },
    /*S_ABOUT_DESC*/      { L"Harfbek — özbek maxsus harflarini (ö ğ ş ç) "
                            L"trigger tugma bilan oson yozish uçun kiçik dastur. "
                            L"Belgilangan matnni o' g' sh ch ↔ ö ğ ş ç ga ögiradi.",
                            L"Harfbek — небольшая программа для удобного ввода "
                            L"особых узбекских букв (ö ğ ş ç) через клавишу-триггер. "
                            L"Преобразует выделенный текст o' g' sh ch ↔ ö ğ ş ç.",
                            L"Harfbek — a small app for easily typing the special "
                            L"Uzbek letters (ö ğ ş ç) with a trigger key. "
                            L"Converts selected text o' g' sh ch ↔ ö ğ ş ç." },
    /*S_ABOUT_DEV*/       { L"Işlab çiquvçi: ascoder",
                            L"Разработчик: ascoder",
                            L"Developer: ascoder" },
    /*S_ABOUT_OPEN*/      { L"LinkedIn", L"LinkedIn", L"LinkedIn" },
    /*S_ABOUT_GITHUB*/    { L"GitHub", L"GitHub", L"GitHub" },
    /*S_ABOUT_CLOSE*/     { L"Yopiş", L"Закрыть", L"Close" },

    // --- litsenziya / ogohlantirish ---
    /*S_LIC_TITLE*/       { L"Litsenziya", L"Лицензия", L"License" },
    /*S_LIC_BODY*/        { L"Harfbek — erkin va oçiq dastur, MIT litsenziyasi ostida tarqatiladi.\r\n\r\n"
                            L"Dastur \"borligiça\" (AS IS), heç qanday kafolatsiz beriladi. Undan foydalaniş natijasida yuzaga kelgan har qanday zarar uçun muallif javobgar emas.\r\n\r\n"
                            L"Özgartirilgan yoki boşqa manbadan olingan versiyalar muallifga aloqador emas.\r\n\r\n"
                            L"© 2026 Husanboy Umarov (ascoder)\r\n"
                            L"Rasmiy manba: github.com/HusanboyUmarov/Harfbek\r\n"
                            L"Töliq matn: LICENSE fayli.",
                            L"Harfbek — свободная программа с открытым кодом, распространяется по лицензии MIT.\r\n\r\n"
                            L"Программа предоставляется «как есть» (AS IS), без каких-либо гарантий. Автор не несёт ответственности за любой ущерб, возникший в результате её использования.\r\n\r\n"
                            L"Изменённые или сторонние сборки не связаны с автором.\r\n\r\n"
                            L"© 2026 Husanboy Umarov (ascoder)\r\n"
                            L"Официальный источник: github.com/HusanboyUmarov/Harfbek\r\n"
                            L"Полный текст: файл LICENSE.",
                            L"Harfbek is free and open-source software, distributed under the MIT license.\r\n\r\n"
                            L"The software is provided \"AS IS\", without any warranty. The author is not liable for any damages arising from its use.\r\n\r\n"
                            L"Modified or third-party builds are not associated with the author.\r\n\r\n"
                            L"© 2026 Husanboy Umarov (ascoder)\r\n"
                            L"Official source: github.com/HusanboyUmarov/Harfbek\r\n"
                            L"Full text: LICENSE file." },
    /*S_LIC_OK*/          { L"Tuşunarli", L"Понятно", L"OK" },

    // --- til tanlash ---
    /*S_LANG_TITLE*/      { L"Til / Язык / Language", L"Til / Язык / Language", L"Til / Язык / Language" },
    /*S_LANG_PROMPT*/     { L"Dastur tilini tanlang:",
                            L"Выберите язык программы:",
                            L"Choose the program language:" },

    // --- qo'llanma ---
    /*S_TUT_TITLE*/       { L"Harfbek bilan tanişuv",
                            L"Знакомство с Harfbek",
                            L"Getting started with Harfbek" },
    /*S_TUT_NEXT*/        { L"Keyingi ▶", L"Далее ▶", L"Next ▶" },
    /*S_TUT_BACK*/        { L"◀ Orqaga", L"◀ Назад", L"◀ Back" },
    /*S_TUT_FINISH*/      { L"Tugatiş", L"Готово", L"Finish" },
    /*S_TUT_STEP*/        { L"Qadam %d / %d", L"Шаг %d / %d", L"Step %d / %d" },

    // --- sozlamalar ---
    /*S_SET_TITLE*/         { L"Sozlamalar", L"Настройки", L"Settings" },
    /*S_SET_LANGUAGE*/      { L"Til:", L"Язык:", L"Language:" },
    /*S_SET_TRIGGER*/       { L"Trigger tugma:", L"Клавиша-триггер:", L"Trigger key:" },
    /*S_SET_STARTUP*/       { L"Windows bilan işga tuşsin",
                              L"Запускать вместе с Windows",
                              L"Run at Windows startup" },
    /*S_SET_SAVE*/          { L"Saqlaş", L"Сохранить", L"Save" },
    /*S_SET_CANCEL*/        { L"Bekor qiliş", L"Отмена", L"Cancel" },
    /*S_SET_APOSTROPHE*/    { L"Apostrof (o' g'):", L"Апостроф (o' g'):", L"Apostrophe (o' g'):" },

    // --- trigger nomlari (faqat sinalgan tugmalar) ---
    /*S_TRIG_RALT*/       { L"Öng Alt (AltGr)", L"Правый Alt (AltGr)", L"Right Alt (AltGr)" },
    /*S_TRIG_LALT*/       { L"Çap Alt", L"Левый Alt", L"Left Alt" },
    /*S_TRIG_BOTHALT*/    { L"Ikkala Alt", L"Оба Alt", L"Both Alt" },
    /*S_TRIG_RCTRL*/      { L"Öng Ctrl", L"Правый Ctrl", L"Right Ctrl" },
    /*S_TRIG_RSHIFT*/     { L"Öng Shift", L"Правый Shift", L"Right Shift" },
    /*S_TRIG_CAPS*/       { L"CapsLock", L"CapsLock", L"CapsLock" },
    /*S_TRIG_APPS*/       { L"Menyu tugmasi (≣)", L"Клавиша меню (≣)", L"Menu key (≣)" },
    /*S_TRIG_SCROLL*/     { L"Scroll Lock", L"Scroll Lock", L"Scroll Lock" },
    /*S_TRIG_PAUSE*/      { L"Pause", L"Pause", L"Pause" },
    /*S_TRIG_F12*/        { L"F12", L"F12", L"F12" },

    // --- dasturni o'chirish (uninstall) ---
    /*S_MENU_UNINSTALL*/  { L"Dasturni öçiriş...",
                            L"Удалить программу...",
                            L"Uninstall..." },
    /*S_UNINST_CONFIRM*/  { L"Harfbek öçiriladi va barça ma'lumotlari tozalanadi:\r\n\r\n"
                            L"  •  sozlamalar (AppData\\Harfbek)\r\n"
                            L"  •  Windows bilan işga tuşiş yozuvi\r\n"
                            L"  •  iş stolidagi yorliq\r\n\r\n"
                            L"harfbek.exe fayli qoladi - qayta örnatiş uçun uni yana işga tuşiring.\r\n\r\n"
                            L"Davom ettirilsinmi?",
                            L"Harfbek будет удалён, все его данные будут очищены:\r\n\r\n"
                            L"  •  настройки (AppData\\Harfbek)\r\n"
                            L"  •  запись автозапуска Windows\r\n"
                            L"  •  ярлык на рабочем столе\r\n\r\n"
                            L"Файл harfbek.exe останется - для переустановки просто запустите его снова.\r\n\r\n"
                            L"Продолжить?",
                            L"Harfbek will be removed and all its data cleaned:\r\n\r\n"
                            L"  •  settings (AppData\\Harfbek)\r\n"
                            L"  •  Windows startup entry\r\n"
                            L"  •  desktop shortcut\r\n\r\n"
                            L"The harfbek.exe file will remain - to reinstall, just run it again.\r\n\r\n"
                            L"Continue?" },

    // --- avto rejim ---
    /*S_MENU_AUTOMODE*/   { L"Avto rejim (o' g' sh ch → ö ğ ş ç)",
                            L"Авторежим (o' g' sh ch → ö ğ ş ç)",
                            L"Auto mode (o' g' sh ch → ö ğ ş ç)" },
    // S_TOOLTIP_AUTO da %s YO'Q (tooltip formatlanmaydi).
    /*S_TOOLTIP_AUTO*/    { L"Harfbek — avto rejim: o' g' sh ch → ö ğ ş ç",
                            L"Harfbek — авторежим: o' g' sh ch → ö ğ ş ç",
                            L"Harfbek — auto mode: o' g' sh ch → ö ğ ş ç" },
    /*S_MODE_TRIGGER*/    { L"Trigger tugma + harf",
                            L"Клавиша-триггер + буква",
                            L"Trigger key + letter" },
    /*S_MODE_AUTO*/       { L"Avtomatik: o' g' sh ch → ö ğ ş ç",
                            L"Автоматически: o' g' sh ch → ö ğ ş ç",
                            L"Automatic: o' g' sh ch → ö ğ ş ç" },

    // --- sozlamalar tablari ---
    /*S_TAB_GENERAL*/     { L"Umumiy", L"Общие", L"General" },
    /*S_TAB_MODE*/        { L"Rejim", L"Режим", L"Mode" },
    /*S_TAB_HOTKEYS*/     { L"Tezkor tugmalar", L"Горячие клавиши", L"Hotkeys" },
    /*S_TAB_EXCLUDE*/     { L"Istisnolar", L"Исключения", L"Exclusions" },

    // --- sozlamalar: rejim tabi ---
    /*S_SET_NOTIFY*/      { L"Rejim almaşganda ekranda xabar körsatilsin",
                            L"Показывать уведомление на экране при смене режима",
                            L"Show an on-screen message when the mode changes" },
    /*S_SET_RULES*/       { L"Avtomatik almaştirilsin:",
                            L"Заменять автоматически:",
                            L"Replace automatically:" },
    // S_SET_EXAMPLE_TRIGGER da aynan bitta %s bor: trigger keycap nomi.
    /*S_SET_EXAMPLE_TRIGGER*/ { L"Misol: %s + o → ö",
                            L"Пример: %s + o → ö",
                            L"Example: %s + o → ö" },
    /*S_SET_EXAMPLE_AUTO*/  { L"Misol: o'zbek → özbek, Toshkent → Toşkent",
                            L"Пример: o'zbek → özbek, Toshkent → Toşkent",
                            L"Example: o'zbek → özbek, Toshkent → Toşkent" },

    // --- sozlamalar: tezkor tugmalar tabi ---
    /*S_HK_MODE*/         { L"Rejimni almaştiriş:", L"Переключить режим:", L"Switch mode:" },
    /*S_HK_ENABLE*/       { L"Yoqiş / öçiriş:", L"Включить / выключить:", L"Enable / disable:" },
    /*S_HK_CONVERT*/      { L"Belgilangan matnni ögiriş:",
                            L"Преобразовать выделенный текст:",
                            L"Convert selected text:" },
    /*S_HK_HINT*/         { L"Maydonni tanlab kerakli tugmalarni bosing. Öçiriş uçun — Backspace.",
                            L"Выберите поле и нажмите нужные клавиши. Для отключения — Backspace.",
                            L"Select the field and press the keys. To clear — Backspace." },
    // S_HK_BUSY da aynan bitta %s bor: tezkor tugma matni (Hotkey_Format).
    /*S_HK_BUSY*/         { L"\"%s\" tezkor tugmasi band (boşqa dastur işlatmoqda) yoki takrorlangan. Boşqasini tanlang.",
                            L"Сочетание «%s» занято (используется другой программой) или повторяется. Выберите другое.",
                            L"The hotkey \"%s\" is busy (used by another program) or duplicated. Choose another one." },

    // --- sozlamalar: istisnolar tabi ---
    /*S_EX_HINT*/         { L"Bu dasturlarda avto rejim işlamaydi (terminallar doim istisno):",
                            L"В этих программах авторежим не работает (терминалы всегда исключены):",
                            L"Auto mode is off in these programs (terminals are always excluded):" },
    /*S_EX_ADD*/          { L"Qöşiş", L"Добавить", L"Add" },
    /*S_EX_BROWSE*/       { L"Tanlaş...", L"Обзор...", L"Browse..." },
    /*S_EX_REMOVE*/       { L"Öçiriş", L"Удалить", L"Remove" },
    /*S_EX_FILTER*/       { L"Dasturlar (*.exe)", L"Программы (*.exe)", L"Programs (*.exe)" },

    // --- ekrandagi bildirishnoma ---
    /*S_OSD_MODE_TRIGGER*/{ L"Trigger rejimi", L"Режим триггера", L"Trigger mode" },
    /*S_OSD_MODE_AUTO*/   { L"Avto rejim", L"Авторежим", L"Auto mode" },
    /*S_OSD_ENABLED*/     { L"Harfbek yoqildi", L"Harfbek включён", L"Harfbek enabled" },
    /*S_OSD_DISABLED*/    { L"Harfbek öçirildi", L"Harfbek выключен", L"Harfbek disabled" },

    // --- tanishtiruv (wizard) ---
    /*S_WIZ_1_TITLE*/     { L"Xuş kelibsiz!", L"Добро пожаловать!", L"Welcome!" },
    /*S_WIZ_1_BODY*/      { L"Özbek alifbosi özgardi: o' g' sh ch örniga endi ö ğ ş ç yoziladi. "
                            L"Harfbek bu harflarni oson yoziş uçun yordam beradi.",
                            L"Алфавит узбекского языка изменился: вместо o' g' sh ch теперь пишутся ö ğ ş ç. "
                            L"Harfbek помогает легко их вводить.",
                            L"The Uzbek alphabet has changed: ö ğ ş ç are now used instead of o' g' sh ch. "
                            L"Harfbek helps you type them easily." },
    /*S_WIZ_2_TITLE*/     { L"1-rejim: Trigger tugma",
                            L"Режим 1: клавиша-триггер",
                            L"Mode 1: Trigger key" },
    /*S_WIZ_2_BODY*/      { L"Trigger tugmani bosib turib harfni bosing: o → ö, g → ğ, s → ş, c → ç. "
                            L"Katta harf uçun Shift ni ham bosing.",
                            L"Удерживая клавишу-триггер, нажмите букву: o → ö, g → ğ, s → ş, c → ç. "
                            L"Для заглавной добавьте Shift.",
                            L"Hold the trigger key and press a letter: o → ö, g → ğ, s → ş, c → ç. "
                            L"Add Shift for uppercase." },
    /*S_WIZ_3_TITLE*/     { L"2-rejim: Avtomatik",
                            L"Режим 2: автоматический",
                            L"Mode 2: Automatic" },
    /*S_WIZ_3_BODY*/      { L"Odatdagidek o' g' sh ch yozing — ular özi ö ğ ş ç ga aylanadi. "
                            L"Darhol Backspace bossangiz, asl holiga qaytadi. Terminallarda işlamaydi.",
                            L"Пишите o' g' sh ch как обычно — они сами станут ö ğ ş ç. "
                            L"Сразу нажмите Backspace — вернётся исходный вид. В терминалах не работает.",
                            L"Type o' g' sh ch as usual — they turn into ö ğ ş ç automatically. "
                            L"Press Backspace right away to undo. Doesn't work in terminals." },
    /*S_WIZ_4_TITLE*/     { L"Rejimni tanlang", L"Выберите режим", L"Choose a mode" },
    /*S_WIZ_4_BODY*/      { L"Qaysi usul sizga qulay? Keyin Sozlamalardan özgartiriş mumkin.",
                            L"Какой способ вам удобнее? Позже можно изменить в Настройках.",
                            L"Which way suits you? You can change it later in Settings." },
    /*S_WIZ_5_TITLE*/     { L"Tezkor tugmalar", L"Горячие клавиши", L"Hotkeys" },
    /*S_WIZ_5_BODY*/      { L"Belgilangan matnni Ctrl + öng tugma bilan ham ögiriş mumkin. "
                            L"Tugmalarni Sozlamalarda özgartiring.",
                            L"Выделенный текст можно преобразовать и через Ctrl + правую кнопку мыши. "
                            L"Клавиши меняются в Настройках.",
                            L"Selected text can also be converted with Ctrl + right-click. "
                            L"Change the keys in Settings." },
    /*S_WIZ_6_TITLE*/     { L"Tayyor!", L"Готово!", L"Ready!" },
    /*S_WIZ_6_BODY*/      { L"Harfbek soat yonidagi ikonka orqali boşqariladi.",
                            L"Harfbek управляется через значок возле часов.",
                            L"Harfbek is controlled from the tray icon near the clock." },
};

const wchar_t* T(StrId id)
{
    if (id < 0 || id >= S_STR_COUNT) return L"";
    int lang = (int)g_settings.lang;
    if (lang < 0 || lang >= LANG_COUNT) lang = LANG_UZ;
    const wchar_t* s = STR[id][lang];
    return s ? s : STR[id][LANG_EN];
}
