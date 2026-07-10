#include "i18n.h"

// Har bir qator: { o'zbekcha, ruscha, inglizcha }  -  tartib StrId enum bilan bir xil.
// O'zbekcha matnlar YANGI ALIFBODA (ş ç ğ ö) yozilgan.
static const wchar_t* STR[S_STR_COUNT][LANG_COUNT] = {
    // --- umumiy ---
    /*S_APP_TITLE*/       { L"Harfbek", L"Harfbek", L"Harfbek" },
    /*S_TOOLTIP*/         { L"Harfbek — AltGr bilan özbek harflari",
                            L"Harfbek — узбекские буквы через AltGr",
                            L"Harfbek — Uzbek letters via AltGr" },
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
    /*S_MENU_TUTORIAL*/   { L"Qöllanma...", L"Руководство...", L"Guide..." },
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
                            L"Belgilangan matnni o' g' sh ch ↔ ö ğ ş ç ga o'giradi.",
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
                            L"O'zgartirilgan yoki boşqa manbadan olingan versiyalar muallifga aloqador emas.\r\n\r\n"
                            L"© 2026 Husanboy Umarov (ascoder)\r\n"
                            L"Rasmiy manba: github.com/HusanboyUmarov/Harfbek\r\n"
                            L"To'liq matn: LICENSE fayli.",
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
    /*S_TUT_TITLE*/       { L"Qöllanma — qanday işlatiş",
                            L"Руководство — как пользоваться",
                            L"Guide — how to use" },
    /*S_TUT_NEXT*/        { L"Keyingi ▶", L"Далее ▶", L"Next ▶" },
    /*S_TUT_BACK*/        { L"◀ Orqaga", L"◀ Назад", L"◀ Back" },
    /*S_TUT_FINISH*/      { L"Tugatiş", L"Готово", L"Finish" },
    /*S_TUT_STEP*/        { L"Qadam %d / %d", L"Шаг %d / %d", L"Step %d / %d" },

    /*S_TUT_1_TITLE*/     { L"Harf yoziş", L"Ввод буквы", L"Typing a letter" },
    /*S_TUT_1_BODY*/      { L"Trigger tugmani (standart: AltGr — öng Alt) bosib turib harfni bosing.",
                            L"Удерживая клавишу-триггер (по умолч.: AltGr — правый Alt), нажмите букву.",
                            L"Hold the trigger key (default: AltGr — right Alt) and press the letter." },
    /*S_TUT_2_TITLE*/     { L"Katta harf", L"Заглавная буква", L"Uppercase" },
    /*S_TUT_2_BODY*/      { L"Katta harf uçun Shift ni ham bosing. CapsLock ham işlaydi.",
                            L"Для заглавной буквы добавьте Shift. CapsLock тоже работает.",
                            L"For an uppercase letter, also press Shift. CapsLock works too." },
    /*S_TUT_3_TITLE*/     { L"Barça harflar", L"Все буквы", L"All letters" },
    /*S_TUT_3_BODY*/      { L"Törtala maxsus harf şu tartibda yoziladi.",
                            L"Все четыре особые буквы вводятся так.",
                            L"All four special letters are typed like this." },
    /*S_TUT_4_TITLE*/     { L"Sozlamalar", L"Настройки", L"Settings" },
    /*S_TUT_4_BODY*/      { L"Soat yonidagi ikonkaga öng tugma bosing → Sozlamalar. U yerdan til va trigger tugmani özgartiring.",
                            L"Правый клик по значку у часов → Настройки. Там можно сменить язык и клавишу-триггер.",
                            L"Right-click the tray icon → Settings. There you can change the language and the trigger key." },

    // --- sozlamalar ---
    /*S_SET_TITLE*/         { L"Sozlamalar", L"Настройки", L"Settings" },
    /*S_SET_LANGUAGE*/      { L"Til:", L"Язык:", L"Language:" },
    /*S_SET_TRIGGER*/       { L"Trigger tugma:", L"Клавиша-триггер:", L"Trigger key:" },
    /*S_SET_STARTUP*/       { L"Windows bilan işga tuşsin",
                              L"Запускать вместе с Windows",
                              L"Run at Windows startup" },
    /*S_SET_SAVE*/          { L"Saqlaş", L"Сохранить", L"Save" },
    /*S_SET_CANCEL*/        { L"Bekor qiliş", L"Отмена", L"Cancel" },
    /*S_SET_CURRENT*/       { L"Joriy:", L"Текущая:", L"Current:" },
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
};

const wchar_t* T(StrId id)
{
    if (id < 0 || id >= S_STR_COUNT) return L"";
    int lang = (int)g_settings.lang;
    if (lang < 0 || lang >= LANG_COUNT) lang = LANG_UZ;
    const wchar_t* s = STR[id][lang];
    return s ? s : STR[id][LANG_EN];
}
