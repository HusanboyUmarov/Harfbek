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

    // sozlamalar
    S_SET_TITLE, S_SET_LANGUAGE, S_SET_TRIGGER,
    S_SET_STARTUP, S_SET_SAVE, S_SET_CANCEL,
    S_SET_APOSTROPHE,

    // trigger nomlari (faqat sinalgan tugmalar ro'yxati)
    S_TRIG_RALT, S_TRIG_LALT, S_TRIG_BOTHALT, S_TRIG_RCTRL, S_TRIG_RSHIFT,
    S_TRIG_CAPS, S_TRIG_APPS, S_TRIG_SCROLL, S_TRIG_PAUSE, S_TRIG_F12,

    // dasturni o'chirish (uninstall)
    S_MENU_UNINSTALL, S_UNINST_CONFIRM,

    // avto rejim
    S_MENU_AUTOMODE, S_TOOLTIP_AUTO, S_MODE_TRIGGER, S_MODE_AUTO,

    // sozlamalar tablari, tezkor tugmalar, istisnolar, bildirishnoma
    S_TAB_GENERAL, S_TAB_MODE, S_TAB_HOTKEYS, S_TAB_EXCLUDE,
    S_SET_NOTIFY, S_SET_RULES, S_SET_EXAMPLE_TRIGGER, S_SET_EXAMPLE_AUTO,
    S_HK_MODE, S_HK_ENABLE, S_HK_CONVERT, S_HK_HINT, S_HK_BUSY,
    S_EX_HINT, S_EX_ADD, S_EX_BROWSE, S_EX_REMOVE, S_EX_FILTER,
    S_OSD_MODE_TRIGGER, S_OSD_MODE_AUTO, S_OSD_ENABLED, S_OSD_DISABLED,

    // tanishtiruv (wizard)
    S_WIZ_1_TITLE, S_WIZ_1_BODY, S_WIZ_2_TITLE, S_WIZ_2_BODY,
    S_WIZ_3_TITLE, S_WIZ_3_BODY, S_WIZ_4_TITLE, S_WIZ_4_BODY,
    S_WIZ_5_TITLE, S_WIZ_5_BODY, S_WIZ_6_TITLE, S_WIZ_6_BODY,

    // flyout, tezkor tugma yozish, istisnolar, sozlamalar sahifalari
    S_FLY_ACTIVE, S_FLY_PAUSED, S_FLY_GUIDE, S_HK_RECORD,
    S_HK_NONE, S_EX_EMPTY, S_NAV_ABOUT, S_SET_APPEARANCE_HINT,

    // sinov maydoni (playground)
    S_NAV_PLAYGROUND, S_PG_HINT, S_PG_PLACEHOLDER, S_PG_CHALLENGES, S_PG_CLEAR,
    S_PG_COUNTER, S_PG_MODE_BADGE, S_PG_DONE, S_PG_UNSAVED, S_PG_TRY_NOW,

    S_STR_COUNT
};

// Joriy tildagi (g_settings.lang) matnni qaytaradi
const wchar_t* T(StrId id);
