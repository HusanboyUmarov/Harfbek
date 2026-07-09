#pragma once
#include "settings.h"

// Barcha interfeys matnlari uchun identifikatorlar
enum StrId {
    // umumiy
    S_APP_TITLE, S_TOOLTIP, S_ERROR,
    S_ALREADY_RUNNING, S_HOOK_FAILED,

    // tray menyu
    S_MENU_ENABLED, S_MENU_SETTINGS, S_MENU_LANGUAGE,
    S_MENU_TUTORIAL, S_MENU_ABOUT, S_MENU_STARTUP, S_MENU_EXIT,

    // matnni o'girish menyusi (Ctrl+o'ng-tugma)
    S_CONV_TODIGRAPH, S_CONV_TOSPECIAL,

    // dastur haqida
    S_ABOUT_TITLE, S_ABOUT_DESC, S_ABOUT_DEV, S_ABOUT_OPEN, S_ABOUT_GITHUB, S_ABOUT_CLOSE,

    // litsenziya / ogohlantirish
    S_LIC_TITLE, S_LIC_BODY, S_LIC_OK,

    // til tanlash
    S_LANG_TITLE, S_LANG_PROMPT,

    // qo'llanma
    S_TUT_TITLE, S_TUT_NEXT, S_TUT_BACK, S_TUT_FINISH, S_TUT_STEP,
    S_TUT_1_TITLE, S_TUT_1_BODY,
    S_TUT_2_TITLE, S_TUT_2_BODY,
    S_TUT_3_TITLE, S_TUT_3_BODY,
    S_TUT_4_TITLE, S_TUT_4_BODY,

    // sozlamalar
    S_SET_TITLE, S_SET_LANGUAGE, S_SET_TRIGGER, S_SET_CAPTURE,
    S_SET_CAPTURE_PROMPT, S_SET_STARTUP, S_SET_SAVE, S_SET_CANCEL,
    S_SET_WARN_BADKEY, S_SET_CURRENT, S_SET_APOSTROPHE,

    // trigger nomlari
    S_TRIG_RALT, S_TRIG_LALT, S_TRIG_BOTHALT, S_TRIG_RCTRL, S_TRIG_CAPS, S_TRIG_CUSTOM,

    S_STR_COUNT
};

// Joriy tildagi (g_settings.lang) matnni qaytaradi
const wchar_t* T(StrId id);
